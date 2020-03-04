#include "GifToolsImage.h"

#define STB_IMAGE_IMPLEMENTATION 1
#include "stb_image.h"
#include "stb_image_resize.h"
#include "stb_image_write.h"
#include "gif.h"

template<>
uint8_t giftools::managedType<giftools::Image>() { return 1; }


giftools::Image::Image() {
    storageType = ImageStorageTypePtr;
    ptrStorage.reset();
}

giftools::Image::~Image() {
    switch (storageType) {
    case ImageStorageTypePtr:
        ptrStorage.reset();
        ptrStorage.~unique_ptr();
        break;

    default:
        assert(storageType == ImageStorageTypeVector);
        std::vector<uint8_t>().swap(vectorStorage);
        vectorStorage.~vector();
        break;
    }
}

giftools::UniqueManagedObj<giftools::Image>
giftools::imageLoadFromFileBuffer(const std::vector<uint8_t>& fileBuffer) {
    int x, y, components;
    stbi_uc* imgBuffer = stbi_load_from_memory(fileBuffer.data(), fileBuffer.size() , &x, &y, &components, STBI_rgb_alpha);
    size_t imgBufferSize = x * y * STBI_rgb_alpha;
    
    if (!imgBuffer || !imgBufferSize) { return {}; }

    auto img = managedObjStorageDefault().make<Image>();
    img->storageType = ImageStorageTypePtr;
    img->ptrStorage = ImageStoragePtr(imgBuffer, stbi_image_free);
    img->bufferPtr = imgBuffer;
    img->bufferSize = imgBufferSize;
    
    return img;
}
