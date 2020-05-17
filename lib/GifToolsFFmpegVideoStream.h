#pragma once

#pragma once
#include "GifToolsManagedObject.h"

namespace giftools {

class Buffer;
class Image;
class FFmpegInputStream;
class FFmpegVideoFrame;
class FFmpegVideoStream;

template <>
uint8_t managedType<FFmpegVideoFrame>();

template <>
uint8_t managedType<FFmpegVideoStream>();

class FFmpegVideoFrame : public ManagedObj {
public:
    ~FFmpegVideoFrame() override = default;
    virtual double estimatedSampleTimeSeconds() const = 0;
    virtual const Image* image() const = 0;
protected:
    FFmpegVideoFrame() = default;
};

class FFmpegVideoStream : public ManagedObj {
public:
    ~FFmpegVideoStream() override = default;
    virtual double estimatedTotalDurationSeconds() const = 0;
    virtual double estimatedFrameDurationSeconds() const = 0;
protected:
    FFmpegVideoStream() = default;
};

UniqueManagedObj<FFmpegVideoStream> ffmpegVideoStreamOpen(const FFmpegInputStream* ffmpegInputStream);
UniqueManagedObj<FFmpegVideoFrame> ffmpegVideoStreamPickBestFrame(const FFmpegVideoStream* ffmpegVideoStream, double sampleTime);
void ffmpegVideoStreamClose(FFmpegVideoStream* ffmpegVideoStream);

}
