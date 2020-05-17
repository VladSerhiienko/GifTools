#pragma once
#include "GifToolsManagedObject.h"

namespace giftools {

class Buffer;
class FFmpegInputStream;

template <>
uint8_t managedType<FFmpegInputStream>();

using FFmpegInputStreamReadFnPtr = int32_t (*) (void* opaque, uint8_t* p_buffer, int32_t buffer_capacity_bytes_count) noexcept;
using FFmpegInputStreamSeekFnPtr = int64_t (*) (void* opaque, int64_t pos, int32_t whence) noexcept;

class FFmpegInputStream : public ManagedObj {
public:
    ~FFmpegInputStream() override = default;
    virtual const Buffer* buffer() const = 0;
    virtual FFmpegInputStreamReadFnPtr ffmpegInputStreamReadFnPtr() const = 0;
    virtual FFmpegInputStreamSeekFnPtr ffmpegInputStreamSeekFnPtr() const = 0;
protected:
    FFmpegInputStream() = default;
};

UniqueManagedObj<FFmpegInputStream> ffmpegInputStreamLoadFromMemory(const Buffer* bufferObj);


}
