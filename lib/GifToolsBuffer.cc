#include "GifToolsBuffer.h"

template<>
uint8_t giftools::managedType<giftools::Buffer>() { return 2; }


template <>
giftools::Buffer* giftools::managedCast<giftools::Buffer>(ManagedObj* managedObj) {
    if (!managedObj) { return nullptr; }
    if (managedObj->objId().type != managedType<Buffer>()) { return nullptr; }
    return static_cast<Buffer*>(managedObj);
}

giftools::UniqueManagedObj<giftools::Buffer> giftools::bufferCopyFromVector(const std::vector<uint8_t>& buffer) {
    return nullptr;
}

uint8_t* giftools::bufferData(Buffer* buffer);
void giftools::bufferResize(Buffer* buffer, size_t value);
void giftools::bufferReserve(Buffer* buffer, size_t value);
void giftools::bufferFree(Buffer* buffer);
