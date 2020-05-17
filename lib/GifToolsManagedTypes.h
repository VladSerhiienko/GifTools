#pragma once

namespace giftools {
    class Buffer;
    class File;
    class Image;
    class GifBuilder;
    class FFmpegInputStream;
    class FFmpegVideoFrame;
    class FFmpegVideoStream;
    
    enum class BuiltinManagedType : uint8_t {
        Undefined = 0,
        Buffer,
        File,
        Image,
        GifBuilder,
        FFmpegInputStream,
        FFmpegVideoFrame,
        FFmpegVideoStream,
        Count
    };
}
