#include <cstdio>
#include <cstdint>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

std::vector<uint8_t> fileReadFileToBuffer(const char* imgFilePath) {
    if (FILE* imgFileHandle = fopen(imgFilePath, "rb")) {
        fseek(imgFileHandle, 0, SEEK_END);
        size_t imgFileSize = ftell(imgFileHandle);
        fseek(imgFileHandle, 0, SEEK_SET);
        std::vector<uint8_t> imgFileBuffer;
        imgFileBuffer.resize(imgFileSize);
        fread(imgFileBuffer.data(), imgFileSize, 1, imgFileHandle);
        fclose(imgFileHandle);
        return imgFileBuffer;
    }
    
    return {};
}

static int read_buffer(void *opaque, uint8_t *buf, int buf_size)
{
    // This function must fill the buffer with data and return number of bytes copied.
    // opaque is the pointer to private_data in the call to av_alloc_put_byte (4th param)
    
    memcpy(buf, opaque, buf_size);
    return buf_size;
}

struct InputStream {
    int64_t offset = 0;
    std::vector<uint8_t> buffer = {};
    
    int read(uint8_t *buf, int buf_size) {
        int bytes_read = std::max<int>(buf_size, buffer.size() - offset);
        memcpy(buf, buffer.data() + offset, bytes_read);
        offset += bytes_read;
        return bytes_read;
    }
    
};

static auto
Read(void * const opaque, uint8_t * p_buffer, int const buffer_capacity_bytes_count) noexcept {
    int result{-1};
    assert(opaque);
    
    if(opaque && p_buffer && (0 <= buffer_capacity_bytes_count)) {
        InputStream& stream = *reinterpret_cast<InputStream*>(opaque);
        auto const read_result{stream.read(p_buffer, buffer_capacity_bytes_count)};
        if((0 <= read_result) && (read_result <= buffer_capacity_bytes_count)) {
            result = read_result;
        }
    }
    
    return(result);
}

/// <summary>
/// Changes file pointer position or retrieves file size.
/// Basically should work like ::lseek and ::fstat C methods.
/// </summary>
/// <param name="opaque">
/// Opaque pointer to reader instance. Passing nullptr is not allowed.
/// </param>
/// <param name="pos">
/// Target offset. When retrieving file size this should be 0.
/// </param>
/// <param name="whence">
/// Flag indicating operation. Valid values are SEEK_SET, SEEK_CUR, SEEK_END (as in C library),
/// AVSEEK_SIZE and optional AVSEEK_FORCE bit.
/// </param>
/// <returns>
/// Non-negative values containing offset of the file pointer or file size in bytes.
/// Negative values if an error occurred.
/// </returns>
static auto
Seek(void * const opaque, int64_t const pos, int const whence) noexcept {
    int64_t result{AVERROR(EBADF)};
    assert(opaque);
    
    if(opaque) {
        InputStream& stream = *reinterpret_cast<InputStream*>(opaque);
        auto const action{whence & (SEEK_SET | SEEK_CUR | SEEK_END | AVSEEK_SIZE)};
        auto const forced{0 != (whence & AVSEEK_FORCE)}; // can be ignored
        switch(action) {
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
    return(result);
}

void write_rgb24_to_png(const uint8_t* data, size_t width, size_t height) {
    static uint32_t frame_counter = 0;
    char buff[256] = {};
    sprintf(buff, "write_rgb24_to_png_%09u.png", frame_counter++);
    stbi_write_png(buff, width, height, 3, data, width * 3);
}

int main(int argc, char** argv) {
    printf("Yup.");
    
    const char* file = "/Users/vserhiienko/Downloads/2020-02-23 18.53.40.mp4";
    InputStream stream = {0, fileReadFileToBuffer(file)};
    
    av_register_all();
    av_log_set_level(AV_LOG_TRACE);
    
    AVFormatContext *fmtContext;
    fmtContext = avformat_alloc_context();
    fmtContext->pb = avio_alloc_context(stream.buffer.data(), stream.buffer.size(), 0, &stream, Read, NULL, Seek);
    if(!fmtContext->pb) {
        return -1;
    }

    AVProbeData probeData = {};
    probeData.buf_size = std::min<int>(stream.buffer.size(), 4096);
    probeData.filename = "stream";
    probeData.buf = (unsigned char *) malloc(probeData.buf_size);
    memcpy(probeData.buf, stream.buffer.data(), 4096);
    
    AVInputFormat *inputFmt = av_probe_input_format(&probeData, 1);
    if(!inputFmt) {
        inputFmt = av_probe_input_format(&probeData, 0);
    }
    
    free(probeData.buf);
    probeData.buf = NULL;
    
    if(!inputFmt) {
        return -1;
    }

    inputFmt->flags |= AVFMT_NOFILE;
    
    if (avformat_open_input(&fmtContext, "stream", inputFmt, nullptr) < 0) {
        return -1;
    }
    
    // retrive input stream information
    if (avformat_find_stream_info(fmtContext, nullptr) < 0) {
        return -1;
    }
    
    AVCodec* primaryCodec = nullptr;
    int bestStreamIndex = av_find_best_stream(fmtContext, AVMEDIA_TYPE_VIDEO, -1, -1, &primaryCodec, 0);
    if (bestStreamIndex < 0) {
        return -1;
    }

    AVStream* primaryStream = fmtContext->streams[bestStreamIndex];
    
    // TODO(vserhiienko): Codec is deprecated, switch to newer API.
    // AVCodecContext codecContext = {};
    // if (avcodec_parameters_to_context(&codecContext, primaryStream->codecpar) < 0) {
    //     return -1;
    // }
    
    auto primaryStreamCodec = primaryStream->codec;
    if (avcodec_open2(primaryStreamCodec, primaryCodec, nullptr) < 0) {
        return -1;
    }
    

    const int dst_width = primaryStreamCodec->width;
    const int dst_height = primaryStreamCodec->height;
    const AVPixelFormat dst_pix_fmt = AV_PIX_FMT_RGB24;
    // const AVPixelFormat dst_pix_fmt = AV_PIX_FMT_BGR24;
    
    SwsContext* swsContext = sws_getCachedContext(
        nullptr, primaryStreamCodec->width, primaryStreamCodec->height, primaryStreamCodec->pix_fmt,
        dst_width, dst_height, dst_pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr);

    if (!swsContext) {
        return -1;
    }
    
    AVFrame* frame = av_frame_alloc();
    AVPicture* picture = reinterpret_cast<AVPicture*>(frame);
    
    size_t imageByteSize = av_image_get_buffer_size(dst_pix_fmt, dst_width, dst_height, 1);
    std::vector<uint8_t> imageBuffer(imageByteSize);
    av_image_fill_arrays(picture->data, picture->linesize, imageBuffer.data(), dst_pix_fmt, dst_width, dst_height, 1);

    AVFrame* decframe = av_frame_alloc();
    AVPacket packet = {};

    unsigned nb_frames = 0;
    bool end_of_stream = false;
    int got_pic = 0;
    int ret = 0;
    
    do {
        if (!end_of_stream) {
            // read packet from input file
            ret = av_read_frame(fmtContext, &packet);
            if (ret < 0 && ret != AVERROR_EOF) {
                return 2;
            }
            
            if (ret == 0 && packet.stream_index != bestStreamIndex)
                goto next_packet;
                
            end_of_stream = (ret == AVERROR_EOF);
        }
            
        if (end_of_stream) {
            // null packet for bumping process
            av_init_packet(&packet);
            packet.data = nullptr;
            packet.size = 0;
        }

        // decode video frame
        avcodec_decode_video2(primaryStreamCodec, decframe, &got_pic, &packet);
        
        if (!got_pic) {
            goto next_packet;
        }
        
        // convert frame to OpenCV matrix
        sws_scale(swsContext, decframe->data, decframe->linesize, 0, decframe->height, frame->data, frame->linesize);
        write_rgb24_to_png(imageBuffer.data(), dst_width, dst_height);

        ++nb_frames;
        next_packet:
        
        // av_free_packet(&packet);
        if (packet.buf) {
            av_buffer_unref(&packet.buf);
        }
        
        packet.data = nullptr;
        packet.size = 0;
        av_packet_free_side_data(&packet);
        
    } while (!end_of_stream || got_pic);
    
    // std::cout << nb_frames << " frames decoded" << std::endl;
    
    av_frame_free(&decframe);
    av_frame_free(&frame);
    avcodec_close(primaryStreamCodec);
    avformat_close_input(&fmtContext);
    
    return 0;
}
