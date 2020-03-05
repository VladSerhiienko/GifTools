#include "GifToolsImage.h"
#include "GifToolsBuffer.h"

#define STB_IMAGE_IMPLEMENTATION 1
#define STB_IMAGE_WRITE_IMPLEMENTATION 1
#define STB_IMAGE_RESIZE_IMPLEMENTATION 1
#include "stb_image.h"
#include "stb_image_resize.h"
#include "stb_image_write.h"
#include "gif.h"

template<>
uint8_t giftools::managedType<giftools::Image>() { return 1; }

size_t giftools::pixelFormatByteWidth(giftools::PixelFormat format) {
    switch (format) {
        case giftools::PixelFormatR8G8B8Unorm: return 3;
        case giftools::PixelFormatR8G8B8A8Unorm: return 4;
        default: return 0;
    }
}

giftools::Image::Image() {
    storageType = ImageStorageTypePtr;
    new (&ptrStorage)ImageStoragePtr(0, stbi_image_free);
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

void giftools::imageFree(Image* imageObj) {
    ManagedObjStorageDeleter()(imageObj);
}

giftools::UniqueManagedObj<giftools::Image>
giftools::imageMakeResized(giftools::Image* imageObj, size_t width, size_t height) {
    if (!imageObj) { return {}; }
    if (!width || !height) { return {}; }
    if (imageObj->width == width && imageObj->height == height) { return {}; }
    
    const size_t pixelWidth = pixelFormatByteWidth(imageObj->format);
    const size_t imgStride = imageObj->width * pixelWidth;
    const size_t resizedImgStride = width * pixelWidth;
    const size_t resizedImgBufferSize = width * height * pixelWidth;
    
    auto resizedImageObj = managedObjStorageDefault().make<Image>();
    resizedImageObj->width = width;
    resizedImageObj->height = height;
    resizedImageObj->format = imageObj->format;
    resizedImageObj->storageType = ImageStorageTypeVector;
    resizedImageObj->vectorStorage.resize(resizedImgBufferSize);
    resizedImageObj->bufferPtr = resizedImageObj->vectorStorage.data();
    resizedImageObj->bufferSize = resizedImageObj->vectorStorage.size();
    stbir_resize_uint8(imageObj->bufferPtr,
                       imageObj->width,
                       imageObj->height,
                       imgStride,
                       resizedImageObj->bufferPtr,
                       resizedImageObj->width,
                       resizedImageObj->height,
                       resizedImgStride,
                       pixelWidth);
    return resizedImageObj;
}

namespace {

struct StbiWriterContext {
    std::vector<uint8_t> contents;
};

void StbiWriterFn(void *context, void *data, int size) {
    if (auto w = reinterpret_cast<StbiWriterContext*>(context)) {
        size_t currentPosition = w->contents.size();
        w->contents.resize(currentPosition + size);
        std::memcpy(&w->contents[currentPosition], data, size);
    }
}

}

giftools::UniqueManagedObj<giftools::Image>
giftools::imageLoadFromMemory(const Buffer* bufferObj) {
    return imageLoadFromMemory(bufferObj->contents.data(), bufferObj->contents.size());
}

giftools::UniqueManagedObj<giftools::Image>
giftools::imageLoadFromMemory(const std::vector<uint8_t>& buffer) {
    return imageLoadFromMemory(buffer.data(), buffer.size());
}

giftools::UniqueManagedObj<giftools::Image>
giftools::imageLoadFromMemory(const uint8_t* bufferPtr, size_t bufferSize) {
    int x = 0, y = 0, components = 0;
    stbi_uc* imgBuffer = stbi_load_from_memory(bufferPtr, bufferSize , &x, &y, &components, STBI_rgb_alpha);
    size_t imgBufferSize = x * y * STBI_rgb_alpha;

    if (!imgBuffer || !imgBufferSize) { return {}; }

    auto imageObj = managedObjStorageDefault().make<Image>();
    imageObj->width = x;
    imageObj->height = y;
    imageObj->format = PixelFormatR8G8B8A8Unorm;
    imageObj->storageType = ImageStorageTypePtr;
    imageObj->ptrStorage = ImageStoragePtr(imgBuffer, stbi_image_free);
    imageObj->bufferPtr = imgBuffer;
    imageObj->bufferSize = imgBufferSize;
    return imageObj;
}

giftools::UniqueManagedObj<giftools::Buffer>
giftools::imageExportPNG(Image* imageObj) {
    if (!imageObj) { return {}; }
    
    const size_t pixelWidth = pixelFormatByteWidth(imageObj->format);
    const size_t stride = imageObj->width * pixelWidth;
    assert(pixelWidth && stride && imageObj->height);
    
    StbiWriterContext writer = {};
    stbi_write_png_to_func(StbiWriterFn, &writer, imageObj->width, imageObj->height, pixelWidth, imageObj->bufferPtr, stride);

    auto bufferObj = bufferFromVector(std::move(writer.contents));
    return bufferObj;
}

template<> uint8_t giftools::managedType<giftools::GifBuilder>() { return 4; }

struct ConcreteGifBuilder;
template<> uint8_t giftools::managedType<ConcreteGifBuilder>() { return managedType<giftools::GifBuilder>() ; }

struct ConcreteGifBuilder : public giftools::GifBuilder {
    ~ConcreteGifBuilder() override = default;
    
    
    GifWriter writer = {};
    GifVectorFileBuffer vectorBuffer = {};
    
    ConcreteGifBuilder() {
        writer.fileBuffer = &vectorBuffer;
    }
    
    bool Begin(size_t width, size_t height, size_t delay) override {
        return GifBegin(&writer, "", width, height, delay);
    }
    bool AddImage(const giftools::Image* imageObj, size_t delay) override {
        return GifWriteFrame(&writer, imageObj->bufferPtr, imageObj->width, imageObj->height, delay);
    }
    giftools::UniqueManagedObj<giftools::Buffer> End() override {
        if (!GifEnd(&writer)) { return {}; }
        return giftools::bufferFromVector(std::move(vectorBuffer.contents));
    }
};

giftools::UniqueManagedObj<giftools::GifBuilder> giftools::gifBuilderInitialize(size_t width, size_t height, size_t delay) {
    auto gifBuilderObj = managedObjStorageDefault().make<ConcreteGifBuilder>();
    if (!gifBuilderObj) { return {}; }
    if (!gifBuilderObj->Begin(width, height, delay))  { return {}; }
    return gifBuilderObj;
}

bool giftools::gifBuilderAddImage(GifBuilder* gifBuilderObj, const Image* imageObj, size_t delay) {
    if (!gifBuilderObj) { return {}; }
    return gifBuilderObj->AddImage(imageObj, delay);
}

giftools::UniqueManagedObj<giftools::Buffer> giftools::gifBuilderFinalize(GifBuilder* gifBuilderObj) {
    if (!gifBuilderObj) { return {}; }
    return gifBuilderObj->End();
}
