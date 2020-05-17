#include <GifTools.h>

#include <cstdint>
#include <cstdio>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#ifdef GIFTOOLS_EMSCRIPTEN
//#warning "GifTools: emscripten"
//#include <emscripten/emscripten.h>
//#include <emscripten/bind.h>
//#include <emscripten/val.h>
// using namespace emscripten;
//
//
// extern "C" {
// EMSCRIPTEN_KEEPALIVE
// float lerp(float a, float b, float t) ;
// EMSCRIPTEN_KEEPALIVE
// int imageReadFromFile(const char* imgFilePath);
//}
//
// float lerp(float a, float b, float t) {
//    return (1 - t) * a + t * b;
//}
//
// int imageReadFromFile(const char* imgFilePath) {
//    printf("%s", imgFilePath);
//    return 3;
//}
//
// EMSCRIPTEN_BINDINGS(GifTools) {
//    function("lerp", &lerp);
//}
//
// class Lerper {
// public:
//    static float linearStep(float a, float b, float t) {
//        return lerp(a, b, t);
//    }
//};
//
// EMSCRIPTEN_BINDINGS(GifToolsLerper) {
//    class_<Lerper>("Lerper")
//        .constructor<>()
//        .class_function("linearStep", &Lerper::linearStep)
//        ;
//}

#else

#ifdef GIFTOOLS_USE_FFMPEG
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

enum FileType {
    FileTypeNone,
    FileTypeBinary,
    FileTypeText,
};
struct File {
public:
    std::string path = {};
    std::vector<uint8_t> buffer = {};
    FileType type = FileTypeNone;
};

std::optional<File> fileReadBinary(const std::string& imgFilePath) {
    if (FILE* imgFileHandle = fopen(imgFilePath.c_str(), "rb")) {
        File file = {};
        file.path = imgFilePath;
        file.type = FileTypeBinary;

        fseek(imgFileHandle, 0, SEEK_END);
        size_t imgFileSize = ftell(imgFileHandle);
        fseek(imgFileHandle, 0, SEEK_SET);
        file.buffer.resize(imgFileSize);
        fread(file.buffer.data(), imgFileSize, 1, imgFileHandle);
        fclose(imgFileHandle);
        return file;
    }

    return {};
}

void writeRGB24ToPNG(const uint8_t* data, size_t width, size_t height, size_t frame, double duration, const std::string_view sv) {
    char buff[256] = {};
    
    size_t mm = size_t(floor(duration / 60));
    size_t ss = size_t(floor(duration));
    double ms = double(duration - size_t(duration));
    
    sprintf(buff, "%s_%zu-%zu-%3.3f_%09zu-%09zu.png", sv.data(), mm, ss, ms * 1000, frame, size_t(duration * 1000.0));
    printf("PNG: %s\n", buff);
    stbi_write_png(buff, width, height, 3, data, width * 3);
}

struct VideoStream {
    giftools::UniqueManagedObj<giftools::Buffer> streamContents;
    giftools::UniqueManagedObj<giftools::FFmpegInputStream> stream;
    
    AVFormatContext* fmtContext = nullptr;
    std::vector<uint8_t> probeBuffer = {};
    AVProbeData probeData = {};
    AVInputFormat* inputFmt = nullptr;
    AVCodec* primaryCodec = nullptr;
    AVCodecContext* primaryCodecContext = nullptr;
    AVStream* primaryStream = nullptr;
    int primaryStreamIndex = -1;
    SwsContext* swsContext = nullptr;
    int64_t frameCount = 0;
    double durationSeconds = 0.0;
    int64_t durationTimeBase = 0;
    int64_t frameDurationTimeBase = 0;
    double frameDurationSeconds = 0.0;
    double primaryStreamTimeBase = 0.0;
    int width = -1;
    int height = -1;
    int alignment = 1;
    AVPixelFormat decodeFmt = AV_PIX_FMT_RGB24;
};

struct VideoFrame {
    double timeSecondsEst = 0.0;
    int64_t timeTimeBaseEst = 0;
    int width = -1;
    int height = -1;
    int alignment = 1;
    AVPixelFormat decodeFmt = AV_PIX_FMT_RGB24;
    std::vector<uint8_t> imageByteBuffer = {};
    AVFrame* encodedFrame = nullptr;
    AVFrame* decodedFrame = nullptr;
    AVPacket packet = {};
};

std::optional<VideoStream> videoStreamOpen(const std::string& filePath) {
    int ret = 0;

    auto optFile = fileReadBinary(filePath);
    if (!optFile || optFile->buffer.empty()) { return {}; }

    VideoStream videoStream = {};
    videoStream.streamContents = giftools::bufferFromVector(std::move(optFile.value().buffer));
    videoStream.stream = giftools::ffmpegInputStreamLoadFromMemory(videoStream.streamContents.get());

    // TODO(vserhiienko): deprecated.
    av_register_all();
    // av_log_set_level(AV_LOG_TRACE);
    av_log_set_level(AV_LOG_ERROR);

    videoStream.fmtContext = avformat_alloc_context();
    videoStream.fmtContext->pb = avio_alloc_context(videoStream.streamContents->mutableData(),
                                                    videoStream.streamContents->size(),
                                                    0,
                                                    videoStream.stream.get(),
                                                    videoStream.stream->ffmpegInputStreamReadFnPtr(),
                                                    nullptr,
                                                    videoStream.stream->ffmpegInputStreamSeekFnPtr());
    if (!videoStream.fmtContext->pb) { return {}; }

    videoStream.probeData.filename = "stream";
    videoStream.probeData.buf_size = std::min<int>(videoStream.streamContents->size(), 4096);
    videoStream.probeBuffer.resize(videoStream.probeData.buf_size);
    videoStream.probeData.buf = videoStream.probeBuffer.data();
    memcpy(videoStream.probeData.buf, videoStream.streamContents->data(), 4096);

    videoStream.inputFmt = av_probe_input_format(&videoStream.probeData, 1);
    if (!videoStream.inputFmt) { videoStream.inputFmt = av_probe_input_format(&videoStream.probeData, 0); }

    videoStream.probeData.buf = nullptr;
    std::vector<uint8_t>().swap(videoStream.probeBuffer);

    if (!videoStream.inputFmt) { return {}; }
    videoStream.inputFmt->flags |= AVFMT_NOFILE;

    // clang-format off
    ret = avformat_open_input(&videoStream.fmtContext, videoStream.probeData.filename, videoStream.inputFmt, nullptr);
    if (ret < 0) { return {}; }
    ret = avformat_find_stream_info(videoStream.fmtContext, nullptr);
    if (ret < 0) { return {}; }

    videoStream.primaryStreamIndex = av_find_best_stream(videoStream.fmtContext, AVMEDIA_TYPE_VIDEO, -1, -1, &videoStream.primaryCodec, 0);
    if (videoStream.primaryStreamIndex < 0 || videoStream.primaryStreamIndex >= videoStream.fmtContext->nb_streams) { return {}; }
    // clang-format on

    videoStream.primaryStream = videoStream.fmtContext->streams[videoStream.primaryStreamIndex];
    videoStream.primaryStreamTimeBase = av_q2d(videoStream.primaryStream->time_base);
    videoStream.frameCount = videoStream.primaryStream->nb_frames;
    videoStream.durationTimeBase = videoStream.primaryStream->duration;
    videoStream.frameDurationTimeBase = videoStream.durationTimeBase / videoStream.frameCount;
    videoStream.durationSeconds = double(videoStream.durationTimeBase) * videoStream.primaryStreamTimeBase;
    videoStream.frameDurationSeconds = double(videoStream.frameDurationTimeBase) * videoStream.primaryStreamTimeBase;

    // clang-format off
    // TODO(vserhiienko): deprecated.
    videoStream.primaryCodecContext = videoStream.primaryStream->codec;
    if (int ret = avcodec_open2(videoStream.primaryCodecContext, videoStream.primaryCodec, nullptr); ret < 0) { return {}; }
    // clang-format on

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
    return videoStream;
}

void videoFrameFreePacket(VideoFrame& stagingFrame) {
    if (stagingFrame.packet.buf) { av_buffer_unref(&stagingFrame.packet.buf); }
    stagingFrame.packet.data = nullptr;
    stagingFrame.packet.size = 0;
    av_packet_free_side_data(&stagingFrame.packet);
}

// TODO(vserhiienko): test for memory leaks.
void videoStreamClose(VideoStream& videoStream) {
    sws_freeContext(videoStream.swsContext);
    avcodec_close(videoStream.primaryCodecContext);
    avformat_close_input(&videoStream.fmtContext);
    videoStream = VideoStream{};
}

struct FlushBuffersGuard {
    VideoStream& videoStream;

    void flushBuffers() {
        avcodec_flush_buffers(videoStream.primaryCodecContext);
        av_seek_frame(videoStream.fmtContext, videoStream.primaryStreamIndex, 0, 0);
    }

    FlushBuffersGuard(VideoStream& videoStream) : videoStream(videoStream) { flushBuffers(); }
    ~FlushBuffersGuard() { flushBuffers(); }
};

struct FreeFramePacketGuard {
    VideoFrame& videoFrame;
    void freePacket() { videoFrameFreePacket(videoFrame); }
    FreeFramePacketGuard(VideoFrame& videoFrame) : videoFrame(videoFrame) { freePacket(); }
    ~FreeFramePacketGuard() { freePacket(); }
};

struct FreeFrameGuard {
    VideoFrame& videoFrame;
    void freeFrame() {
        av_frame_free(&videoFrame.decodedFrame);
        av_frame_free(&videoFrame.encodedFrame);
    }
    FreeFrameGuard(VideoFrame& videoFrame) : videoFrame(videoFrame) {}
    ~FreeFrameGuard() { freeFrame(); }
};

std::vector<uint8_t> videoStreamBrutePickBestFrame(VideoStream& videoStream, double sampleTime) {
    int ret = 0;

    // clang-format off
    assert(videoStream.width > 0);
    assert(videoStream.height > 0);
    assert(videoStream.alignment == 1);
    const size_t imgBufferSize = av_image_get_buffer_size(videoStream.decodeFmt, videoStream.width, videoStream.height, videoStream.alignment);
    // clang-format on

    VideoFrame prevFrame = {};
    prevFrame.decodedFrame = av_frame_alloc();
    prevFrame.encodedFrame = av_frame_alloc();
    prevFrame.imageByteBuffer.resize(imgBufferSize);

    ret = av_image_fill_arrays(prevFrame.encodedFrame->data,
                               prevFrame.encodedFrame->linesize,
                               prevFrame.imageByteBuffer.data(),
                               videoStream.decodeFmt,
                               videoStream.width,
                               videoStream.height,
                               videoStream.alignment);
    
    VideoFrame currFrame = {};
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
    if (ret < 0) { return {}; }

    FlushBuffersGuard flushBuffersGuard{videoStream};
    FreeFramePacketGuard freePrevPacketGuard{prevFrame};
    FreeFramePacketGuard freeCurrPacketGuard{currFrame};
    FreeFrameGuard freePrevFrameGuard{currFrame};
    FreeFrameGuard freeCurrFrameGuard{prevFrame};

    bool endOfStream = false;
    bool frameAcquired = false;

    printf("while\n");
    while (!endOfStream || frameAcquired) {
        frameAcquired = false;

        while (!frameAcquired) {
            bool keepSearchingPackets = true;
            while (keepSearchingPackets && !endOfStream) {
                printf("av_read_frame\n");

                videoFrameFreePacket(currFrame);
                ret = av_read_frame(videoStream.fmtContext, &currFrame.packet);
                endOfStream = (ret == AVERROR_EOF);
                if (ret < 0 && ret != AVERROR_EOF) { return {}; }

                if (ret == 0 && currFrame.packet.stream_index == videoStream.primaryStreamIndex) {
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
            
            printf("avcodec_decode_video2\n");
            int didRetrievePicture = 0;
            ret = avcodec_decode_video2(videoStream.primaryCodecContext,
                                        currFrame.decodedFrame,
                                        &didRetrievePicture,
                                        &currFrame.packet);
            if (ret < 0) { return {}; }
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

        printf("mostAccurateTime = %f\n", mostAccurateTime);
        printf("sampleTime = %f\n", sampleTime);
        printf("diff = %f\n", diff);
        printf("frame = %f\n", videoStream.frameDurationSeconds);
        
        if (diff < videoStream.frameDurationSeconds) {
            printf("curr frame is good (%f, %f -> %f)\n", sampleTime, mostAccurateTime, diff);
            printf("sws_scale\n");
            ret = sws_scale(videoStream.swsContext,
                            currFrame.decodedFrame->data,
                            currFrame.decodedFrame->linesize,
                            0,
                            currFrame.decodedFrame->height,
                            currFrame.encodedFrame->data,
                            currFrame.encodedFrame->linesize);
            if (ret < 0) { return {}; }
            return currFrame.imageByteBuffer;
        }

        if (mostAccurateTime < 0) {
            printf("last frame is the only option (%f, %f -> %f)\n", sampleTime, prevFrame.timeSecondsEst, fabs(sampleTime - prevFrame.timeSecondsEst));
            printf("sws_scale\n");
            ret = sws_scale(videoStream.swsContext,
                            prevFrame.decodedFrame->data,
                            prevFrame.decodedFrame->linesize,
                            0,
                            prevFrame.decodedFrame->height,
                            prevFrame.encodedFrame->data,
                            prevFrame.encodedFrame->linesize);
            if (ret < 0) { return {}; }
            return prevFrame.imageByteBuffer;
        }
        
        if (mostAccurateTime > sampleTime) {
            const double prevDiff = fabs(prevFrame.timeSecondsEst - sampleTime);
            const double currDiff = fabs(currFrame.timeSecondsEst - sampleTime);
            if (prevDiff < currDiff) {
                printf("previous frame is closer (%f, %f vs %f)\n", sampleTime, prevFrame.timeSecondsEst, currFrame.timeSecondsEst);
                printf("sws_scale\n");
                ret = sws_scale(videoStream.swsContext,
                                prevFrame.decodedFrame->data,
                                prevFrame.decodedFrame->linesize,
                                0,
                                prevFrame.decodedFrame->height,
                                prevFrame.encodedFrame->data,
                                prevFrame.encodedFrame->linesize);
                if (ret < 0) { return {}; }
                return prevFrame.imageByteBuffer;
            }
            
            printf("curr frame is closer (%f, %f vs %f)\n", sampleTime, currFrame.timeSecondsEst, diff);
            printf("sws_scale\n");
            ret = sws_scale(videoStream.swsContext,
                            currFrame.decodedFrame->data,
                            currFrame.decodedFrame->linesize,
                            0,
                            currFrame.decodedFrame->height,
                            currFrame.encodedFrame->data,
                            currFrame.encodedFrame->linesize);
            if (ret < 0) { return {}; }
            return currFrame.imageByteBuffer;
        }
        
        printf("trying next frame\n");
        if (mostAccurateTime >= 0) {
            std::swap(prevFrame, currFrame);
        }
    }

    return {};
}
#endif

void testGifWriter() {
    using namespace giftools;

    UniqueManagedObj<Buffer> bufferObjs[4];
    bufferObjs[0] = fileBinaryRead("/Users/vserhiienko/Downloads/Photos/IMG_20191217_083058.jpg");
    bufferObjs[1] = fileBinaryRead("/Users/vserhiienko/Downloads/Photos/IMG_20191217_083059.jpg");
    bufferObjs[2] = fileBinaryRead("/Users/vserhiienko/Downloads/Photos/IMG_20191217_083101.jpg");
    bufferObjs[3] = fileBinaryRead("/Users/vserhiienko/Downloads/Photos/IMG_20191217_083059.jpg");

    UniqueManagedObj<Image> imageObjs[4];
    imageObjs[0] = imageLoadFromFileMemory(bufferObjs[0].get());
    imageObjs[1] = imageLoadFromFileMemory(bufferObjs[1].get());
    imageObjs[2] = imageLoadFromFileMemory(bufferObjs[2].get());
    imageObjs[3] = imageLoadFromFileMemory(bufferObjs[3].get());

    const size_t delay = 100;
    const size_t width = 1200;
    const size_t height = 900;

    imageObjs[0] = imageResizeOrClone(imageObjs[0].get(), width, height);
    imageObjs[1] = imageResizeOrClone(imageObjs[1].get(), width, height);
    imageObjs[2] = imageResizeOrClone(imageObjs[2].get(), width, height);
    imageObjs[3] = imageResizeOrClone(imageObjs[3].get(), width, height);

    UniqueManagedObj<GifBuilder> gifBuilderObj = gifBuilderInitialize(width, height, delay);
    gifBuilderAddImage(gifBuilderObj.get(), imageObjs[0].get(), delay);
    gifBuilderAddImage(gifBuilderObj.get(), imageObjs[1].get(), delay);
    gifBuilderAddImage(gifBuilderObj.get(), imageObjs[2].get(), delay);
    gifBuilderAddImage(gifBuilderObj.get(), imageObjs[3].get(), delay);

    UniqueManagedObj<Buffer> gifBufferObj = gifBuilderFinalize(gifBuilderObj.get());
    fileBinaryWrite("TestGif.gif", gifBufferObj.get());
}

void linkEmsdk() {
#ifdef GIFTOOLS_EMSCRIPTEN
    EM_ASM(console.log("GifTools!"););
#endif
}

int main(int argc, char** argv) {
    printf("Yup.");

    testGifWriter();

#ifdef GIFTOOLS_USE_FFMPEG

    const char* testFileMP4 = "/Users/vserhiienko/Downloads/2020-02-23 18.53.40.mp4";
    auto fileBuffer = giftools::fileBinaryRead(testFileMP4);
    auto fileStream = giftools::ffmpegInputStreamLoadFromMemory(fileBuffer.get());
    auto videoStream = giftools::ffmpegVideoStreamOpen(fileStream.get());
    
    size_t frameCounter = 0;
    
    for (double t = 0; t <= videoStream->estimatedFrameDurationSeconds(); t += videoStream->estimatedFrameDurationSeconds()) {
        auto sampledVideoFrame = giftools::ffmpegVideoStreamPickBestFrame(videoStream.get(), t);
        if (sampledVideoFrame) {
            char buff[256] = {};
            
            size_t mm = size_t(floor(t / 60));
            size_t ss = size_t(floor(t));
            double ms = double(t - size_t(t));
            
            sprintf(buff, "test_dump_frame_%zu-%zu-%3.3f_%09zu-%09zu.png", mm, ss, ms * 1000, frameCounter++, size_t(t * 1000.0));
            printf("PNG: %s\n", buff);
        
            auto imageBuffer = giftools::imageExportToPngFileMemory(sampledVideoFrame->image());
            giftools::fileBinaryWrite("dump_video_frame.png", imageBuffer->data(), imageBuffer->size());
        } else {
            printf("videoStreamBruteDumpFrameAt failed.");
        }
    }
    
    giftools::ffmpegVideoStreamClose(videoStream.get());
    return 0;
    
//
//    auto optVideoStream = videoStreamOpen(testMp4File);
//    if (!optVideoStream) { return -1; }
//
//    VideoStream& videoStream = *optVideoStream;

#if 1
//    size_t frameCounter = 0;
//    for (double t = 0; t <= videoStream.durationSeconds; t += videoStream.frameDurationSeconds) {
//        auto imageBuffer = videoStreamBrutePickBestFrame(videoStream, t);
//        if (!imageBuffer.empty()) { writeRGB24ToPNG(imageBuffer.data(), videoStream.width, videoStream.height, frameCounter++, t, "dump_video"); }
//        else { printf("videoStreamBruteDumpFrameAt failed."); }
//    }

    //    videoStreamBruteDumpFrameAt(videoStream, 0);
    //    videoStreamBruteDumpFrameAt(videoStream, 2207);
    //    videoStreamBruteDumpFrameAt(videoStream, 3428);
    //    videoStreamBruteDumpFrameAt(videoStream, 6740);

    //    int64_t t0 = 0;
    //    int64_t t1 = 100;
    //    for (int64_t t = t0; t < t1; t += 5) {
    //        videoStreamDumpFrameAt(videoStream, t);
    //    }

//    videoStreamClose(videoStream);
//    return 0;

#else

    //    InputStream stream = {0, fileReadFileToBuffer(file)};
    //
    //    av_register_all();
    //    av_log_set_level(AV_LOG_TRACE);
    //
    //    AVFormatContext* fmtContext;
    //    fmtContext = avformat_alloc_context();
    //    // fmtContext->pb = avio_alloc_context(stream.buffer.data(), stream.buffer.size(), 0, &stream, Read, NULL,
    //    Seek); if (!fmtContext->pb) { return -1; }
    //
    //    AVProbeData probeData = {};
    //    probeData.buf_size = std::min<int>(stream.buffer.size(), 4096);
    //    probeData.filename = "stream";
    //    probeData.buf = (unsigned char*)malloc(probeData.buf_size);
    //    memcpy(probeData.buf, stream.buffer.data(), 4096);
    //
    //    AVInputFormat* inputFmt = av_probe_input_format(&probeData, 1);
    //    if (!inputFmt) { inputFmt = av_probe_input_format(&probeData, 0); }
    //
    //    free(probeData.buf);
    //    probeData.buf = NULL;
    //
    //    if (!inputFmt) { return -1; }
    //
    //    inputFmt->flags |= AVFMT_NOFILE;
    //
    //    if (avformat_open_input(&fmtContext, "stream", inputFmt, nullptr) < 0) { return -1; }
    //
    //    // retrive input stream information
    //    if (avformat_find_stream_info(fmtContext, nullptr) < 0) { return -1; }
    //
    //    AVCodec* primaryCodec = nullptr;
    //    int bestStreamIndex = av_find_best_stream(fmtContext, AVMEDIA_TYPE_VIDEO, -1, -1, &primaryCodec, 0);
    //    if (bestStreamIndex < 0) { return -1; }
    //
    //    AVStream* primaryStream = fmtContext->streams[bestStreamIndex];
    //
    //    int64_t duration = fmtContext->duration;
    //    double durationSeconds = double(duration) / double(AV_TIME_BASE);
    //    printf("Duration: %f", durationSeconds);
    //    printf("Frames: %lli\n", primaryStream->nb_frames);
    //
    //    // TODO(vserhiienko): Codec is deprecated, switch to newer API.
    //    // AVCodecContext codecContext = {};
    //    // if (avcodec_parameters_to_context(&codecContext, primaryStream->codecpar) < 0) {
    //    //     return -1;
    //    // }
    //
    //    auto primaryStreamCodec = primaryStream->codec;
    //    if (avcodec_open2(primaryStreamCodec, primaryCodec, nullptr) < 0) { return -1; }
    //
    //    const int dst_width = primaryStreamCodec->width;
    //    const int dst_height = primaryStreamCodec->height;
    //    const AVPixelFormat dst_pix_fmt = AV_PIX_FMT_RGB24;
    //    // const AVPixelFormat dst_pix_fmt = AV_PIX_FMT_BGR24;
    //
    //    SwsContext* swsContext = sws_getCachedContext(nullptr,
    //                                                  primaryStreamCodec->width,
    //                                                  primaryStreamCodec->height,
    //                                                  primaryStreamCodec->pix_fmt,
    //                                                  dst_width,
    //                                                  dst_height,
    //                                                  dst_pix_fmt,
    //                                                  SWS_BICUBIC,
    //                                                  nullptr,
    //                                                  nullptr,
    //                                                  nullptr);
    //
    //    if (!swsContext) { return -1; }

    AVFrame* frame = av_frame_alloc();
    AVPicture* picture = reinterpret_cast<AVPicture*>(frame);
    std::vector<uint8_t> imageBuffer = {};

    size_t imageByteSize =
        av_image_get_buffer_size(videoStream.decodeFmt, videoStream.width, videoStream.height, videoStream.alignment);
    imageBuffer.resize(imageByteSize);
    av_image_fill_arrays(picture->data,
                         picture->linesize,
                         imageBuffer.data(),
                         videoStream.decodeFmt,
                         videoStream.width,
                         videoStream.height,
                         videoStream.alignment);

    AVFrame* decframe = av_frame_alloc();
    AVPacket packet = {};

    unsigned nb_frames = 0;
    bool end_of_stream = false;
    int got_pic = 0;
    int ret = 0;

    int64_t durationCounter = 0;
    double currentDuration = 0.0;

    do {
        if (!end_of_stream) {
            // read packet from input file
            ret = av_read_frame(videoStream.fmtContext, &packet);
            if (ret < 0 && ret != AVERROR_EOF) { return 2; }

            if (ret == 0 && packet.stream_index != videoStream.primaryStreamIndex) goto next_packet;

            end_of_stream = (ret == AVERROR_EOF);
        }

        if (end_of_stream) {
            // null packet for bumping process
            av_init_packet(&packet);
            packet.data = nullptr;
            packet.size = 0;
        }

        // decode video frame

        avcodec_decode_video2(videoStream.primaryCodecContext, decframe, &got_pic, &packet);
        // currentDuration = double(decframe->best_effort_timestamp) / double(AV_TIME_BASE);
        // currentDuration = double(durationCounter) / double(AV_TIME_BASE);
        // durationCounter += packet.duration;

        if (!got_pic) { goto next_packet; }

        currentDuration = av_frame_get_best_effort_timestamp(decframe) * av_q2d(videoStream.primaryStream->time_base);
        printf("\tav_frame_get_best_effort_timestamp=%lli\n", av_frame_get_best_effort_timestamp(decframe));
        printf("\tcurrentDuration=%f\n", currentDuration);

        // convert frame to OpenCV matrix
        sws_scale(videoStream.swsContext,
                  decframe->data,
                  decframe->linesize,
                  0,
                  decframe->height,
                  frame->data,
                  frame->linesize);
        //writeRGB24ToPNG(imageBuffer.data(), videoStream.width, videoStream.height, currentDuration);

        ++nb_frames;
    next_packet:

        // av_free_packet(&packet);
        if (packet.buf) { av_buffer_unref(&packet.buf); }

        packet.data = nullptr;
        packet.size = 0;
        av_packet_free_side_data(&packet);

    } while (!end_of_stream || got_pic);

    // std::cout << nb_frames << " frames decoded" << std::endl;

    av_frame_free(&decframe);
    av_frame_free(&frame);

    videoStreamClose(videoStream);
#endif
#endif
    return 0;
}

#endif
