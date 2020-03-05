#pragma once
#include "GifToolsManagedObject.h"

namespace giftools {

struct Buffer;
template <> Buffer* managedCast<Buffer>(ManagedObj* managedObj);
template<> uint8_t managedType<Buffer>();

struct Buffer : public ManagedObj {
    Buffer();
    virtual ~Buffer() override;

    std::vector<uint8_t> contents;
};

UniqueManagedObj<Buffer> bufferCopyFromVector(const std::vector<uint8_t>& buffer);
UniqueManagedObj<Buffer> bufferFromVector(std::vector<uint8_t>&& buffer);
uint8_t* bufferMutableData(Buffer* bufferObj);
const uint8_t* bufferData(Buffer* bufferObj);
size_t bufferSize(Buffer* bufferObj);
void bufferResize(Buffer* bufferObj, size_t value);
void bufferReserve(Buffer* bufferObj, size_t value);
void bufferFree(Buffer* bufferObj);

}

