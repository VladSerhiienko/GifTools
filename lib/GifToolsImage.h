#pragma once
#include "GifToolsManagedObject.h"
#include <span>

namespace giftools {

struct Buffer;

struct Image;
template <> Image* managedCast<Image>(ManagedObj* managedObj);

enum PixelFormat {
    PixelFormatUndefined,
    PixelFormatR8G8B8Unorm,
    PixelFormatR8G8B8A8Unorm,
};

size_t pixelFormatByteWidth(PixelFormat format);
using ImageStoragePtr = std::unique_ptr<uint8_t, void(*)(void*)>;
enum ImageStorageType { ImageStorageTypePtr, ImageStorageTypeVector };

struct Image : public ManagedObj {
    Image();
    ~Image() override;
    
    PixelFormat format = PixelFormatUndefined;
    size_t width = 0;
    size_t height = 0;
    size_t alignment = 1;
    uint8_t* bufferPtr = nullptr;
    size_t bufferSize = 0;
    
    ImageStorageType storageType;
    union {
        ImageStoragePtr ptrStorage;
        std::vector<uint8_t> vectorStorage;
    };
};

UniqueManagedObj<Image> imageLoadFromMemory(const Buffer* bufferObj);
UniqueManagedObj<Image> imageLoadFromMemory(const std::vector<uint8_t>& buffer);
UniqueManagedObj<Image> imageLoadFromMemory(const uint8_t* bufferPtr, size_t bufferSize);
UniqueManagedObj<Image> imageMakeResized(Image* imageObj, size_t width, size_t height);
UniqueManagedObj<Buffer> imageExportPNG(Image* imageObj);
void imageFree(Image* imageObj);

struct GifBuilder;
template <> GifBuilder* managedCast<GifBuilder>(ManagedObj* managedObj);

struct GifBuilder : public ManagedObj {
    virtual bool Begin(size_t width, size_t height, size_t delay) = 0;
    virtual bool AddImage(const Image* imageObj, size_t delay) = 0;
    virtual UniqueManagedObj<Buffer> End() = 0;
};

UniqueManagedObj<GifBuilder> gifBuilderInitialize(size_t width, size_t height, size_t delay);
bool gifBuilderAddImage(GifBuilder* gifBuilderObj, const Image* imageObj, size_t delay);
UniqueManagedObj<Buffer> gifBuilderFinalize(GifBuilder* gifBuilderObj);

}
