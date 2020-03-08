#include "GifToolsBuffer.h"
#include "base64.h"

template<>
uint8_t giftools::managedType<giftools::Buffer>() { return 2; }


struct ConcreteBuffer;
template<>
uint8_t giftools::managedType<ConcreteBuffer>() { return 2; }

struct ConcreteBuffer : public giftools::Buffer {
    ConcreteBuffer() = default;
    virtual ~ConcreteBuffer() override = default;
    std::vector<uint8_t> contents;
};

giftools::UniqueManagedObj<giftools::Buffer>
giftools::bufferCopyFromMemory(const uint8_t *bufferPtr, size_t bufferSize) {
    if (!bufferPtr || !bufferSize) { return {}; }
    
    auto bufferObj = managedObjStorageDefault().make<ConcreteBuffer>();
    if (!bufferObj) { return {}; }

    bufferObj->contents.resize(bufferSize);
    std::memcpy(bufferObj->contents.data(), bufferPtr, bufferSize);
    return bufferObj;
}

giftools::UniqueManagedObj<giftools::Buffer>
giftools::bufferCopyFromVector(const std::vector<uint8_t>& buffer) {
    if (buffer.empty()) { return {}; }
    return bufferCopyFromMemory(buffer.data(), buffer.size());
}

giftools::UniqueManagedObj<giftools::Buffer>
giftools::bufferFromVector(std::vector<uint8_t>&& buffer) {
    if (buffer.empty()) { return {}; }
    
    auto bufferObj = managedObjStorageDefault().make<ConcreteBuffer>();
    if (!bufferObj) { return {}; }

    bufferObj->contents = std::move(buffer);
    return bufferObj;
}

uint8_t* giftools::bufferMutableData(Buffer* bufferHandle) {
    auto bufferObj = static_cast<ConcreteBuffer*>(bufferHandle);
    return bufferObj ? bufferObj->contents.data() : nullptr;
}

const uint8_t* giftools::bufferData(const Buffer* bufferHandle) {
    auto bufferObj = static_cast<const ConcreteBuffer*>(bufferHandle);
    return bufferObj ? bufferObj->contents.data() : nullptr;
}

size_t giftools::bufferSize(const Buffer* bufferHandle) {
    auto bufferObj = static_cast<const ConcreteBuffer*>(bufferHandle);
    return bufferObj ? bufferObj->contents.size() : 0;
}

void giftools::bufferResize(Buffer* bufferHandle, size_t value) {
    auto bufferObj = static_cast<ConcreteBuffer*>(bufferHandle);
    if (bufferObj && bufferObj->contents.size() < value) { bufferObj->contents.resize(value); }
}

void giftools::bufferReserve(Buffer* bufferHandle, size_t value) {
    auto bufferObj = static_cast<ConcreteBuffer*>(bufferHandle);
    if (bufferObj && bufferObj->contents.capacity() < value) { bufferObj->contents.reserve(value); }
}

void giftools::bufferFree(Buffer* bufferHandle) {
    auto bufferObj = static_cast<ConcreteBuffer*>(bufferHandle);
    if (!bufferEmpty(bufferObj)) { decltype(bufferObj->contents)().swap(bufferObj->contents); }
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
