#include "GifToolsImage.h"
#include "GifToolsBuffer.h"

#define STB_IMAGE_IMPLEMENTATION 1
#define STB_IMAGE_WRITE_IMPLEMENTATION 1
#define STB_IMAGE_RESIZE_IMPLEMENTATION 1
#include "stb_image.h"
#include "stb_image_resize.h"
#include "stb_image_write.h"
#include "gif.h"
#include "gifenc.h"

template<>
uint8_t giftools::managedType<giftools::Image>() { return 1; }

struct ConcreteImage;

template<>
uint8_t giftools::managedType<ConcreteImage>() { return 1; }

size_t giftools::pixelFormatByteWidth(giftools::PixelFormat format) {
    switch (format) {
        case giftools::PixelFormatR8G8B8Unorm: return 3;
        case giftools::PixelFormatR8G8B8A8Unorm: return 4;
        default: return 0;
    }
}

using ImageStoragePtr = std::unique_ptr<uint8_t, void(*)(void*)>;
enum ImageStorageType { ImageStorageTypePtr, ImageStorageTypeVector };
struct ConcreteImageStorage {
    ConcreteImageStorage() {}
    ~ConcreteImageStorage() {}
    ImageStorageType storageType;
    union {
        ImageStoragePtr ptrStorage;
        std::vector<uint8_t> vectorStorage;
    };
};

void imageStorageInit(ConcreteImageStorage* image, ImageStorageType storageType, void(*freeFn)(void*) = nullptr) {
    switch (storageType) {
        case ImageStorageTypePtr: {
            image->storageType = ImageStorageTypePtr;
            new (&image->ptrStorage)ImageStoragePtr(0, freeFn);
        } break;
        default: {
            assert(storageType == ImageStorageTypeVector);
            image->storageType = ImageStorageTypeVector;
            new (&image->vectorStorage)std::vector<uint8_t>{};
        } break;
    }
}

void imageStorageFinalize(ConcreteImageStorage* image) {
    switch (image->storageType) {
    case ImageStorageTypePtr:
        image->ptrStorage.reset();
        image->ptrStorage.~unique_ptr();
        break;

    default:
        assert(image->storageType == ImageStorageTypeVector);
        std::vector<uint8_t>().swap(image->vectorStorage);
        image->vectorStorage.~vector();
        break;
    }
}

void imageStorageReinit(ConcreteImageStorage* image, ImageStorageType storageType, void(*freeFn)(void*) = nullptr) {
    if (image->storageType == storageType) { return; }
    imageStorageFinalize(image);
    imageStorageInit(image, storageType, freeFn);
}

struct ConcreteImage : public giftools::Image {
    ConcreteImage() { imageStorageInit(&internals.storage, ImageStorageTypePtr); }
    ~ConcreteImage() override  { imageStorageFinalize(&internals.storage); }

    giftools::PixelFormat format() const override { return internals.format; }
    size_t width() const override { return internals.width; }
    size_t height() const override { return internals.height; }
    size_t alignment() const override { return internals.alignment; }
    const uint8_t* bufferPtr() const override { return internals.bufferPtr; }
    uint8_t* mutableBufferPtr() override { return internals.bufferPtr; }
    size_t bufferSize() const override { return internals.bufferSize; }
    
    void acquire(uint8_t* bufferPtr, size_t bufferSize, void(*freeFn)(void*) ) {
        imageStorageReinit(&internals.storage, ImageStorageTypePtr, freeFn);
        internals.bufferPtr = bufferPtr;
        internals.bufferSize = bufferSize;
    }
    
    void alloc(size_t bufferSize) {
        imageStorageReinit(&internals.storage, ImageStorageTypeVector);
        internals.storage.vectorStorage.resize(bufferSize);
        internals.bufferPtr = internals.storage.vectorStorage.data();
        internals.bufferSize = bufferSize;
    }

    struct {
        giftools::PixelFormat format = giftools::PixelFormatUndefined;
        size_t width = 0;
        size_t height = 0;
        size_t alignment = 1;
        uint8_t* bufferPtr = nullptr;
        size_t bufferSize = 0;
        ConcreteImageStorage storage = {};
    } internals;
};

size_t giftools::imageWidth(const giftools::Image* imageObj) { return imageObj ? imageObj->width() : 0; }
size_t giftools::imageHeight(const giftools::Image* imageObj) { return imageObj ? imageObj->height() : 0; }
size_t giftools::imageFormat(const giftools::Image* imageObj) { return imageObj ? imageObj->format() : 0; }

giftools::UniqueManagedObj<giftools::Image>
giftools::imageClone(const Image* imageObj) {
    if (!imageObj) { return {}; }
    auto clonedImageObj = managedObjStorageDefault().make<ConcreteImage>();
    clonedImageObj->alloc(imageObj->bufferSize());
    clonedImageObj->internals.width = imageObj->width();
    clonedImageObj->internals.height = imageObj->height();
    clonedImageObj->internals.format = imageObj->format();
    std::memcpy(clonedImageObj->mutableBufferPtr(), imageObj->bufferPtr(), imageObj->bufferSize());
    return clonedImageObj;
}

giftools::UniqueManagedObj<giftools::Image>
imageResize(const giftools::Image* imageObj, size_t width, size_t height) {
    using namespace giftools;
    
    const size_t pixelWidth = pixelFormatByteWidth(imageObj->format());
    const size_t imgStride = imageObj->width() * pixelWidth;
    const size_t resizedImgStride = width * pixelWidth;
    const size_t resizedImgBufferSize = width * height * pixelWidth;
    
    auto resizedImageObj = managedObjStorageDefault().make<ConcreteImage>();
    resizedImageObj->alloc(resizedImgBufferSize);
    resizedImageObj->internals.width = width;
    resizedImageObj->internals.height = height;
    resizedImageObj->internals.format = imageObj->format();
    stbir_resize_uint8(imageObj->bufferPtr(),
                       imageObj->width(),
                       imageObj->height(),
                       imgStride,
                       resizedImageObj->mutableBufferPtr(),
                       resizedImageObj->width(),
                       resizedImageObj->height(),
                       resizedImgStride,
                       pixelWidth);
    return resizedImageObj;
}

giftools::UniqueManagedObj<giftools::Image>
giftools::imageResizeOrClone(const Image* imageObj, size_t width, size_t height) {
    if (!imageObj) { return {}; }
    if (!width || !height) { return imageClone(imageObj); }
    if (imageObj->width() == width && imageObj->height() == height) { return imageClone(imageObj); }
    return imageResize(imageObj, width, height);
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
    return imageLoadFromMemory(bufferData(bufferObj), bufferSize(bufferObj));
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

    auto imageObj = managedObjStorageDefault().make<ConcreteImage>();
    imageObj->acquire(imgBuffer, imgBufferSize, stbi_image_free);
    imageObj->internals.width = x;
    imageObj->internals.height = y;
    imageObj->internals.format = PixelFormatR8G8B8A8Unorm;
    return imageObj;
}

giftools::UniqueManagedObj<giftools::Buffer>
giftools::imageExportToPNG(const Image* imageObj) {
    if (!imageObj) { return {}; }
    
    const size_t pixelWidth = pixelFormatByteWidth(imageObj->format());
    const size_t stride = imageObj->width() * pixelWidth;
    assert(pixelWidth && stride && imageObj->height());
    
    StbiWriterContext writer = {};
    stbi_write_png_to_func(StbiWriterFn, &writer, imageObj->width(), imageObj->height(), pixelWidth, imageObj->bufferPtr(), stride);

    auto bufferObj = bufferFromVector(std::move(writer.contents));
    return bufferObj;
}

template<> uint8_t giftools::managedType<giftools::GifBuilder>() { return 4; }

struct GifBuilderGIFH;
template<> uint8_t giftools::managedType<GifBuilderGIFH>() { return managedType<giftools::GifBuilder>() ; }

struct GifBuilderGIFH : public giftools::GifBuilder {
    ~GifBuilderGIFH() override = default;
    
    
    GifWriter writer = {};
    GifVectorFileBuffer vectorBuffer = {};
    
    GifBuilderGIFH() {
        writer.fileBuffer = &vectorBuffer;
    }
    
    bool Begin(size_t width, size_t height, size_t delay) override {
        return GifBegin(&writer, "", width, height, delay);
    }
    bool AddImage(const giftools::Image* imageObj, size_t delay) override {
        return GifWriteFrame(&writer, imageObj->bufferPtr(), imageObj->width(), imageObj->height(), delay);
    }
    giftools::UniqueManagedObj<giftools::Buffer> End() override {
        if (!GifEnd(&writer)) { return {}; }
        return giftools::bufferFromVector(std::move(vectorBuffer.contents));
    }
};

giftools::UniqueManagedObj<giftools::GifBuilder> giftools::gifBuilderInitialize(size_t width, size_t height, size_t delay) {
    auto gifBuilderObj = managedObjStorageDefault().make<GifBuilderGIFH>();
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
