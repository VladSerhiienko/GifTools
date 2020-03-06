#include "GifToolsBuffer.h"
#include "base64.h"

template<>
uint8_t giftools::managedType<giftools::Buffer>() { return 2; }

giftools::Buffer::Buffer() = default;
giftools::Buffer::~Buffer() = default;

giftools::UniqueManagedObj<giftools::Buffer>
giftools::bufferCopyFromVector(const std::vector<uint8_t>& buffer) {
    if (buffer.empty()) { return {}; }
    
    auto bufferObj = managedObjStorageDefault().make<Buffer>();
    if (!bufferObj) { return {}; }

    bufferObj->contents = buffer;
    return bufferObj;
}

giftools::UniqueManagedObj<giftools::Buffer>
giftools::bufferFromVector(std::vector<uint8_t>&& buffer) {
    if (buffer.empty()) { return {}; }
    
    auto bufferObj = managedObjStorageDefault().make<Buffer>();
    if (!bufferObj) { return {}; }

    bufferObj->contents = std::move(buffer);
    return bufferObj;
}

uint8_t* giftools::bufferMutableData(Buffer* bufferObj) {
    return bufferObj ? bufferObj->contents.data() : nullptr;
}

const uint8_t* giftools::bufferData(const Buffer* bufferObj) {
    return bufferObj ? bufferObj->contents.data() : nullptr;
}

size_t giftools::bufferSize(const Buffer* bufferObj) {
    return bufferObj ? bufferObj->contents.size() : 0;
}

void giftools::bufferResize(Buffer* bufferObj, size_t value) {
    if (bufferObj) { bufferObj->contents.resize(value); }
}

void giftools::bufferReserve(Buffer* bufferObj, size_t value) {
    if (bufferObj) { bufferObj->contents.reserve(value); }
}

void giftools::bufferFree(Buffer* bufferObj) {
    if (bufferObj) { decltype(bufferObj->contents)().swap(bufferObj->contents); }
}

bool giftools::bufferEmpty(const giftools::Buffer* bufferObj) {
    return bufferObj ? (bufferSize(bufferObj) == 0) : true;
}

bool giftools::bufferZeroTerminated(const giftools::Buffer* bufferObj) {
    if (bufferEmpty(bufferObj)) { return false; }
    return bufferData(bufferObj)[bufferSize(bufferObj) - 1] == '\0';
}

giftools::UniqueManagedObj<giftools::Buffer>
giftools::bufferToStringBase64(const Buffer* bufferObj) {
    if (bufferEmpty(bufferObj)) { return {}; }

    auto encodedBuffer = base64_encode(bufferData(bufferObj), bufferSize(bufferObj));
    if (encodedBuffer.empty()) { return {}; }
    
    auto encodedBufferObj = bufferFromVector(std::move(encodedBuffer));
    assert(bufferZeroTerminated(encodedBufferObj.get()));
    return encodedBufferObj;
}

giftools::UniqueManagedObj<giftools::Buffer>
giftools::bufferFromStringBase64(const Buffer* bufferObj) {
    if (bufferEmpty(bufferObj)) { return {}; }
    assert(bufferZeroTerminated(bufferObj));
    
    std::vector<uint8_t> decodedBuffer = base64_decode(bufferData(bufferObj), bufferSize(bufferObj) - 1);
    if (decodedBuffer.empty()) { return {}; }
    
    auto decodedBufferObj = bufferFromVector(std::move(decodedBuffer));
    return decodedBufferObj;
}
