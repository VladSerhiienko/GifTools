#pragma once
#include "GifToolsManagedObject.h"

namespace giftools {

struct Buffer;

struct File;
template <>
File* managedCast<File>(ManagedObj* managedObj);
template <>
uint8_t managedType<File>();

struct File : public ManagedObj {
    File();
    virtual ~File() override;
};

void fileBinaryWrite(const char* path, const uint8_t* bufferPtr, size_t bufferSize);
void fileBinaryWrite(const char* path, const Buffer* bufferObj);

UniqueManagedObj<Buffer> fileBinaryRead(const char* path);

} // namespace giftools
