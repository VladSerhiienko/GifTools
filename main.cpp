#include <GifToolsImage.h>
#include <GifToolsBuffer.h>
#include <GifToolsFile.h>

#include <cstdint>
#include <cstdio>
#include <optional>
#include <string_view>
#include <vector>
#include <string>
// #include <iostream>

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

enum FileType { FileTypeNone, FileTypeBinary, FileTypeText, };
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

struct InputStream {
    int64_t offset = 0;
    std::vector<uint8_t> buffer = {};

    int read(uint8_t* buf, int buf_size) {
        int bytes_read = std::max<int>(buf_size, buffer.size() - offset);
        memcpy(buf, buffer.data() + offset, bytes_read);
        offset += bytes_read;
        return bytes_read;
    }
};

static int inputStreamRead(void* const opaque, uint8_t* p_buffer, int const buffer_capacity_bytes_count) noexcept {
    int result{-1};
    assert(opaque);

    if (opaque && p_buffer && (0 <= buffer_capacity_bytes_count)) {
        InputStream& stream = *reinterpret_cast<InputStream*>(opaque);
        auto const read_result{stream.read(p_buffer, buffer_capacity_bytes_count)};
        if ((0 <= read_result) && (read_result <= buffer_capacity_bytes_count)) { result = read_result; }
    }

    return result;
}

static int64_t inputStreamSeek(void* const opaque, int64_t const pos, int const whence) noexcept {
    int64_t result{AVERROR(EBADF)};
    assert(opaque);

    if (opaque) {
        InputStream& stream = *reinterpret_cast<InputStream*>(opaque);
        auto const action{whence & (SEEK_SET | SEEK_CUR | SEEK_END | AVSEEK_SIZE)};
        // auto const forced{0 != (whence & AVSEEK_FORCE)}; // can be ignored

        switch (action) {
            case SEEK_SET: {
                stream.offset = pos;
            } break;
            case SEEK_CUR: {
                stream.offset += pos;
            } break;
            case SEEK_END: {
                stream.offset = stream.buffer.size() + pos;
            } break;
            case AVSEEK_SIZE: {
                result = stream.buffer.size();
            } break;
        }
    }

    return result;
}

void writeRGB24ToPNG(const uint8_t* data, size_t width, size_t height, double duration) {
    static uint32_t frame_counter = 0;
    char buff[256] = {};
    sprintf(buff, "write_rgb24_to_png_%09u_%f.png", frame_counter++, duration);
    printf("PNG: %s\n", buff);
    stbi_write_png(buff, width, height, 3, data, width * 3);
}

struct VideoStream {
    InputStream stream = {};
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
    videoStream.stream = {0, std::move(optFile.value().buffer)};

    // TODO(vserhiienko): deprecated.
    av_register_all();
    av_log_set_level(AV_LOG_TRACE);

    videoStream.fmtContext = avformat_alloc_context();
    videoStream.fmtContext->pb = avio_alloc_context(videoStream.stream.buffer.data(),
                                                    videoStream.stream.buffer.size(),
                                                    0,
                                                    &videoStream.stream,
                                                    inputStreamRead,
                                                    NULL,
                                                    inputStreamSeek);
    if (!videoStream.fmtContext->pb) { return {}; }

    videoStream.probeData.filename = "stream";
    videoStream.probeData.buf_size = std::min<int>(videoStream.stream.buffer.size(), 4096);
    videoStream.probeBuffer.resize(videoStream.probeData.buf_size);
    videoStream.probeData.buf = videoStream.probeBuffer.data();
    memcpy(videoStream.probeData.buf, videoStream.stream.buffer.data(), 4096);

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

std::vector<uint8_t> videoStreamBruteDumpFrameAt(VideoStream& videoStream, double sampleTime) {
    int ret = 0;
    
    // clang-format off
    assert(videoStream.width > 0);
    assert(videoStream.height > 0);
    assert(videoStream.alignment == 1);
    const size_t imgBufferSize = av_image_get_buffer_size(videoStream.decodeFmt, videoStream.width, videoStream.height, videoStream.alignment);
    // clang-format on
    
    VideoFrame frame = {};
    frame.decodedFrame = av_frame_alloc();
    frame.encodedFrame = av_frame_alloc();
    frame.imageByteBuffer.resize(imgBufferSize);
    
    ret = av_image_fill_arrays(frame.encodedFrame->data,
                               frame.encodedFrame->linesize,
                               frame.imageByteBuffer.data(),
                               videoStream.decodeFmt,
                               videoStream.width,
                               videoStream.height,
                               videoStream.alignment);
    if (ret < 0) { return {}; }

    FlushBuffersGuard flushBuffersGuard{videoStream};
    FreeFramePacketGuard freePacketGuard{frame};

    bool endOfStream = false;
    bool frameAcquired = false;

    printf("while\n");
    while (!endOfStream || frameAcquired) {
        frameAcquired = false;

        while (!frameAcquired) {
            bool keepSearchingPackets = true;
            while (keepSearchingPackets && !endOfStream) {
                printf("av_read_frame\n");

                videoFrameFreePacket(frame);
                ret = av_read_frame(videoStream.fmtContext, &frame.packet);
                endOfStream = (ret == AVERROR_EOF);
                if (ret < 0 && ret != AVERROR_EOF) { return {}; }

                if (ret == 0 && frame.packet.stream_index == videoStream.primaryStreamIndex) {
                    keepSearchingPackets = false;
                    break;
                }
            }

            // clang-format off
            int didRetrievePicture = 0;
            printf("avcodec_decode_video2\n");
            ret = avcodec_decode_video2(videoStream.primaryCodecContext, frame.decodedFrame, &didRetrievePicture, &frame.packet);
            if (ret < 0) { return {}; }
            // clang-format on

            frameAcquired = didRetrievePicture > 0;
            if (endOfStream) { break; }
        }

        const int64_t mostAccurateTimestamp = av_frame_get_best_effort_timestamp(frame.decodedFrame);
        const double mostAccurateTime = mostAccurateTimestamp * videoStream.primaryStreamTimeBase;
        frame.timeTimeBaseEst = mostAccurateTimestamp;
        frame.timeSecondsEst = mostAccurateTime;

        printf("mostAccurateTime = %f\n", mostAccurateTime);
        printf("sampleTime = %f\n", sampleTime);

        constexpr double kSampleTimeTolerance = 0.001;
        if (mostAccurateTime >= std::max(kSampleTimeTolerance, sampleTime - kSampleTimeTolerance)) {
            printf("sws_scale\n");

            ret = sws_scale(videoStream.swsContext,
                            frame.decodedFrame->data,
                            frame.decodedFrame->linesize,
                            0,
                            frame.decodedFrame->height,
                            frame.encodedFrame->data,
                            frame.encodedFrame->linesize);
            if (ret < 0) { return {}; }
            return frame.imageByteBuffer;
        }
    }

    return {};
}

void testGifWriter() {
    using namespace giftools;
    
    UniqueManagedObj<Buffer> bufferObjs[4];
    bufferObjs[0] = fileBinaryRead("/Users/vserhiienko/Downloads/Photos/IMG_20191217_083058.jpg");
    bufferObjs[1] = fileBinaryRead("/Users/vserhiienko/Downloads/Photos/IMG_20191217_083059.jpg");
    bufferObjs[2] = fileBinaryRead("/Users/vserhiienko/Downloads/Photos/IMG_20191217_083101.jpg");
    bufferObjs[3] = fileBinaryRead("/Users/vserhiienko/Downloads/Photos/IMG_20191217_083059.jpg");
    
    
    UniqueManagedObj<Image> imageObjs[4];
    imageObjs[0] = imageLoadFromMemory(bufferObjs[0].get());
    imageObjs[1] = imageLoadFromMemory(bufferObjs[1].get());
    imageObjs[2] = imageLoadFromMemory(bufferObjs[2].get());
    imageObjs[3] = imageLoadFromMemory(bufferObjs[3].get());
    
    const size_t delay = 100;
    const size_t width = imageObjs[0]->width;
    const size_t height = imageObjs[0]->height;
    
    UniqueManagedObj<GifBuilder> gifBuilderObj = gifBuilderInitialize(width, height, delay);
    gifBuilderAddImage(gifBuilderObj.get(), imageObjs[0].get(), delay);
    gifBuilderAddImage(gifBuilderObj.get(), imageObjs[1].get(), delay);
    gifBuilderAddImage(gifBuilderObj.get(), imageObjs[2].get(), delay);
    gifBuilderAddImage(gifBuilderObj.get(), imageObjs[3].get(), delay);
    
    UniqueManagedObj<Buffer> gifBufferObj = gifBuilderFinalize(gifBuilderObj.get());
    fileBinaryWrite("TestGif.gif", gifBufferObj.get());
}

int main(int argc, char** argv) {
    printf("Yup.");
    
    giftools::ManagedObjStorage::init();
    testGifWriter();

    const char* testMp4File = "/Users/vserhiienko/Downloads/2020-02-23 18.53.40.mp4";
    auto optVideoStream = videoStreamOpen(testMp4File);
    if (!optVideoStream) { return -1; }

    VideoStream& videoStream = *optVideoStream;

#if 1
    for (double t = 0.0; t < 4.0; t += 0.01) {
        auto imageBuffer = videoStreamBruteDumpFrameAt(videoStream, t);
        if (!imageBuffer.empty()) { writeRGB24ToPNG(imageBuffer.data(), videoStream.width, videoStream.height, t); }
    }

    //    videoStreamBruteDumpFrameAt(videoStream, 0);
    //    videoStreamBruteDumpFrameAt(videoStream, 2207);
    //    videoStreamBruteDumpFrameAt(videoStream, 3428);
    //    videoStreamBruteDumpFrameAt(videoStream, 6740);

    //    int64_t t0 = 0;
    //    int64_t t1 = 100;
    //    for (int64_t t = t0; t < t1; t += 5) {
    //        videoStreamDumpFrameAt(videoStream, t);
    //    }

    videoStreamClose(videoStream);
    return 0;
#endif

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

    size_t imageByteSize = av_image_get_buffer_size(videoStream.decodeFmt, videoStream.width, videoStream.height, videoStream.alignment);
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
        writeRGB24ToPNG(imageBuffer.data(), videoStream.width, videoStream.height, currentDuration);

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
    return 0;
}
