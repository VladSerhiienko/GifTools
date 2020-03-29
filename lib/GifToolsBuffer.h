#pragma once
#include "GifToolsManagedObject.h"

namespace giftools {

struct Buffer : public ManagedObj {};
template <>
uint8_t managedType<Buffer>();

UniqueManagedObj<Buffer> bufferCopyFromMemory(const uint8_t* bufferPtr, size_t bufferSize);
UniqueManagedObj<Buffer> bufferCopyFromVector(const std::vector<uint8_t>& buffer);
UniqueManagedObj<Buffer> bufferFromVector(std::vector<uint8_t>&& buffer);
uint8_t* bufferMutableData(Buffer* bufferObj);
const uint8_t* bufferData(const Buffer* bufferObj);
size_t bufferSize(const Buffer* bufferObj);
void bufferResize(Buffer* bufferObj, size_t value);
void bufferReserve(Buffer* bufferObj, size_t value);
void bufferFree(Buffer* bufferObj);
bool bufferZeroTerminated(const Buffer* bufferObj);
bool bufferEmpty(const Buffer* bufferObj);

UniqueManagedObj<Buffer> bufferToStringBase64(const Buffer* bufferObj);
UniqueManagedObj<Buffer> bufferFromStringBase64(const Buffer* bufferObj);

} // namespace giftools
