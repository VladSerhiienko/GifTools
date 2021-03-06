#include "GifToolsFile.h"
#include "GifToolsBuffer.h"
#include "GifToolsManagedTypes.h"

#include <stdio.h>

template <>
uint8_t giftools::managedType<giftools::File>() {
    return static_cast<uint8_t>(giftools::BuiltinManagedType::File);
}

void giftools::fileBinaryWrite(const char* path, const uint8_t* bufferPtr, size_t bufferSize) {
    // GIFTOOLS_LOGT("fileBinaryWrite: contents=%.*s\n", (int)bufferSize, (const char*)bufferPtr);
    
    if (FILE* fileHandle = fopen(path, "wb")) {
        fwrite(bufferPtr, bufferSize, 1, fileHandle);
        fclose(fileHandle);
    }
}

void giftools::fileBinaryWrite(const char* path, const Buffer* bufferObj) {
    return fileBinaryWrite(path, bufferObj->data(), bufferObj->size());
}

giftools::UniqueManagedObj<giftools::Buffer> giftools::fileBinaryRead(const char* path) {
    if (FILE* fileHandle = fopen(path, "rb")) {
        fseek(fileHandle, 0, SEEK_END);
        const size_t fileSize = ftell(fileHandle);
        fseek(fileHandle, 0, SEEK_SET);
        std::vector<uint8_t> fileBuffer;
        fileBuffer.resize(fileSize);
        fread(fileBuffer.data(), fileBuffer.size(), 1, fileHandle);
        fclose(fileHandle);

        auto bufferObj = bufferFromVector(std::move(fileBuffer));
        return bufferObj;
    }

    return {};
}
