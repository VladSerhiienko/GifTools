#include <cstdio>
#include <cstdint>
#include <vector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}


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

/// <summary>
/// Reads up to buffer_capacity_bytes_count bytes into supplied buffer.
/// Basically should work like ::read C method.
/// </summary>
/// <param name="opaque">
/// Opaque pointer to reader instance. Passing nullptr is not allowed.
/// </param>
/// <param name="p_buffer">
/// Pointer to data buffer. Passing nullptr is not allowed.
/// </param>
/// <param name="buffer_capacity_bytes_count">
/// Size of the buffer pointed to by p_buffer, in bytes.
/// Passing value less than or equal to 0 is not allowed.
/// </param>
/// <returns>
/// Non negative values containing amount of bytes actually read. 0 if EOF has been reached.
/// -1 if an error occurred.
/// </returns>
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
    
    if (avcodec_open2(primaryStream->codec, primaryCodec, nullptr) < 0) {
        return -1;
    }
    
    return 0;
}
