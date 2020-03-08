#include "GifToolsWebAssemblyBindings.h"
#include "GifToolsManagedObject.h"
#include "GifToolsImage.h"
#include "GifToolsBuffer.h"


#ifdef GIFTOOLS_EMSCRIPTEN
#warning "GifTools for Emscripten"
#endif

namespace {
void ensureGifToolsInitialized() {
}
}

#ifdef GIFTOOLS_EMSCRIPTEN
#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
using namespace emscripten;
#endif

#ifndef EMSCRIPTEN_KEEPALIVE
#define EMSCRIPTEN_KEEPALIVE
#endif

extern "C" {

//
// Ping
//

EMSCRIPTEN_KEEPALIVE void testPing();
EMSCRIPTEN_KEEPALIVE int testAdd(int a, int b);
EMSCRIPTEN_KEEPALIVE int testDump(const char* bufferPtr, int bufferSize);

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
EMSCRIPTEN_KEEPALIVE void imageFree(int imageId);

//
// GIFs
//

EMSCRIPTEN_KEEPALIVE int gifBuilderInitialize(int width, int height, int delay);
EMSCRIPTEN_KEEPALIVE bool gifBuilderAddImage(int gifBuilderId, int imageId, int delay);
EMSCRIPTEN_KEEPALIVE int gifBuilderFinalize(int gifBuilderId);

//
// Video (TODO)
//

}


void testPing() {
    printf("testPing");
}

int testAdd(int a, int b) {
    return a + b;
}

int testDump(const char* bufferPtr, int bufferSize) {
    int sum = 0;
    for (int i = 0; i < bufferSize; ++i) {
        printf("%i", bufferPtr[i]);
        sum += bufferPtr[i];
    }
    return sum;
}

int bufferCopyFromMemory(const char* bufferPtr, int bufferSize) {
    ensureGifToolsInitialized();
    return giftools::bufferCopyFromMemory((const uint8_t*)bufferPtr, bufferSize).release()->objId().identifier;
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
    return giftools::bufferToStringBase64(bufferObj).release()->objId().identifier;
}

int bufferFromStringBase64(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return giftools::bufferFromStringBase64(bufferObj).release()->objId().identifier;
}


int pixelFormatByteWidth(int format) {
    return giftools::pixelFormatByteWidth(giftools::PixelFormat(format));
}

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
    return giftools::imageClone(imageObj).release()->objId().identifier;
}

int imageLoadFromMemory(const char* bufferPtr, int bufferSize) {
    ensureGifToolsInitialized();
    return giftools::imageLoadFromMemory((const uint8_t*)bufferPtr, bufferSize).release()->objId().identifier;
}

int imageLoadFromBuffer(int bufferId) {
    ensureGifToolsInitialized();
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return giftools::imageLoadFromMemory(bufferObj).release()->objId().identifier;
}

int imageResizeOrClone(int imageId, int width, int height) {
    auto imageObj = giftools::managedObjStorageDefault().get<giftools::Image>(imageId);
    return giftools::imageResizeOrClone(imageObj, width, height).release()->objId().identifier;
}

int imageExportToPNG(int imageId) {
    auto imageObj = giftools::managedObjStorageDefault().get<giftools::Image>(imageId);
    return giftools::imageExportToPNG(imageObj).release()->objId().identifier;
}

void imageFree(int imageId) {
    auto imageObj = giftools::managedObjStorageDefault().get<giftools::Image>(imageId);
    return giftools::imageFree(imageObj);
}

int gifBuilderInitialize(int width, int height, int delay) {
    ensureGifToolsInitialized();
    return giftools::gifBuilderInitialize(width, height, delay).release()->objId().identifier;
}

bool gifBuilderAddImage(int gifBuilderId, int imageId, int delay) {
    auto gifBuilderObj = giftools::managedObjStorageDefault().get<giftools::GifBuilder>(gifBuilderId);
    auto imageObj = giftools::managedObjStorageDefault().get<giftools::Image>(imageId);
    return giftools::gifBuilderAddImage(gifBuilderObj, imageObj, delay);
}

int gifBuilderFinalize(int gifBuilderId) {
    auto gifBuilderObj = giftools::managedObjStorageDefault().get<giftools::GifBuilder>(gifBuilderId);
    return giftools::gifBuilderFinalize(gifBuilderObj).release()->objId().identifier;
}

#ifdef GIFTOOLS_EMSCRIPTEN

int bufferFromTypedArray(const val& arr) {
    std::vector<uint8_t> contents = vecFromJSArray<uint8_t>(arr);
    auto bufferObj = giftools::bufferFromVector(std::move(contents));
    return bufferObj.release()->objId().identifier;
}

val bufferMemoryView(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    auto bufferPtr = giftools::bufferData(bufferObj);
    auto bufferSize = giftools::bufferSize(bufferObj);
    return val(typed_memory_view(bufferSize, bufferPtr));
}

EMSCRIPTEN_BINDINGS(GifToolsBindings) {
    function("bufferFromTypedArray", &bufferFromTypedArray);
    function("bufferMemoryView", &bufferMemoryView);
}

#endif
