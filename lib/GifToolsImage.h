#pragma once
#include "GifToolsManagedObject.h"

namespace giftools {

struct Buffer;
struct Image;
struct GifBuilder;

template<> uint8_t managedType<Image>();
template<> uint8_t managedType<GifBuilder>();

enum PixelFormat {
    PixelFormatUndefined,
    PixelFormatR8G8B8Unorm,
    PixelFormatR8G8B8A8Unorm,
};

size_t pixelFormatByteWidth(PixelFormat format);

struct Image : public ManagedObj {
    virtual PixelFormat format() const = 0;
    virtual size_t width() const = 0;
    virtual size_t height() const = 0;
    virtual size_t alignment() const = 0;
    virtual const uint8_t* bufferPtr() const = 0;
    virtual uint8_t* mutableBufferPtr() = 0;
    virtual size_t bufferSize() const = 0;
};

size_t imageWidth(const Image* imageObj);
size_t imageHeight(const Image* imageObj);
size_t imageFormat(const Image* imageObj);
UniqueManagedObj<Image> imageClone(const Image* imageObj);
UniqueManagedObj<Image> imageLoadFromMemory(const Buffer* bufferObj);
UniqueManagedObj<Image> imageLoadFromMemory(const std::vector<uint8_t>& buffer);
UniqueManagedObj<Image> imageLoadFromMemory(const uint8_t* bufferPtr, size_t bufferSize);
UniqueManagedObj<Image> imageResizeOrClone(const Image* imageObj, size_t width, size_t height);
UniqueManagedObj<Buffer> imageExportToPNG(const Image* imageObj);

struct GifBuilder : public ManagedObj {
    virtual bool Begin(size_t width, size_t height, size_t delay) = 0;
    virtual bool AddImage(const Image* imageObj, size_t delay) = 0;
    virtual UniqueManagedObj<Buffer> End() = 0;
};

UniqueManagedObj<GifBuilder> gifBuilderInitialize(size_t width, size_t height, size_t delay);
bool gifBuilderAddImage(GifBuilder* gifBuilderObj, const Image* imageObj, size_t delay);
UniqueManagedObj<Buffer> gifBuilderFinalize(GifBuilder* gifBuilderObj);



}
