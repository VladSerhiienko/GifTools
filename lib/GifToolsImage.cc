#include "GifToolsImage.h"

#define STB_IMAGE_IMPLEMENTATION 1
#define STB_IMAGE_RESIZE_IMPLEMENTATION 1
#include "stb_image.h"
#include "stb_image_resize.h"
#include "stb_image_write.h"
#include "gif.h"

template<>
uint8_t giftools::managedType<giftools::Image>() { return 1; }

template <>
giftools::Image* giftools::managedCast<giftools::Image>(ManagedObj* managedObj) {
    if (!managedObj) { return nullptr; }
    if (managedObj->objId().type != managedType<Image>()) { return nullptr; }
    return static_cast<Image*>(managedObj);
}

size_t giftools::pixelFormatByteWidth(giftools::PixelFormat format) {
    switch (format) {
        case giftools::PixelFormatR8G8B8Unorm: return 3;
        case giftools::PixelFormatR8G8B8A8Unorm: return 4;
        default: return 0;
    }
}

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

void giftools::imageFree(Image* image) {
    ManagedObjStorageDeleter()(image);
}

giftools::UniqueManagedObj<giftools::Image>
giftools::imageMakeResized(giftools::Image* img, size_t width, size_t height) {
    if (!img) { return {}; }
    if (!width || !height) { return {}; }
    if (img->width == width && img->height == height) { return {}; }
    
    const size_t pixelWidth = pixelFormatByteWidth(img->format);
    const size_t imgStride = img->width * pixelWidth;
    const size_t resizedImgStride = width * pixelWidth;
    const size_t resizedImgBufferSize = width * height * pixelWidth;
    
    auto resizedImg = managedObjStorageDefault().make<Image>();
    resizedImg->width = width;
    resizedImg->height = height;
    resizedImg->format = img->format;
    resizedImg->storageType = ImageStorageTypeVector;
    resizedImg->vectorStorage.resize(resizedImgBufferSize);
    resizedImg->bufferPtr = resizedImg->vectorStorage.data();
    resizedImg->bufferSize = resizedImg->vectorStorage.size();
    stbir_resize_uint8(img->bufferPtr,
                       img->width,
                       img->height,
                       imgStride,
                       resizedImg->bufferPtr,
                       resizedImg->width,
                       resizedImg->height,
                       resizedImgStride,
                       pixelWidth);
    return resizedImg;
}

giftools::UniqueManagedObj<giftools::Image>
giftools::imageLoadFromMemory(const std::vector<uint8_t>& buffer) {
    int x = 0, y = 0, components = 0;
    stbi_uc* imgBuffer = stbi_load_from_memory(buffer.data(), buffer.size() , &x, &y, &components, STBI_rgb_alpha);
    size_t imgBufferSize = x * y * STBI_rgb_alpha;

    if (!imgBuffer || !imgBufferSize) { return {}; }

    auto img = managedObjStorageDefault().make<Image>();
    img->width = x;
    img->height = y;
    img->format = PixelFormatR8G8B8A8Unorm;
    img->storageType = ImageStorageTypePtr;
    img->ptrStorage = ImageStoragePtr(imgBuffer, stbi_image_free);
    img->bufferPtr = imgBuffer;
    img->bufferSize = imgBufferSize;
    return img;
}
