#pragma once
#include "GifToolsManagedObject.h"
#include <span>

namespace giftools {

enum PixelFormat {
    PixelFormatUndefined,
    PixelFormatR8G8B8Unorm,
    PixelFormatR8G8B8A8Unorm,
};

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

UniqueManagedObj<Image> imageLoadFromFileBuffer(const std::vector<uint8_t>& fileBuffer);

}
