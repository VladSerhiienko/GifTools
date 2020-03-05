#include "GifToolsBuffer.h"

template<>
uint8_t giftools::managedType<giftools::Buffer>() { return 2; }

template <>
giftools::Buffer* giftools::managedCast<giftools::Buffer>(ManagedObj* managedObj) {
    if (!managedObj) { return nullptr; }
    if (managedObj->objId().type != managedType<Buffer>()) { return nullptr; }
    return static_cast<Buffer*>(managedObj);
}

giftools::Buffer::Buffer() = default;
giftools::Buffer::~Buffer() = default;

giftools::UniqueManagedObj<giftools::Buffer> giftools::bufferCopyFromVector(const std::vector<uint8_t>& buffer) {
    if (auto bufferObj = managedObjStorageDefault().make<Buffer>()) {
        bufferObj->contents = buffer;
        return bufferObj;
    }
    return {};
}

giftools::UniqueManagedObj<giftools::Buffer> giftools::bufferFromVector(std::vector<uint8_t>&& buffer) {
    if (auto bufferObj = managedObjStorageDefault().make<Buffer>()) {
        bufferObj->contents = std::move(buffer);
        return bufferObj;
    }
    return {};
}

uint8_t* giftools::bufferMutableData(Buffer* bufferObj) {
    assert(bufferObj);
    return bufferObj ? bufferObj->contents.data() : nullptr;
}

const uint8_t* giftools::bufferData(Buffer* bufferObj) {
    assert(bufferObj);
    return bufferObj ? bufferObj->contents.data() : nullptr;
}

size_t giftools::bufferSize(Buffer* bufferObj) {
    assert(bufferObj);
    return bufferObj ? bufferObj->contents.size() : 0;
}

void giftools::bufferResize(Buffer* bufferObj, size_t value) {
    assert(bufferObj);
    if (bufferObj) {bufferObj->contents.resize(value); }
}

void giftools::bufferReserve(Buffer* bufferObj, size_t value) {
    assert(bufferObj);
    if (bufferObj) { bufferObj->contents.reserve(value); }
}

void giftools::bufferFree(Buffer* bufferObj) {
    assert(bufferObj);
    if (bufferObj) { decltype(bufferObj->contents)().swap(bufferObj->contents); }
}
