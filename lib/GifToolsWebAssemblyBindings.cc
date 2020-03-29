#include "GifToolsWebAssemblyBindings.h"

#include "GifToolsBuffer.h"
#include "GifToolsImage.h"
#include "GifToolsManagedObject.h"

#ifdef GIFTOOLS_EMSCRIPTEN
#warning "GifTools for Emscripten"
#endif

#ifdef GIFTOOLS_EMSCRIPTEN
#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include <emscripten/val.h>
using namespace emscripten;
#endif

#ifndef EMSCRIPTEN_KEEPALIVE
#define EMSCRIPTEN_KEEPALIVE
#endif

extern "C" {

//
// Object
//

EMSCRIPTEN_KEEPALIVE void objectFree(int objectId);

//
// Buffers
//

EMSCRIPTEN_KEEPALIVE int bufferCopyFromMemory(const char* bufferPtr, int bufferSize);
EMSCRIPTEN_KEEPALIVE uint8_t* bufferMutableData(int bufferId);
EMSCRIPTEN_KEEPALIVE const uint8_t* bufferData(int bufferId);
EMSCRIPTEN_KEEPALIVE int bufferSize(int bufferId);
EMSCRIPTEN_KEEPALIVE void bufferResize(int bufferId, int value);
EMSCRIPTEN_KEEPALIVE void bufferReserve(int bufferId, int value);
EMSCRIPTEN_KEEPALIVE void bufferFree(int bufferId);
EMSCRIPTEN_KEEPALIVE bool bufferZeroTerminated(int bufferId);
EMSCRIPTEN_KEEPALIVE bool bufferEmpty(int bufferId);
EMSCRIPTEN_KEEPALIVE int bufferToStringBase64(int bufferId);
EMSCRIPTEN_KEEPALIVE int bufferFromStringBase64(int bufferId);

//
// Images
//

EMSCRIPTEN_KEEPALIVE int pixelFormatByteWidth(int format);
EMSCRIPTEN_KEEPALIVE int imageWidth(int imageId);
EMSCRIPTEN_KEEPALIVE int imageHeight(int imageId);
EMSCRIPTEN_KEEPALIVE int imageFormat(int imageId);
EMSCRIPTEN_KEEPALIVE int imageClone(int imageId);
EMSCRIPTEN_KEEPALIVE int imageLoadFromMemory(const char* bufferPtr, int bufferSize);
EMSCRIPTEN_KEEPALIVE int imageLoadFromBuffer(int bufferId);
EMSCRIPTEN_KEEPALIVE int imageResizeOrClone(int imageId, int width, int height);
EMSCRIPTEN_KEEPALIVE int imageExportToPNG(int imageId);

//
// GIFs
//

EMSCRIPTEN_KEEPALIVE int gifBuilderInitialize(int width, int height, int delay);
EMSCRIPTEN_KEEPALIVE bool gifBuilderAddImage(int gifBuilderId, int imageId, int delay);
EMSCRIPTEN_KEEPALIVE int gifBuilderFinalize(int gifBuilderId);

//
// Video
// TODO(vserhiienko)
//
}

namespace {
template <typename T>
int objectReleaseAndReturnId(giftools::UniqueManagedObj<T>&& object) {
    return object.release()->objId().composite;
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
    return giftools::bufferMutableData(bufferObj);
}

const uint8_t* bufferData(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return giftools::bufferData(bufferObj);
}

int bufferSize(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return giftools::bufferSize(bufferObj);
}

void bufferResize(int bufferId, int value) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return giftools::bufferResize(bufferObj, value);
}

void bufferReserve(int bufferId, int value) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return giftools::bufferReserve(bufferObj, value);
}

void bufferFree(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return giftools::bufferFree(bufferObj);
}

bool bufferZeroTerminated(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return giftools::bufferZeroTerminated(bufferObj);
}

bool bufferEmpty(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return giftools::bufferEmpty(bufferObj);
}

int bufferToStringBase64(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return objectReleaseAndReturnId(giftools::bufferToStringBase64(bufferObj));
}

int bufferFromStringBase64(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return objectReleaseAndReturnId(giftools::bufferFromStringBase64(bufferObj));
}

int pixelFormatByteWidth(int format) { return giftools::pixelFormatByteWidth(giftools::PixelFormat(format)); }

int imageWidth(int imageId) {
    auto imageObj = giftools::managedObjStorageDefault().get<giftools::Image>(imageId);
    return giftools::imageWidth(imageObj);
}

int imageHeight(int imageId) {
    auto imageObj = giftools::managedObjStorageDefault().get<giftools::Image>(imageId);
    return giftools::imageHeight(imageObj);
}

int imageFormat(int imageId) {
    auto imageObj = giftools::managedObjStorageDefault().get<giftools::Image>(imageId);
    return giftools::imageFormat(imageObj);
}

int imageClone(int imageId) {
    auto imageObj = giftools::managedObjStorageDefault().get<giftools::Image>(imageId);
    return objectReleaseAndReturnId(giftools::imageClone(imageObj));
}

int imageLoadFromMemory(const char* bufferPtr, int bufferSize) {
    return objectReleaseAndReturnId(giftools::imageLoadFromMemory((const uint8_t*)bufferPtr, bufferSize));
}

int imageLoadFromBuffer(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return objectReleaseAndReturnId(giftools::imageLoadFromMemory(bufferObj));
}

int imageResizeOrClone(int imageId, int width, int height) {
    auto imageObj = giftools::managedObjStorageDefault().get<giftools::Image>(imageId);
    return objectReleaseAndReturnId(giftools::imageResizeOrClone(imageObj, width, height));
}

int imageExportToPNG(int imageId) {
    auto imageObj = giftools::managedObjStorageDefault().get<giftools::Image>(imageId);
    return objectReleaseAndReturnId(giftools::imageExportToPNG(imageObj));
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

#ifdef GIFTOOLS_EMSCRIPTEN

int bufferFromUint8Array(const val& arr) {
    std::vector<uint8_t> contents = vecFromJSArray<uint8_t>(arr);
    return objectReleaseAndReturnId(giftools::bufferFromVector(std::move(contents)));
}

val bufferToUint8Array(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    auto bufferPtr = giftools::bufferData(bufferObj);
    auto bufferSize = giftools::bufferSize(bufferObj);
    return val(typed_memory_view(bufferSize, bufferPtr));
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
    function("pixelFormatByteWidth", &pixelFormatByteWidth);
    function("imageWidth", &imageWidth);
    function("imageHeight", &imageHeight);
    function("imageFormat", &imageFormat);
    function("imageClone", &imageClone);
    function("imageLoadFromMemory", &imageLoadFromMemory, allow_raw_pointers());
    function("imageLoadFromBuffer", &imageLoadFromBuffer);
    function("imageResizeOrClone", &imageResizeOrClone);
    function("imageExportToPNG", &imageExportToPNG);
    function("gifBuilderInitialize ", &gifBuilderInitialize);
    function("gifBuilderAddImage", &gifBuilderAddImage);
    function("gifBuilderFinalize", &gifBuilderFinalize);
    function("bufferFromUint8Array", &bufferFromUint8Array);
    function("bufferToUint8Array", &bufferToUint8Array);
}

#endif
