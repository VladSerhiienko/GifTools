#include "GifToolsWebAssemblyBindings.h"
#include "GifTools.h"

#include "base64.h"

//#define GIFTOOLS_EMSCRIPTEN 1

#ifdef GIFTOOLS_EMSCRIPTEN
#warning "GifTools for Emscripten"
#endif

#ifdef GIFTOOLS_USE_FFMPEG
#warning "GifTools with FFmpeg"
#endif

#ifdef GIFTOOLS_EMSCRIPTEN
#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include <emscripten/val.h>
using namespace emscripten;
#endif

namespace {
int32_t objectId(const giftools::ManagedObj* object) {
    return object ? object->objId().composite : 0;
}
template <typename T>
int32_t objectReleaseAndReturnId(giftools::UniqueManagedObj<T>&& object) {
    return object ? object.release()->objId().composite : 0;
}
} // namespace

void objectFree(int objectId) {
    if (auto object = giftools::managedObjStorageDefault().get(objectId)) {
        giftools::managedObjStorageDefault().free(object);
    }
}

int bufferCopyFromMemory(const char* bufferPtr, int bufferSize) {
    return objectReleaseAndReturnId(giftools::bufferCopyFromMemory((const uint8_t*)bufferPtr, bufferSize));
}

uint8_t* bufferMutableData(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return bufferObj ? bufferObj->mutableData() : nullptr;
}

const uint8_t* bufferData(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return bufferObj ? bufferObj->data() : nullptr;
}

int bufferSize(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return bufferObj ? bufferObj->size() : 0;
}

void bufferResize(int bufferId, int value) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    if (value >= 0) { bufferObj->resize(static_cast<size_t>(value)); }
}

void bufferReserve(int bufferId, int value) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    if (value >= 0) { bufferObj->reserve(static_cast<size_t>(value)); }
}

void bufferFree(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    if (bufferObj) { bufferObj->wipe(); }
}

bool bufferZeroTerminated(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return bufferObj ? bufferObj->zeroTerminated() : false;
}

bool bufferEmpty(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return bufferObj ? bufferObj->empty() : false;
}

int bufferToStringBase64(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return objectReleaseAndReturnId(giftools::bufferToStringBase64(bufferObj));
}

int bufferFromStringBase64(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return objectReleaseAndReturnId(giftools::bufferFromStringBase64(bufferObj));
}

int pixelFormatByteWidth(int format) {
    return static_cast<int32_t>(giftools::pixelFormatByteWidth(giftools::PixelFormat(format)));
}

int imageWidth(int imageId) {
    auto imageObj = giftools::managedObjStorageDefault().get<giftools::Image>(imageId);
    return static_cast<int32_t>(imageObj->width());
}

int imageHeight(int imageId) {
    auto imageObj = giftools::managedObjStorageDefault().get<giftools::Image>(imageId);
    return static_cast<int32_t>(imageObj->height());
}

int imageFormat(int imageId) {
    auto imageObj = giftools::managedObjStorageDefault().get<giftools::Image>(imageId);
    return static_cast<int32_t>(imageObj->format());
}

int imageClone(int imageId) {
    auto imageObj = giftools::managedObjStorageDefault().get<giftools::Image>(imageId);
    return objectReleaseAndReturnId(giftools::imageClone(imageObj));
}

int imageLoadFromFileBuffer(const char* bufferPtr, int bufferSize) {
    return objectReleaseAndReturnId(giftools::imageLoadFromFileBuffer((const uint8_t*)bufferPtr, bufferSize));
}

int imageLoadFromBuffer(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return objectReleaseAndReturnId(giftools::imageLoadFromFileBuffer(bufferObj));
}

int imageResizeOrClone(int imageId, int width, int height) {
    auto imageObj = giftools::managedObjStorageDefault().get<giftools::Image>(imageId);
    return objectReleaseAndReturnId(giftools::imageResizeOrClone(imageObj, width, height));
}

int imageExportToPngFileMemory(int imageId) {
    auto imageObj = giftools::managedObjStorageDefault().get<giftools::Image>(imageId);
    return objectReleaseAndReturnId(giftools::imageExportToPngFileMemory(imageObj));
}

int gifBuilderInitialize(int width, int height, int delay) {
    return objectReleaseAndReturnId(giftools::gifBuilderInitialize(width, height, delay));
}

bool gifBuilderAddImage(int gifBuilderId, int imageId, int delay) {
    auto gifBuilderObj = giftools::managedObjStorageDefault().get<giftools::GifBuilder>(gifBuilderId);
    auto imageObj = giftools::managedObjStorageDefault().get<giftools::Image>(imageId);
    return giftools::gifBuilderAddImage(gifBuilderObj, imageObj, delay);
}

int gifBuilderFinalize(int gifBuilderId) {
    auto gifBuilderObj = giftools::managedObjStorageDefault().get<giftools::GifBuilder>(gifBuilderId);
    return objectReleaseAndReturnId(giftools::gifBuilderFinalize(gifBuilderObj));
}


#ifdef GIFTOOLS_USE_FFMPEG

int ffmpegInputStreamLoadFromBuffer(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return objectReleaseAndReturnId(giftools::ffmpegInputStreamLoadFromBuffer(bufferObj));
}

int ffmpegVideoStreamOpen(int ffmpegInputStreamId) {
    auto ffmpegInputStreamObj = giftools::managedObjStorageDefault().get<giftools::FFmpegInputStream>(ffmpegInputStreamId);
    return objectReleaseAndReturnId(giftools::ffmpegVideoStreamOpen(ffmpegInputStreamObj));
}

int ffmpegVideoStreamWidth(int ffmpegVideoStreamId) {
    auto ffmpegVideoStreamObj = giftools::managedObjStorageDefault().get<giftools::FFmpegVideoStream>(ffmpegVideoStreamId);
    return ffmpegVideoStreamObj ? ffmpegVideoStreamObj->frameWidth() : 0;
}

int ffmpegVideoStreamHeight(int ffmpegVideoStreamId) {
    auto ffmpegVideoStreamObj = giftools::managedObjStorageDefault().get<giftools::FFmpegVideoStream>(ffmpegVideoStreamId);
    return ffmpegVideoStreamObj ? ffmpegVideoStreamObj->frameHeight() : 0;
}

double ffmpegVideoStreamDurationSeconds(int ffmpegVideoStreamId) {
    auto ffmpegVideoStreamObj = giftools::managedObjStorageDefault().get<giftools::FFmpegVideoStream>(ffmpegVideoStreamId);
    return ffmpegVideoStreamObj ? ffmpegVideoStreamObj->estimatedTotalDurationSeconds() : 0.0;
}

double ffmpegVideoStreamFrameDurationSeconds(int ffmpegVideoStreamId) {
    auto ffmpegVideoStreamObj = giftools::managedObjStorageDefault().get<giftools::FFmpegVideoStream>(ffmpegVideoStreamId);
    return ffmpegVideoStreamObj ? ffmpegVideoStreamObj->estimatedFrameDurationSeconds() : 0.0;
}

int ffmpegVideoStreamPickBestFrame(int ffmpegVideoStreamId, double sampleTime) {
    auto ffmpegVideoStreamObj = giftools::managedObjStorageDefault().get<giftools::FFmpegVideoStream>(ffmpegVideoStreamId);
    return objectReleaseAndReturnId(giftools::ffmpegVideoStreamPickBestFrame(ffmpegVideoStreamObj, sampleTime));
}

void ffmpegVideoStreamClose(int ffmpegVideoStreamId) {
    auto ffmpegVideoStreamObj = giftools::managedObjStorageDefault().get<giftools::FFmpegVideoStream>(ffmpegVideoStreamId);
    return giftools::ffmpegVideoStreamClose(ffmpegVideoStreamObj);
}

double ffmpegVideoFrameTimeSeconds(int ffmpegVideoFrameId) {
    auto ffmpegVideoFrameObj = giftools::managedObjStorageDefault().get<giftools::FFmpegVideoFrame>(ffmpegVideoFrameId);
    return ffmpegVideoFrameObj ? ffmpegVideoFrameObj->estimatedSampleTimeSeconds() : 0;
}

int ffmpegVideoFrameImage(int ffmpegVideoFrameId) {
    auto ffmpegVideoFrameObj = giftools::managedObjStorageDefault().get<giftools::FFmpegVideoFrame>(ffmpegVideoFrameId);
    return objectId(ffmpegVideoFrameObj->image());
}

#endif

#ifdef GIFTOOLS_EMSCRIPTEN

int bufferFromUint8Array(const val& arr) {
    std::vector<uint8_t> contents = vecFromJSArray<uint8_t>(arr);
    return objectReleaseAndReturnId(giftools::bufferFromVector(std::move(contents)));
}

val bufferToUint8Array(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    auto bufferPtr = bufferObj->data();
    auto bufferSize = bufferObj->size();
    
    // printf("bufferToUint8Array: contents=%.*s\n", (int)bufferSize, (const char*)bufferPtr);
    return val(typed_memory_view(bufferSize, bufferPtr));
}

namespace giftools::bindings {
    struct BufferBindings;
    struct ImageBindings;
}

namespace giftools::bindings {
    struct BufferBindings {
        UniqueManagedObj<Buffer> bufferObj;
        
        BufferBindings() = default;
        ~BufferBindings() = default;
        
        bool ensureInitialized() {
            if (bufferObj) { return true; }
            bufferObj = giftools::managedObjStorageDefault().make<giftools::Buffer>();
            return bufferObj != nullptr;
        }
        
        bool empty() const { return bufferObj ? bufferObj->empty() : true; }
        uint8_t* mutableData() { return bufferObj ? bufferObj->mutableData() : nullptr; };
        const uint8_t* data() const { return bufferObj ? bufferObj->data() : nullptr; };
        size_t size() const { return bufferObj ? bufferObj->size() : 0; };
        void resize(size_t size) { if (bufferObj) { bufferObj->resize(size); }  }
        
        val asUint8ArrayView() const { return empty() ? val::null() : val(typed_memory_view(size(), data())); }
        std::string toStringBase64() const { return empty() ? "" : base64_encode_string(data(), size()); }
        
        bool fromUint8ArrayView(const val& uint8Arr) {
            if (uint8Arr.isNull() || uint8Arr.isUndefined()) { return false; }
            if (!ensureInitialized()) { return false; }
            
            bufferObj->initFrom(vecFromJSArray<uint8_t>(uint8Arr));
            return true;
        }
    };
}

EMSCRIPTEN_BINDINGS(GifToolsBindings) {
    function("objectFree", &objectFree);
    
    function("bufferCopyFromMemory", &bufferCopyFromMemory, allow_raw_pointers());
    function("bufferMutableData", &bufferMutableData, allow_raw_pointers());
    function("bufferData", &bufferData, allow_raw_pointers());
    function("bufferSize", &bufferSize);
    function("bufferResize", &bufferResize);
    function("bufferReserve", &bufferReserve);
    function("bufferFree", &bufferFree);
    function("bufferZeroTerminated", &bufferZeroTerminated);
    function("bufferEmpty", &bufferEmpty);
    function("bufferToStringBase64", &bufferToStringBase64);
    function("bufferFromStringBase64", &bufferFromStringBase64);

    class_<giftools::bindings::BufferBindings>("Buffer")
        .function("mutableData", &giftools::bindings::BufferBindings::mutableData, allow_raw_pointers())
        .function("data", &giftools::bindings::BufferBindings::data, allow_raw_pointers())
        .function("size", &giftools::bindings::BufferBindings::size)
        .function("resize", &giftools::bindings::BufferBindings::resize)
        .function("toStringBase64", &giftools::bindings::BufferBindings::toStringBase64)
        .function("asUint8ArrayView", &giftools::bindings::BufferBindings::asUint8ArrayView)
        .function("fromUint8ArrayView", &giftools::bindings::BufferBindings::fromUint8ArrayView)
        .smart_ptr_constructor("makeBuffer", &std::make_shared<giftools::bindings::BufferBindings>)
        .smart_ptr<std::shared_ptr<giftools::bindings::BufferBindings>>("Buffer");
    
    function("pixelFormatByteWidth", &pixelFormatByteWidth);
    function("imageWidth", &imageWidth);
    function("imageHeight", &imageHeight);
    function("imageFormat", &imageFormat);
    function("imageClone", &imageClone);
    function("imageLoadFromFileBuffer", &imageLoadFromFileBuffer, allow_raw_pointers());
    function("imageLoadFromBuffer", &imageLoadFromBuffer);
    function("imageResizeOrClone", &imageResizeOrClone);
    function("imageExportToPngFileMemory", &imageExportToPngFileMemory);
    
    function("gifBuilderInitialize", &gifBuilderInitialize);
    function("gifBuilderAddImage", &gifBuilderAddImage);
    function("gifBuilderFinalize", &gifBuilderFinalize);
    function("bufferFromUint8Array", &bufferFromUint8Array);
    function("bufferToUint8Array", &bufferToUint8Array);
    
    #ifdef GIFTOOLS_USE_FFMPEG
    function("ffmpegInputStreamLoadFromBuffer", &ffmpegInputStreamLoadFromBuffer);
    function("ffmpegVideoStreamOpen", &ffmpegVideoStreamOpen);
    function("ffmpegVideoStreamWidth", &ffmpegVideoStreamWidth);
    function("ffmpegVideoStreamHeight", &ffmpegVideoStreamHeight);
    function("ffmpegVideoStreamDurationSeconds", &ffmpegVideoStreamDurationSeconds);
    function("ffmpegVideoStreamFrameDurationSeconds", &ffmpegVideoStreamFrameDurationSeconds);
    function("ffmpegVideoStreamPrepareAllFrames", &ffmpegVideoStreamPrepareAllFrames);
    function("ffmpegVideoStreamPrepareFrames", &ffmpegVideoStreamPrepareFrames);
    function("ffmpegVideoStreamPickBestFrame", &ffmpegVideoStreamPickBestFrame);
    function("ffmpegVideoFrameTimeSeconds", &ffmpegVideoFrameTimeSeconds);
    function("ffmpegVideoFrameImage", &ffmpegVideoFrameImage);
    function("ffmpegVideoStreamClose", &ffmpegVideoStreamClose);
    #endif
}

#endif
