#pragma once
#include "GifToolsManagedObject.h"

namespace giftools {

struct Buffer : public ManagedObj {
    Buffer();
    virtual ~Buffer() override;

    std::vector<uint8_t> contents;
};

UniqueManagedObj<Buffer> bufferCopyFromVector(const std::vector<uint8_t>& buffer);
uint8_t* bufferData(Buffer* buffer);
void bufferResize(Buffer* buffer, size_t value);
void bufferReserve(Buffer* buffer, size_t value);
void bufferFree(Buffer* buffer);

template <>
Buffer* managedCast<Buffer>(ManagedObj* managedObj);

template<>
uint8_t managedType<Buffer>();

}

