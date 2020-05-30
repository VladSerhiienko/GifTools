#include "GifToolsFFmpegVideoStream.h"
#include "GifToolsFFmpegInputStream.h"
#include "GifToolsProgress.h"

#include "GifToolsBuffer.h"
#include "GifToolsImage.h"
#include "GifToolsManagedTypes.h"

struct FFmpegVideoFrameImpl;
struct FFmpegVideoStreamImpl;

template <>
uint8_t giftools::managedType<giftools::FFmpegVideoFrame>() {
    return static_cast<uint8_t>(giftools::BuiltinManagedType::FFmpegVideoFrame);
}

template <>
uint8_t giftools::managedType<FFmpegVideoFrameImpl>() {
    return static_cast<uint8_t>(giftools::BuiltinManagedType::FFmpegVideoFrame);
}

template <>
uint8_t giftools::managedType<giftools::FFmpegVideoStream>() {
    return static_cast<uint8_t>(giftools::BuiltinManagedType::FFmpegVideoStream);
}

template <>
uint8_t giftools::managedType<FFmpegVideoStreamImpl>() {
    return static_cast<uint8_t>(giftools::BuiltinManagedType::FFmpegVideoStream);
}

#ifndef GIFTOOLS_USE_FFMPEG

struct FFmpegVideoFrameImpl : giftools::FFmpegVideoFrame {
    FFmpegVideoFrameImpl() = default;
    ~FFmpegVideoFrameImpl() override = default;
    double estimatedSampleTimeSeconds() const override { return 0; }
    const giftools::Image* image() const override { return nullptr; }
};

struct FFmpegVideoStreamImpl : giftools::FFmpegVideoStream {
    FFmpegVideoStreamImpl() = default;
    ~FFmpegVideoStreamImpl() override = default;
    size_t frameWidth() const override { return 0; }
    size_t frameHeight() const override { return 0; }
    size_t frameCount() const override { return 0; }
    double estimatedTotalDurationSeconds() const override { return 0; }
    double estimatedFrameDurationSeconds() const override { return 0; }
};

giftools::UniqueManagedObj<giftools::FFmpegVideoStream> giftools::ffmpegVideoStreamOpen(const giftools::FFmpegInputStream* ffmpegInputStream) { return {}; }
giftools::UniqueManagedObj<giftools::FFmpegVideoFrame> giftools::ffmpegVideoStreamPickBestFrame(const giftools::FFmpegVideoStream* ffmpegVideoStream, double sampleTime) { return {}; }
void giftools::ffmpegVideoStreamClose(giftools::FFmpegVideoStream* ffmpegVideoStream) {}

#else

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/pixdesc.h>
    #include <libswscale/swscale.h>
}

constexpr AVPixelFormat DESIRED_FMT = AV_PIX_FMT_RGBA;

struct FFmpegStagingVideoFrame {
    double timeSecondsEst = 0.0;
    int64_t timeTimeBaseEst = 0;
    int width = -1;
    int height = -1;
    int alignment = 1;
    AVPixelFormat decodeFmt = DESIRED_FMT;
    std::vector<uint8_t> imageByteBuffer = {};
    AVFrame* encodedFrame = nullptr;
    AVFrame* decodedFrame = nullptr;
    AVPacket packet = {};
};

struct FFmpegVideoFrameImpl : public giftools::FFmpegVideoFrame {
    double timeSecondsEst = 0.0;
    giftools::UniqueManagedObj<giftools::Image> imageObj = {};
    
    FFmpegVideoFrameImpl() = default;
    ~FFmpegVideoFrameImpl() override { imageObj = nullptr; }
    
    double estimatedSampleTimeSeconds() const override { return timeSecondsEst; }
    const giftools::Image* image() const override { return imageObj.get(); }
};

struct FFmpegVideoStreamImpl : public giftools::FFmpegVideoStream {
    giftools::Buffer* contentsBufferObj = nullptr;
    const giftools::FFmpegInputStream* streamObj = nullptr;
    
    std::vector<giftools::UniqueManagedObj<giftools::FFmpegVideoFrame>> preparedFrames = {};
    
    AVFormatContext* fmtContext = nullptr;
    std::vector<uint8_t> probeBuffer = {};
    AVProbeData probeData = {};
    AVInputFormat* inputFmt = nullptr;
    AVCodec* primaryCodec = nullptr;
    AVCodecContext* primaryCodecContext = nullptr;
    AVStream* primaryStream = nullptr;
    int primaryStreamIndex = -1;
    SwsContext* swsContext = nullptr;
    int64_t frameCount_ = 0;
    double durationSeconds = 0.0;
    int64_t durationTimeBase = 0;
    int64_t frameDurationTimeBase = 0;
    double frameDurationSeconds = 0.0;
    double primaryStreamTimeBase = 0.0;
    int width = -1;
    int height = -1;
    int alignment = 1;
    AVPixelFormat decodeFmt = DESIRED_FMT;
    
    FFmpegVideoStreamImpl() = default;
    ~FFmpegVideoStreamImpl() override { close(); }
    
    void close() {
        clearPreparedFrames();
        
        sws_freeContext(swsContext);
        avcodec_close(primaryCodecContext);
        avformat_close_input(&fmtContext);
        new (this) FFmpegVideoStreamImpl();
    }
    
    void clearPreparedFrames(bool wipePreparedFrames = true) {
        if (wipePreparedFrames) { decltype(preparedFrames)().swap(preparedFrames); return; }
        preparedFrames.clear();
    }
    
    void sortPreparedFrames() {
        using namespace giftools;
        std::sort(preparedFrames.begin(),
                  preparedFrames.end(),
                  [&](const UniqueManagedObj<FFmpegVideoFrame>& lhs, const UniqueManagedObj<FFmpegVideoFrame>& rhs){
                      return lhs->estimatedSampleTimeSeconds() < rhs->estimatedSampleTimeSeconds();
                  });
    }
    
    size_t frameWidth() const override { return width; }
    size_t frameHeight() const override { return height; }
    size_t frameCount() const override { return frameCount_; }
    double estimatedTotalDurationSeconds() const override { return durationSeconds; }
    double estimatedFrameDurationSeconds() const override { return frameDurationSeconds; }
};

giftools::UniqueManagedObj<giftools::FFmpegVideoStream>
giftools::ffmpegVideoStreamOpen(const giftools::FFmpegInputStream* ffmpegInputStream) {
    if (!ffmpegInputStream) { return {}; }
    
    auto& progressToken = *getMutableProgressToken();
    auto& cancellationToken = *getCancellationToken();
    
    progressToken.setProgress(0);
    
    auto videoStreamPtr = giftools::managedObjStorageDefault().make<FFmpegVideoStreamImpl>();
    auto& videoStream = *videoStreamPtr;
    
    progressToken.setProgress(0.1);
    
    videoStream.streamObj = ffmpegInputStream;
    videoStream.contentsBufferObj = (giftools::Buffer*)ffmpegInputStream->buffer();

    av_register_all();
#if defined(_DEBUG) || defined(DEBUG)
    av_log_set_level(AV_LOG_TRACE);
#else
    av_log_set_level(AV_LOG_ERROR);
#endif
    
    videoStream.fmtContext = avformat_alloc_context();
    videoStream.fmtContext->pb = avio_alloc_context(videoStream.contentsBufferObj->mutableData(),
                                                    videoStream.contentsBufferObj->size(),
                                                    0,
                                                    (void*)videoStream.streamObj,
                                                    videoStream.streamObj->ffmpegInputStreamReadFnPtr(),
                                                    nullptr,
                                                    videoStream.streamObj->ffmpegInputStreamSeekFnPtr());
    if (!videoStream.fmtContext->pb) { return {}; }
    
    progressToken.setProgress(0.2);

    videoStream.probeData.filename = "stream";
    videoStream.probeData.buf_size = std::min<int>(videoStream.contentsBufferObj->size(), 4096);
    videoStream.probeBuffer.resize(videoStream.probeData.buf_size);
    videoStream.probeData.buf = videoStream.probeBuffer.data();
    memcpy(videoStream.probeData.buf, videoStream.contentsBufferObj->data(), 4096);
    
    progressToken.setProgress(0.3);

    videoStream.inputFmt = av_probe_input_format(&videoStream.probeData, 1);
    if (!videoStream.inputFmt) { videoStream.inputFmt = av_probe_input_format(&videoStream.probeData, 0); }

    videoStream.probeData.buf = nullptr;
    std::vector<uint8_t>().swap(videoStream.probeBuffer);

    if (!videoStream.inputFmt) { return {}; }
    videoStream.inputFmt->flags |= AVFMT_NOFILE;
    
    int ret = 0;

    // clang-format off
    ret = avformat_open_input(&videoStream.fmtContext, videoStream.probeData.filename, videoStream.inputFmt, nullptr);
    if (ret < 0) { return {}; }
    ret = avformat_find_stream_info(videoStream.fmtContext, nullptr);
    if (ret < 0) { return {}; }
    
    progressToken.setProgress(0.5);

    videoStream.primaryStreamIndex = av_find_best_stream(videoStream.fmtContext, AVMEDIA_TYPE_VIDEO, -1, -1, &videoStream.primaryCodec, 0);
    if (videoStream.primaryStreamIndex < 0 || videoStream.primaryStreamIndex >= videoStream.fmtContext->nb_streams) { return {}; }
    // clang-format on

    videoStream.primaryStream = videoStream.fmtContext->streams[videoStream.primaryStreamIndex];
    videoStream.primaryStreamTimeBase = av_q2d(videoStream.primaryStream->time_base);
    videoStream.frameCount_ = videoStream.primaryStream->nb_frames;
    videoStream.durationTimeBase = videoStream.primaryStream->duration;
    videoStream.frameDurationTimeBase = videoStream.durationTimeBase / videoStream.frameCount_;
    videoStream.durationSeconds = double(videoStream.durationTimeBase) * videoStream.primaryStreamTimeBase;
    videoStream.frameDurationSeconds = double(videoStream.frameDurationTimeBase) * videoStream.primaryStreamTimeBase;

    // clang-format off
    // TODO(vserhiienko): deprecated.
    videoStream.primaryCodecContext = videoStream.primaryStream->codec;
    if (int ret = avcodec_open2(videoStream.primaryCodecContext, videoStream.primaryCodec, nullptr); ret < 0) { return {}; }
    // clang-format on
    
    progressToken.setProgress(0.8);

    videoStream.width = videoStream.primaryCodecContext->width;
    videoStream.height = videoStream.primaryCodecContext->height;
    videoStream.swsContext = sws_getCachedContext(videoStream.swsContext,
                                                  videoStream.primaryCodecContext->width,
                                                  videoStream.primaryCodecContext->height,
                                                  videoStream.primaryCodecContext->pix_fmt,
                                                  videoStream.width,
                                                  videoStream.height,
                                                  videoStream.decodeFmt,
                                                  SWS_BICUBIC,
                                                  nullptr,
                                                  nullptr,
                                                  nullptr);
    if (!videoStream.swsContext) { return {}; }

    progressToken.setProgress(1);
    return videoStreamPtr;
}

void giftools::ffmpegVideoStreamClose(FFmpegVideoStream* ffmpegVideoStream) {
    if (!ffmpegVideoStream) { return; }
    
    auto ffmpegVideoStreamImpl = (FFmpegVideoStreamImpl*)ffmpegVideoStream;
    ffmpegVideoStreamImpl->close();
}

struct FlushBuffersGuard {
    FFmpegVideoStreamImpl& videoStream;

    static void flushBuffers(FFmpegVideoStreamImpl& videoStream) {
        avcodec_flush_buffers(videoStream.primaryCodecContext);
        av_seek_frame(videoStream.fmtContext, videoStream.primaryStreamIndex, 0, 0);
    }

    FlushBuffersGuard(FFmpegVideoStreamImpl& videoStream) : videoStream(videoStream) { flushBuffers(videoStream); }
    ~FlushBuffersGuard() { flushBuffers(videoStream); }
};


struct FreeFramePacketGuard {
    FFmpegStagingVideoFrame& videoFrame;
    
    static void videoFrameFreePacket(FFmpegStagingVideoFrame& stagingFrame) {
        if (stagingFrame.packet.buf) { av_buffer_unref(&stagingFrame.packet.buf); }
        stagingFrame.packet.data = nullptr;
        stagingFrame.packet.size = 0;
        av_packet_free_side_data(&stagingFrame.packet);
    }

    void freePacket() { videoFrameFreePacket(videoFrame); }
    FreeFramePacketGuard(FFmpegStagingVideoFrame& videoFrame) : videoFrame(videoFrame) { freePacket(); }
    ~FreeFramePacketGuard() { freePacket(); }
};

struct FreeFrameGuard {
    FFmpegStagingVideoFrame& videoFrame;
    
    static void freeFrame(FFmpegStagingVideoFrame& videoFrame) {
        av_frame_free(&videoFrame.decodedFrame);
        av_frame_free(&videoFrame.encodedFrame);
    }
    
    FreeFrameGuard(FFmpegStagingVideoFrame& videoFrame) : videoFrame(videoFrame) {}
    ~FreeFrameGuard() { freeFrame(videoFrame); }
};

giftools::UniqueManagedObj<giftools::FFmpegVideoFrame>
ffmpegVideoFrameFromStaging(const FFmpegVideoStreamImpl& videoStream, FFmpegStagingVideoFrame& ffmpegStagingVideoFrame) {
    int ret;
    
    GIFTOOLS_LOGT("sws_scale");
    ret = sws_scale(videoStream.swsContext,
                    ffmpegStagingVideoFrame.decodedFrame->data,
                    ffmpegStagingVideoFrame.decodedFrame->linesize,
                    0,
                    ffmpegStagingVideoFrame.decodedFrame->height,
                    ffmpegStagingVideoFrame.encodedFrame->data,
                    ffmpegStagingVideoFrame.encodedFrame->linesize);
    if (ret < 0) { GIFTOOLS_LOGE("sws_scale failed."); return {}; }
    
    auto frame = giftools::managedObjStorageDefault().make<FFmpegVideoFrameImpl>();
    frame->timeSecondsEst = ffmpegStagingVideoFrame.timeSecondsEst;
    
    assert(ffmpegStagingVideoFrame.decodeFmt == DESIRED_FMT);
    frame->imageObj = giftools::imageLoadFromMemory(videoStream.width,
                                                    videoStream.height,
                                                    giftools::PixelFormatR8G8B8A8Unorm,
                                                    ffmpegStagingVideoFrame.imageByteBuffer.data(),
                                                    ffmpegStagingVideoFrame.imageByteBuffer.size());
    return frame;
}

giftools::UniqueManagedObj<giftools::FFmpegVideoFrame>
ffmpegCloneFrame(const giftools::FFmpegVideoFrame& targetFrame) {

    auto frame = giftools::managedObjStorageDefault().make<FFmpegVideoFrameImpl>();
    frame->timeSecondsEst = targetFrame.estimatedSampleTimeSeconds();
    frame->imageObj = giftools::imageClone(targetFrame.image());
    return frame;
}

giftools::UniqueManagedObj<giftools::FFmpegVideoFrame>
ffmpegVideoStreamPickBestFrameFromPrepared(const giftools::FFmpegVideoStream* ffmpegVideoStream, double sampleTime, double tolerance = -1.0) {
    if (!ffmpegVideoStream) { GIFTOOLS_LOGE("Caught null video stream."); return {}; }
    
    auto& videoStream = (FFmpegVideoStreamImpl&)*ffmpegVideoStream;
    if (videoStream.preparedFrames.empty()) { GIFTOOLS_LOGT("No prepared frames."); return {}; }
    
    auto lowerBoundIt = std::lower_bound(
        videoStream.preparedFrames.begin(),
        videoStream.preparedFrames.end(),
        sampleTime,
        [](giftools::UniqueManagedObj<giftools::FFmpegVideoFrame>& frame, double sampleTime) {
            return frame->estimatedSampleTimeSeconds() < sampleTime;
        });
    
    if (lowerBoundIt == videoStream.preparedFrames.end()) {
        GIFTOOLS_LOGT("No lower bound, returning the last prepared frame.");
        return ffmpegCloneFrame(*videoStream.preparedFrames.back());
    }
    
    auto upperBoundIt = lowerBoundIt + 1;
    if (upperBoundIt == videoStream.preparedFrames.end()) {
        GIFTOOLS_LOGT("No upper bound, returning the last prepared frame.");
        return ffmpegCloneFrame(*videoStream.preparedFrames.back());
    }
    
    const double lowerDiff = fabs(lowerBoundIt->get()->estimatedSampleTimeSeconds() - sampleTime);
    const double upperDiff = fabs(upperBoundIt->get()->estimatedSampleTimeSeconds() - sampleTime);
    GIFTOOLS_LOGT("Choosing between two frames, diffs: %f, %f.", lowerDiff, upperDiff);
    
    if (tolerance > 0.0 && std::min(lowerDiff, upperDiff) > tolerance) { return {}; }
    return lowerDiff <= upperDiff ? ffmpegCloneFrame(**lowerBoundIt) : ffmpegCloneFrame(**upperBoundIt);
}

giftools::UniqueManagedObj<giftools::FFmpegVideoFrame>
giftools::ffmpegVideoStreamPickBestFrame(const giftools::FFmpegVideoStream* ffmpegVideoStream, double sampleTime) {
    if (!ffmpegVideoStream) { GIFTOOLS_LOGE("Caught null video stream."); return {}; }
    
    auto& videoStream = (FFmpegVideoStreamImpl&)*ffmpegVideoStream;
    auto preparedFrame = ffmpegVideoStreamPickBestFrameFromPrepared(ffmpegVideoStream, sampleTime, videoStream.frameDurationSeconds * 0.5);
    if (preparedFrame) { GIFTOOLS_LOGT("Found good prepared frame."); return preparedFrame; }
    
    // clang-format off
    assert(videoStream.width > 0);
    assert(videoStream.height > 0);
    assert(videoStream.alignment == 1);
    const size_t imgBufferSize = av_image_get_buffer_size(videoStream.decodeFmt, videoStream.width, videoStream.height, videoStream.alignment);
    // clang-format on

    FFmpegStagingVideoFrame prevFrame = {};
    prevFrame.decodedFrame = av_frame_alloc();
    prevFrame.encodedFrame = av_frame_alloc();
    prevFrame.imageByteBuffer.resize(imgBufferSize);

    int ret = 0;
    ret = av_image_fill_arrays(prevFrame.encodedFrame->data,
                               prevFrame.encodedFrame->linesize,
                               prevFrame.imageByteBuffer.data(),
                               videoStream.decodeFmt,
                               videoStream.width,
                               videoStream.height,
                               videoStream.alignment);
    if (ret < 0) { GIFTOOLS_LOGE("av_image_fill_arrays failed."); return {}; }
    
    FFmpegStagingVideoFrame currFrame = {};
    currFrame.decodedFrame = av_frame_alloc();
    currFrame.encodedFrame = av_frame_alloc();
    currFrame.imageByteBuffer.resize(imgBufferSize);

    ret = av_image_fill_arrays(currFrame.encodedFrame->data,
                               currFrame.encodedFrame->linesize,
                               currFrame.imageByteBuffer.data(),
                               videoStream.decodeFmt,
                               videoStream.width,
                               videoStream.height,
                               videoStream.alignment);
    if (ret < 0) { GIFTOOLS_LOGE("av_image_fill_arrays failed."); return {}; }

    FlushBuffersGuard flushBuffersGuard{videoStream};
    FreeFramePacketGuard freePrevPacketGuard{prevFrame};
    FreeFramePacketGuard freeCurrPacketGuard{currFrame};
    FreeFrameGuard freePrevFrameGuard{currFrame};
    FreeFrameGuard freeCurrFrameGuard{prevFrame};

    bool endOfStream = false;
    bool frameAcquired = false;

    GIFTOOLS_LOGT("Starting main loop.");
    GIFTOOLS_LOGW("Reporting progress:");
    
    while (!endOfStream || frameAcquired) {
        frameAcquired = false;

        while (!frameAcquired) {
            bool keepSearchingPackets = true;
            while (keepSearchingPackets && !endOfStream) {
                GIFTOOLS_LOGT("av_read_frame");

                FreeFramePacketGuard::videoFrameFreePacket(currFrame);
                ret = av_read_frame(videoStream.fmtContext, &currFrame.packet);
                endOfStream = (ret == AVERROR_EOF);
                if (ret < 0 && ret != AVERROR_EOF) { GIFTOOLS_LOGE("av_read_frame failed."); return {}; }

                if (ret == 0 && currFrame.packet.stream_index == videoStream.primaryStreamIndex) {
                    GIFTOOLS_LOGT("Packet found.");
                    keepSearchingPackets = false;
                    break;
                }
            }

            // clang-format off
            
            // printf("avcodec_send_packet\n");
            // ret = avcodec_send_packet(videoStream.primaryCodecContext, &currFrame.packet);
            // if (ret < 0) { return {}; }
            //
            // printf("avcodec_receive_frame\n");
            // ret = avcodec_receive_frame(videoStream.primaryCodecContext, currFrame.decodedFrame);
            // if (ret < 0) { return {}; }
            // frameAcquired = true;
            
            GIFTOOLS_LOGT("avcodec_decode_video2");
            
            int didRetrievePicture = 0;
            ret = avcodec_decode_video2(videoStream.primaryCodecContext,
                                        currFrame.decodedFrame,
                                        &didRetrievePicture,
                                        &currFrame.packet);
            if (ret < 0) { GIFTOOLS_LOGE("avcodec_decode_video2 failed."); return {}; }
            frameAcquired = didRetrievePicture > 0;
            
            // clang-format on

            if (endOfStream) { break; }
        }

        // av_frame_get_best_effort_timestamp(currFrame.decodedFrame);
        const int64_t mostAccurateTimestamp = currFrame.decodedFrame->best_effort_timestamp;
        const double mostAccurateTime = mostAccurateTimestamp * videoStream.primaryStreamTimeBase;
        currFrame.timeTimeBaseEst = mostAccurateTimestamp;
        currFrame.timeSecondsEst = mostAccurateTime;
        
        const double diff = fabs(mostAccurateTime - sampleTime);

        GIFTOOLS_LOGT("mostAccurateTime = %f.", mostAccurateTime);
        GIFTOOLS_LOGT("sampleTime = %f.", sampleTime);
        GIFTOOLS_LOGT("diff = %f.", diff);
        GIFTOOLS_LOGT("frame = %f.", videoStream.frameDurationSeconds);

        if (diff < videoStream.frameDurationSeconds) {
            GIFTOOLS_LOGT("Current frame is good (%f, %f -> %f).", sampleTime, mostAccurateTime, diff);
            GIFTOOLS_LOGW("\t> Progress: 100.0%%");
    
            auto frame = ffmpegVideoFrameFromStaging(videoStream, currFrame);
            return frame;
        }

        if (mostAccurateTime < 0) {
            GIFTOOLS_LOGT("Last frame is the only option (%f, %f -> %f).", sampleTime, prevFrame.timeSecondsEst, fabs(sampleTime - prevFrame.timeSecondsEst));
            GIFTOOLS_LOGW("\t> Progress: 100.0%%");
            
            auto frame = ffmpegVideoFrameFromStaging(videoStream, prevFrame);
            return frame;
        }
        
        if (mostAccurateTime > sampleTime) {
            GIFTOOLS_LOGW("\t> Progress: 100.0%%");
            
            const double prevDiff = fabs(prevFrame.timeSecondsEst - sampleTime);
            const double currDiff = fabs(currFrame.timeSecondsEst - sampleTime);
            
            if (prevDiff < currDiff) {
                GIFTOOLS_LOGT("Previous frame is closer (%f, %f vs %f).", sampleTime, prevFrame.timeSecondsEst, currFrame.timeSecondsEst);
                
                auto frame = ffmpegVideoFrameFromStaging(videoStream, prevFrame);
                return frame;
            }
            
            GIFTOOLS_LOGT("Current frame is closer (%f, %f vs %f).", sampleTime, currFrame.timeSecondsEst, diff);

            auto frame = ffmpegVideoFrameFromStaging(videoStream, currFrame);
            return frame;
        }
        
        GIFTOOLS_LOGT("Trying the next frame.");
        GIFTOOLS_LOGW("\t> Progress: %.1f%%", std::min(99.9, 100.0 * (mostAccurateTime / sampleTime)));
        
        if (mostAccurateTime >= 0.0) {
            GIFTOOLS_LOGT("Swapping staing frames.");
            std::swap(prevFrame, currFrame);
        }
    }

    GIFTOOLS_LOGW("\t> Progress: 100.0%%");
    return {};
}

size_t
giftools::ffmpegVideoStreamPrepareAllFrames(const FFmpegVideoStream* ffmpegVideoStream) {
    if (!ffmpegVideoStream) { GIFTOOLS_LOGE("Caught null video stream."); return 0; }

    auto& videoStream = (FFmpegVideoStreamImpl&)*ffmpegVideoStream;

    // clang-format off
    assert(videoStream.width > 0);
    assert(videoStream.height > 0);
    assert(videoStream.alignment == 1);
    const size_t imgBufferSize = av_image_get_buffer_size(videoStream.decodeFmt, videoStream.width, videoStream.height, videoStream.alignment);
    // clang-format on
    
    FFmpegStagingVideoFrame currFrame = {};
    currFrame.decodedFrame = av_frame_alloc();
    currFrame.encodedFrame = av_frame_alloc();
    currFrame.imageByteBuffer.resize(imgBufferSize);

    int ret;
    ret = av_image_fill_arrays(currFrame.encodedFrame->data,
                               currFrame.encodedFrame->linesize,
                               currFrame.imageByteBuffer.data(),
                               videoStream.decodeFmt,
                               videoStream.width,
                               videoStream.height,
                               videoStream.alignment);
    if (ret < 0) { GIFTOOLS_LOGE("av_image_fill_arrays failed."); return 0; }

    FlushBuffersGuard flushBuffersGuard{videoStream};
    FreeFramePacketGuard freeCurrPacketGuard{currFrame};
    FreeFrameGuard freePrevFrameGuard{currFrame};

    bool endOfStream = false;
    bool frameAcquired = false;

    GIFTOOLS_LOGT("Starting main loop.");
    GIFTOOLS_LOGW("Reporting progress:");
    videoStream.clearPreparedFrames(false);
    
    while (!endOfStream || frameAcquired) {
        frameAcquired = false;

        while (!frameAcquired) {
            bool keepSearchingPackets = true;
            while (keepSearchingPackets && !endOfStream) {
                GIFTOOLS_LOGT("av_read_frame");

                FreeFramePacketGuard::videoFrameFreePacket(currFrame);
                ret = av_read_frame(videoStream.fmtContext, &currFrame.packet);
                endOfStream = (ret == AVERROR_EOF);
                if (ret < 0 && ret != AVERROR_EOF) { GIFTOOLS_LOGE("av_read_frame failed."); return 0; }

                if (ret == 0 && currFrame.packet.stream_index == videoStream.primaryStreamIndex) {
                    GIFTOOLS_LOGT("Packet found.");
                    keepSearchingPackets = false;
                    break;
                }
            }

            // clang-format off
            
            // printf("avcodec_send_packet\n");
            // ret = avcodec_send_packet(videoStream.primaryCodecContext, &currFrame.packet);
            // if (ret < 0) { return {}; }
            //
            // printf("avcodec_receive_frame\n");
            // ret = avcodec_receive_frame(videoStream.primaryCodecContext, currFrame.decodedFrame);
            // if (ret < 0) { return {}; }
            // frameAcquired = true;
            
            GIFTOOLS_LOGT("avcodec_decode_video2");
            
            int didRetrievePicture = 0;
            ret = avcodec_decode_video2(videoStream.primaryCodecContext,
                                        currFrame.decodedFrame,
                                        &didRetrievePicture,
                                        &currFrame.packet);
            if (ret < 0) { GIFTOOLS_LOGE("avcodec_decode_video2 failed."); return 0; }
            frameAcquired = didRetrievePicture > 0;
            
            // clang-format on

            if (endOfStream) { break; }
        }
        
        // av_frame_get_best_effort_timestamp(currFrame.decodedFrame);
        const int64_t mostAccurateTimestamp = currFrame.decodedFrame->best_effort_timestamp;
        const double mostAccurateTime = mostAccurateTimestamp * videoStream.primaryStreamTimeBase;
        currFrame.timeTimeBaseEst = mostAccurateTimestamp;
        currFrame.timeSecondsEst = mostAccurateTime;

        GIFTOOLS_LOGT("mostAccurateTime = %f", mostAccurateTime);
        GIFTOOLS_LOGT("frame = %f", videoStream.frameDurationSeconds);
        
        if (mostAccurateTime < 0) { GIFTOOLS_LOGE("mostAccurateTime is negative."); break; }

        videoStream.preparedFrames.emplace_back(ffmpegVideoFrameFromStaging(videoStream, currFrame));
        GIFTOOLS_LOGW("\t> Progress: %.1f%%", 100.0 * (mostAccurateTime / videoStream.durationSeconds));
    }
    
    GIFTOOLS_LOGT("Sorting prepared frames: %zu", videoStream.preparedFrames.size());
    videoStream.sortPreparedFrames();

    GIFTOOLS_LOGW("\t> Progress: 100.0%%");
    return videoStream.preparedFrames.size();
}

size_t
giftools::ffmpegVideoStreamPrepareFrames(const FFmpegVideoStream* ffmpegVideoStream,
                                         double framesPerSecond,
                                         double offsetSeconds,
                                         double durationSeconds) {
    if (!ffmpegVideoStream) { GIFTOOLS_LOGE("Caught null video stream."); return 0; }
    
    auto& videoStream = (FFmpegVideoStreamImpl&)*ffmpegVideoStream;
    auto& progressToken = *getMutableProgressToken();
    auto& cancellationToken = *getMutableCancellationToken();
    
    double maxFramesPerSecond = double(videoStream.frameCount_) / videoStream.durationSeconds;
    double desiredFrameTimeSeconds = 1.0 / framesPerSecond;
    
    if (offsetSeconds >= videoStream.durationSeconds) { GIFTOOLS_LOGE("Caught invalid offset."); return 0; }
    if (desiredFrameTimeSeconds > videoStream.durationSeconds) { GIFTOOLS_LOGE("Caught invalid framerate."); return 0; }
    
    durationSeconds = std::min(durationSeconds, videoStream.durationSeconds - offsetSeconds);
    
    double endTimeSeconds = durationSeconds + offsetSeconds;
    
    if (framesPerSecond >= maxFramesPerSecond) { return ffmpegVideoStreamPrepareAllFrames(ffmpegVideoStream); }

    if (cancellationToken.isCancelled()) { return 0; }

    // clang-format off
    assert(videoStream.width > 0);
    assert(videoStream.height > 0);
    assert(videoStream.alignment == 1);
    const size_t imgBufferSize = av_image_get_buffer_size(videoStream.decodeFmt, videoStream.width, videoStream.height, videoStream.alignment);
    // clang-format on

    FFmpegStagingVideoFrame prevFrame = {};
    prevFrame.decodedFrame = av_frame_alloc();
    prevFrame.encodedFrame = av_frame_alloc();
    prevFrame.imageByteBuffer.resize(imgBufferSize);

    int ret = 0;
    ret = av_image_fill_arrays(prevFrame.encodedFrame->data,
                               prevFrame.encodedFrame->linesize,
                               prevFrame.imageByteBuffer.data(),
                               videoStream.decodeFmt,
                               videoStream.width,
                               videoStream.height,
                               videoStream.alignment);
    if (ret < 0) { GIFTOOLS_LOGE("av_image_fill_arrays failed."); return 0; }
    
    FFmpegStagingVideoFrame currFrame = {};
    currFrame.decodedFrame = av_frame_alloc();
    currFrame.encodedFrame = av_frame_alloc();
    currFrame.imageByteBuffer.resize(imgBufferSize);

    ret = av_image_fill_arrays(currFrame.encodedFrame->data,
                               currFrame.encodedFrame->linesize,
                               currFrame.imageByteBuffer.data(),
                               videoStream.decodeFmt,
                               videoStream.width,
                               videoStream.height,
                               videoStream.alignment);
    if (ret < 0) { GIFTOOLS_LOGE("av_image_fill_arrays failed."); return 0; }

    FlushBuffersGuard flushBuffersGuard{videoStream};
    FreeFramePacketGuard freePrevPacketGuard{prevFrame};
    FreeFramePacketGuard freeCurrPacketGuard{currFrame};
    FreeFrameGuard freePrevFrameGuard{currFrame};
    FreeFrameGuard freeCurrFrameGuard{prevFrame};
    
    if (cancellationToken.isCancelled()) { return 0; }

    bool endOfStream = false;
    bool frameAcquired = false;
    double sampleTime = 0.0f;
    
    GIFTOOLS_LOGT("Starting main loop.");
    GIFTOOLS_LOGW("Reporting progress:");
    videoStream.clearPreparedFrames(false);
    
    while (!endOfStream || frameAcquired) {
        if (cancellationToken.isCancelled()) { videoStream.clearPreparedFrames(); return 0; }
        
        frameAcquired = false;

        while (!frameAcquired) {
            if (cancellationToken.isCancelled()) { videoStream.clearPreparedFrames(); return 0; }
            
            bool keepSearchingPackets = true;
            while (keepSearchingPackets && !endOfStream) {
                GIFTOOLS_LOGT("av_read_frame");

                FreeFramePacketGuard::videoFrameFreePacket(currFrame);
                
                if (cancellationToken.isCancelled()) { videoStream.clearPreparedFrames(); return 0; }
                
                ret = av_read_frame(videoStream.fmtContext, &currFrame.packet);
                endOfStream = (ret == AVERROR_EOF);
                if (ret < 0 && ret != AVERROR_EOF) { GIFTOOLS_LOGE("av_read_frame failed."); return 0; }

                if (ret == 0 && currFrame.packet.stream_index == videoStream.primaryStreamIndex) {
                    GIFTOOLS_LOGT("Packet found.");
                    keepSearchingPackets = false;
                    break;
                }
            }

            // clang-format off
            
            // printf("avcodec_send_packet\n");
            // ret = avcodec_send_packet(videoStream.primaryCodecContext, &currFrame.packet);
            // if (ret < 0) { return {}; }
            //
            // printf("avcodec_receive_frame\n");
            // ret = avcodec_receive_frame(videoStream.primaryCodecContext, currFrame.decodedFrame);
            // if (ret < 0) { return {}; }
            // frameAcquired = true;
            
            if (cancellationToken.isCancelled()) { videoStream.clearPreparedFrames(); return 0; }
            
            GIFTOOLS_LOGT("avcodec_decode_video2");
            
            int didRetrievePicture = 0;
            ret = avcodec_decode_video2(videoStream.primaryCodecContext,
                                        currFrame.decodedFrame,
                                        &didRetrievePicture,
                                        &currFrame.packet);
            if (ret < 0) { GIFTOOLS_LOGE("avcodec_decode_video2 failed."); return 0; }
            frameAcquired = didRetrievePicture > 0;
            
            // clang-format on

            if (endOfStream) { break; }
        }

        // av_frame_get_best_effort_timestamp(currFrame.decodedFrame);
        const int64_t mostAccurateTimestamp = currFrame.decodedFrame->best_effort_timestamp;
        const double mostAccurateTime = mostAccurateTimestamp * videoStream.primaryStreamTimeBase;
        currFrame.timeTimeBaseEst = mostAccurateTimestamp;
        currFrame.timeSecondsEst = mostAccurateTime;
        
        if (mostAccurateTime > endTimeSeconds) {
            GIFTOOLS_LOGW("\t> Progress: %.1f%%", std::min(99.9, 100.0 * (sampleTime / endTimeSeconds)));
            break;
        }
        
        if (mostAccurateTime > 0 && mostAccurateTime < offsetSeconds) {
        
            std::swap(prevFrame, currFrame);
            sampleTime += desiredFrameTimeSeconds;
            
            GIFTOOLS_LOGW("\t> Progress: %.1f%%", std::min(99.9, 100.0 * (sampleTime / endTimeSeconds)));
            continue;
        }
        
        const double diff = fabs(mostAccurateTime - sampleTime);

        GIFTOOLS_LOGT("mostAccurateTime = %f", mostAccurateTime);
        GIFTOOLS_LOGT("sampleTime = %f", sampleTime);
        GIFTOOLS_LOGT("diff = %f", diff);
        GIFTOOLS_LOGT("frame = %f", videoStream.frameDurationSeconds);
        
        if (diff < videoStream.frameDurationSeconds) {
            if (cancellationToken.isCancelled()) { videoStream.clearPreparedFrames(); return 0; }
            
            GIFTOOLS_LOGT("Current frame is good (%f, %f -> %f)", sampleTime, mostAccurateTime, diff);
                
            auto frame = ffmpegVideoFrameFromStaging(videoStream, currFrame);
            if (frame) { videoStream.preparedFrames.emplace_back(std::move(frame)); }
            sampleTime += desiredFrameTimeSeconds;
            
            GIFTOOLS_LOGW("\t> Progress: %.1f%%", std::min(99.9, 100.0 * (sampleTime / endTimeSeconds)));
            continue;
        }

        if (mostAccurateTime < 0) {
            if (cancellationToken.isCancelled()) { videoStream.clearPreparedFrames(); return 0; }
            
            GIFTOOLS_LOGT("Last frame is the only option (%f, %f -> %f)", sampleTime, prevFrame.timeSecondsEst, fabs(sampleTime - prevFrame.timeSecondsEst));
            
            auto frame = ffmpegVideoFrameFromStaging(videoStream, prevFrame);
            if (frame) { videoStream.preparedFrames.emplace_back(std::move(frame)); }
            
            std::swap(prevFrame, currFrame);
            sampleTime += desiredFrameTimeSeconds;
            
            GIFTOOLS_LOGW("\t> Progress: %.1f%%", std::min(99.9, 100.0 * (sampleTime / endTimeSeconds)));
            continue;
        }
        
        if (mostAccurateTime > sampleTime) {
            if (cancellationToken.isCancelled()) { videoStream.clearPreparedFrames(); return 0; }
                
            const double prevDiff = fabs(prevFrame.timeSecondsEst - sampleTime);
            const double currDiff = fabs(currFrame.timeSecondsEst - sampleTime);
            
            if (prevDiff < currDiff) {
            
                GIFTOOLS_LOGT("Previous frame is closer (%f, %f vs %f)\n", sampleTime, prevFrame.timeSecondsEst, currFrame.timeSecondsEst);

                auto frame = ffmpegVideoFrameFromStaging(videoStream, prevFrame);
                if (frame) { videoStream.preparedFrames.emplace_back(std::move(frame)); }
                
                std::swap(prevFrame, currFrame);
                sampleTime += desiredFrameTimeSeconds;
                
                GIFTOOLS_LOGW("\t> Progress: %.1f%%", std::min(99.9, 100.0 * (sampleTime / endTimeSeconds)));
                continue;
            }

            GIFTOOLS_LOGT("Current frame is closer (%f, %f vs %f)", sampleTime, currFrame.timeSecondsEst, diff);

            auto frame = ffmpegVideoFrameFromStaging(videoStream, currFrame);
            if (frame) { videoStream.preparedFrames.emplace_back(std::move(frame)); }
            
            std::swap(prevFrame, currFrame);
            sampleTime += desiredFrameTimeSeconds;
        
            GIFTOOLS_LOGW("\t> Progress: %.1f%%", std::min(99.9, 100.0 * (sampleTime / endTimeSeconds)));
            continue;
        }

        GIFTOOLS_LOGT("Trying the next frame.");
        
        if (mostAccurateTime >= 0) {
            GIFTOOLS_LOGT("Swapping staging frames.");
            std::swap(prevFrame, currFrame);
        }
    }

    GIFTOOLS_LOGT("Sorting prepared frames: %zu", videoStream.preparedFrames.size());
    videoStream.sortPreparedFrames();

    GIFTOOLS_LOGW("\t> Progress: 100.0%%");
    return videoStream.preparedFrames.size();
}

giftools::UniqueManagedObj<giftools::FFmpegVideoFrame> giftools::ffmpegVideoStreamPickBestPreparedFrame(const FFmpegVideoStream* ffmpegVideoStream, double sampleTime) {
    auto preparedFrame = ffmpegVideoStreamPickBestFrameFromPrepared(ffmpegVideoStream, sampleTime);
    if (!preparedFrame) { return {}; }
    return preparedFrame;
}

void giftools::ffmpegVideoStreamClearPreparedFrames(const giftools::FFmpegVideoStream* ffmpegVideoStream) {
    if (!ffmpegVideoStream) { GIFTOOLS_LOGE("Caught null video stream."); return; }
    auto& videoStream = (FFmpegVideoStreamImpl&)*ffmpegVideoStream;
    videoStream.clearPreparedFrames();
}

#pragma clang diagnostic pop
#endif
