#include "GifToolsFFmpegInputStream.h"
#include "GifToolsBuffer.h"
#include "GifToolsManagedTypes.h"

struct FFmpegInputStreamImpl;

template <>
uint8_t giftools::managedType<giftools::FFmpegInputStream>() {
    return static_cast<uint8_t>(giftools::BuildintManagedType::FFmpegInputStream);
}

template <>
uint8_t giftools::managedType<FFmpegInputStreamImpl>() {
    return static_cast<uint8_t>(giftools::BuildintManagedType::FFmpegInputStream);
}

#ifndef GIFTOOLS_USE_FFMPEG

struct FFmpegInputStreamImpl : giftools::FFmpegInputStream {
    FFmpegInputStreamImpl() = default;
    ~FFmpegInputStreamImpl() override = default;
    giftools::FFmpegInputStreamReadFnPtr ffmpegInputStreamReadFnPtr() { return nullptr; }
    giftools::FFmpegInputStreamSeekFnPtr ffmpegInputStreamSeekFnPtr() { return nullptr; }
};

giftools::UniqueManagedObj<giftools::FFmpegInputStream> ffmpegInputStreamLoadFromMemory(const giftools::Buffer* bufferObj) { return nullptr; }
// int32_t giftools::ffmpegInputStreamRead(void* opaque, uint8_t* p_buffer, int32_t buffer_capacity_bytes_count) noexcept { return -1; }
// int64_t giftools::ffmpegInputStreamSeek(void* opaque, int64_t pos, int32_t whence) noexcept { return -1; }

#else

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/avutil.h>
    #include <libavutil/imgutils.h>
    #include <libavutil/pixdesc.h>
    #include <libswscale/swscale.h>
}

struct FFmpegInputStreamImpl : giftools::FFmpegInputStream {
    FFmpegInputStreamImpl() = default;
    ~FFmpegInputStreamImpl() override = default;

    const giftools::Buffer* bufferObj = nullptr;
    const uint8_t* bufferDataPtr = nullptr;
    size_t bufferSize = 0;
    int64_t currBufferOffset = 0;

    int read(uint8_t* buf, int buf_size) {
        int bytes_read = std::max<int>(buf_size, bufferSize - currBufferOffset);
        memcpy(buf, bufferDataPtr + currBufferOffset, bytes_read);
        currBufferOffset += bytes_read;
        return bytes_read;
    }
    
    const giftools::Buffer* buffer() const override;
    giftools::FFmpegInputStreamReadFnPtr ffmpegInputStreamReadFnPtr() const override;
    giftools::FFmpegInputStreamSeekFnPtr ffmpegInputStreamSeekFnPtr() const override;
};

giftools::UniqueManagedObj<giftools::FFmpegInputStream>
giftools::ffmpegInputStreamLoadFromMemory(const giftools::Buffer* bufferObj) {
    auto streamObj = managedObjStorageDefault().make<FFmpegInputStreamImpl>();
    streamObj->bufferObj = bufferObj;
    streamObj->bufferDataPtr = bufferObj->data();
    streamObj->bufferSize = bufferObj->size();
    streamObj->currBufferOffset = 0;
    return giftools::UniqueManagedObj<giftools::FFmpegInputStream>(streamObj.release());
}

namespace {
int32_t ffmpegInputStreamRead(void* opaque, uint8_t* p_buffer, int32_t buffer_capacity_bytes_count) noexcept {
    int32_t result{AVERROR(EPERM)};
    
    assert(opaque);
    if (opaque && p_buffer && (0 <= buffer_capacity_bytes_count)) {
        auto& stream = *reinterpret_cast<FFmpegInputStreamImpl*>(opaque);
        auto const read_result{stream.read(p_buffer, buffer_capacity_bytes_count)};
        if ((0 <= read_result) && (read_result <= buffer_capacity_bytes_count)) { result = read_result; }
    }

    return result;
}
int64_t ffmpegInputStreamSeek(void* opaque, int64_t const pos, int32_t whence) noexcept {
    int64_t result{AVERROR(EBADF)};
    
    assert(opaque);
    if (opaque) {
        auto& stream = *reinterpret_cast<FFmpegInputStreamImpl*>(opaque);
        
        // auto const forced{0 != (whence & AVSEEK_FORCE)}; // can be ignored
        const int32_t action = whence & (SEEK_SET | SEEK_CUR | SEEK_END | AVSEEK_SIZE);
        switch (action) {
            case SEEK_SET: {
                stream.currBufferOffset = pos;
            } break;
            case SEEK_CUR: {
                stream.currBufferOffset += pos;
            } break;
            case SEEK_END: {
                stream.currBufferOffset = stream.bufferSize + pos;
            } break;
            case AVSEEK_SIZE: {
                result = stream.bufferSize;
            } break;
        }
    }

    return result;
}
}

const giftools::Buffer* FFmpegInputStreamImpl::buffer() const { return bufferObj; }
giftools::FFmpegInputStreamReadFnPtr FFmpegInputStreamImpl::ffmpegInputStreamReadFnPtr() const { return &ffmpegInputStreamRead; }
giftools::FFmpegInputStreamSeekFnPtr FFmpegInputStreamImpl::ffmpegInputStreamSeekFnPtr() const { return &ffmpegInputStreamSeek; }

#endif
