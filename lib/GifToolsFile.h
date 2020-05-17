#pragma once
#include "GifToolsManagedObject.h"

namespace giftools {

class Buffer;

class File;
template <>
File* managedCast<File>(ManagedObj* managedObj);
template <>
uint8_t managedType<File>();

class File : public ManagedObj {
public:
    ~File() override = default;
protected:
    File() = default;
};

void fileBinaryWrite(const char* path, const uint8_t* bufferPtr, size_t bufferSize);
void fileBinaryWrite(const char* path, const Buffer* bufferObj);

UniqueManagedObj<Buffer> fileBinaryRead(const char* path);

} // namespace giftools
