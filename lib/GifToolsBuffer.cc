#include "GifToolsBuffer.h"
#include "GifToolsManagedTypes.h"
#include "base64.h"

#include <stdio.h>

struct ConcreteBuffer;

template <>
uint8_t giftools::managedType<giftools::Buffer>() {
    return static_cast<uint8_t>(giftools::BuiltinManagedType::Buffer);
}

template <>
uint8_t giftools::managedType<ConcreteBuffer>() {
    return static_cast<uint8_t>(giftools::BuiltinManagedType::Buffer);
}

struct ConcreteBuffer : public giftools::Buffer {
    std::vector<uint8_t> contents;
    
    ConcreteBuffer() = default;
    virtual ~ConcreteBuffer() override { contents = {}; }
    
    uint8_t* mutableData() override { return contents.data(); }
    const uint8_t* data() const override { return contents.data(); }
    size_t size() const override { return contents.size(); }
    void resize(size_t value) override { contents.resize(value); }
    void reserve(size_t value) override { contents.reserve(value); }
    void wipe() override { decltype(contents)().swap(contents); }
    bool zeroTerminated() const override { return contents.back() == 0; }
    bool empty() const override { return contents.empty(); }
    
    void initFrom(std::vector<uint8_t>&& data) override { contents = std::move(data); }
    std::vector<uint8_t> copyToByteVector(const Buffer* bufferObj) const override { return contents; }
};

giftools::UniqueManagedObj<giftools::Buffer> giftools::bufferCopyFromMemory(const uint8_t* bufferPtr, size_t bufferSize) {
    if (!bufferPtr || !bufferSize) { return {}; }

    auto bufferObj = managedObjStorageDefault().make<ConcreteBuffer>();
    if (!bufferObj) { return {}; }

    bufferObj->contents.resize(bufferSize);
    std::memcpy(bufferObj->contents.data(), bufferPtr, bufferSize);
    return bufferObj;
}

giftools::UniqueManagedObj<giftools::Buffer> giftools::bufferCopyFromVector(const std::vector<uint8_t>& buffer) {
    if (buffer.empty()) { return {}; }
    return bufferCopyFromMemory(buffer.data(), buffer.size());
}

giftools::UniqueManagedObj<giftools::Buffer> giftools::bufferFromVector(std::vector<uint8_t>&& buffer) {
    if (buffer.empty()) { return {}; }

    auto bufferObj = managedObjStorageDefault().make<ConcreteBuffer>();
    if (!bufferObj) { return {}; }

    bufferObj->contents = std::move(buffer);
    
    // GIFTOOLS_LOGT("bufferFromVector: contents=%.*s\n", (int)bufferObj->contents.size(), (const char*)bufferObj->contents.data());
    return bufferObj;
}

giftools::UniqueManagedObj<giftools::Buffer> giftools::bufferToStringBase64(const Buffer* bufferObj) {
    if (bufferObj->empty()) { return {}; }

    auto encodedBuffer = base64_encode(bufferObj->data(), bufferObj->size());
    if (encodedBuffer.empty()) { return {}; }

    auto encodedBufferObj = bufferFromVector(std::move(encodedBuffer));
    assert(encodedBufferObj->zeroTerminated());
    return encodedBufferObj;
}

giftools::UniqueManagedObj<giftools::Buffer> giftools::bufferFromStringBase64(const Buffer* bufferObj) {
    if (bufferObj->empty()) { return {}; }
    assert(bufferObj->zeroTerminated());

    std::vector<uint8_t> decodedBuffer = base64_decode(bufferObj->data(), bufferObj->size() - 1);
    if (decodedBuffer.empty()) { return {}; }

    auto decodedBufferObj = bufferFromVector(std::move(decodedBuffer));
    return decodedBufferObj;
}

giftools::UniqueManagedObj<giftools::Buffer> giftools::bufferWithSize(size_t size) {
    if (!size) { return {}; }
    
    auto bufferObj = managedObjStorageDefault().make<ConcreteBuffer>();
    bufferObj->resize(size);
    return bufferObj;
}
