#include "GifToolsWebAssemblyBindings.h"
#include "GifTools.h"

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
EMSCRIPTEN_KEEPALIVE int aimageLoadFromFileBuffer(const char* bufferPtr, int bufferSize);
EMSCRIPTEN_KEEPALIVE int imageLoadFromBuffer(int bufferId);
EMSCRIPTEN_KEEPALIVE int imageResizeOrClone(int imageId, int width, int height);
EMSCRIPTEN_KEEPALIVE int imageExportToPngFileMemory(int imageId);

//
// GIFs
//

EMSCRIPTEN_KEEPALIVE int gifBuilderInitialize(int width, int height, int delay);
EMSCRIPTEN_KEEPALIVE bool gifBuilderAddImage(int gifBuilderId, int imageId, int delay);
EMSCRIPTEN_KEEPALIVE int gifBuilderFinalize(int gifBuilderId);

//
// Video
//

EMSCRIPTEN_KEEPALIVE int ffmpegInputStreamLoadFromBuffer(int bufferId);
EMSCRIPTEN_KEEPALIVE int ffmpegVideoStreamOpen(int ffmpegInputStreamId);
EMSCRIPTEN_KEEPALIVE int ffmpegVideoStreamPickBestFrame(int ffmpegVideoStreamId, double sampleTime);
EMSCRIPTEN_KEEPALIVE double ffmpegVideoFrameTimeSeconds(int ffmpegVideoFrameId);
EMSCRIPTEN_KEEPALIVE int ffmpegVideoFrameImage(int ffmpegVideoFrameId);
EMSCRIPTEN_KEEPALIVE void ffmpegVideoStreamClose(int ffmpegVideoStreamId);

}

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

int aimageLoadFromFileBuffer(const char* bufferPtr, int bufferSize) {
    return objectReleaseAndReturnId(giftools::aimageLoadFromFileBuffer((const uint8_t*)bufferPtr, bufferSize));
}

int imageLoadFromBuffer(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return objectReleaseAndReturnId(giftools::aimageLoadFromFileBuffer(bufferObj));
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

int ffmpegInputStreamLoadFromBuffer(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    return objectReleaseAndReturnId(giftools::ffmpegInputStreamLoadFromBuffer(bufferObj));
}

int ffmpegVideoStreamOpen(int ffmpegInputStreamId) {
    auto ffmpegInputStreamObj = giftools::managedObjStorageDefault().get<giftools::FFmpegInputStream>(ffmpegInputStreamId);
    return objectReleaseAndReturnId(giftools::ffmpegVideoStreamOpen(ffmpegInputStreamObj));
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

#ifdef GIFTOOLS_EMSCRIPTEN

int bufferFromUint8Array(const val& arr) {
    std::vector<uint8_t> contents = vecFromJSArray<uint8_t>(arr);
    return objectReleaseAndReturnId(giftools::bufferFromVector(std::move(contents)));
}

val bufferToUint8Array(int bufferId) {
    auto bufferObj = giftools::managedObjStorageDefault().get<giftools::Buffer>(bufferId);
    auto bufferPtr = bufferObj->data();
    auto bufferSize = bufferObj->size();
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
    function("aimageLoadFromFileBuffer", &aimageLoadFromFileBuffer, allow_raw_pointers());
    function("imageLoadFromBuffer", &imageLoadFromBuffer);
    function("imageResizeOrClone", &imageResizeOrClone);
    function("imageExportToPngFileMemory", &imageExportToPngFileMemory);
    function("gifBuilderInitialize ", &gifBuilderInitialize);
    function("gifBuilderAddImage", &gifBuilderAddImage);
    function("gifBuilderFinalize", &gifBuilderFinalize);
    function("bufferFromUint8Array", &bufferFromUint8Array);
    function("bufferToUint8Array", &bufferToUint8Array);
    function("ffmpegInputStreamLoadFromBuffer", &ffmpegInputStreamLoadFromBuffer);
    function("ffmpegVideoStreamOpen", &ffmpegVideoStreamOpen);
    function("ffmpegVideoStreamPickBestFrame", &ffmpegVideoStreamPickBestFrame);
    function("ffmpegVideoFrameTimeSeconds", &ffmpegVideoFrameTimeSeconds);
    function("ffmpegVideoFrameImage", &ffmpegVideoFrameImage);
    function("ffmpegVideoStreamClose", &ffmpegVideoStreamClose);
}

#endif
