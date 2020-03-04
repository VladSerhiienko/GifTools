#pragma once
#include "GifToolsManagedObject.h"
#include <span>

namespace giftools {

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
    virtual ~Image() override;
    
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

UniqueManagedObj<Image> imageLoadFromMemory(const std::vector<uint8_t>& buffer);
UniqueManagedObj<Image> imageMakeResized(Image* image, size_t width, size_t height);
std::vector<uint8_t> imageExportPNG(Image* image);
void imageFree(Image* image);

template <>
Image* managedCast<Image>(ManagedObj* managedObj);

template<>
uint8_t managedType<Image>();

}
