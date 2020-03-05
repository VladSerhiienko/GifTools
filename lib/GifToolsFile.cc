#include "GifToolsFile.h"
#include "GifToolsBuffer.h"

template<> uint8_t giftools::managedType<giftools::File>() { return 3; }
template <> giftools::File* giftools::managedCast<giftools::File>(ManagedObj* managedObj) {
    if (!managedObj) { return nullptr; }
    if (managedObj->objId().type != managedType<File>()) { return nullptr; }
    return static_cast<File*>(managedObj);
}

giftools::File::File() = default;
giftools::File::~File() = default;

void giftools::fileBinaryWrite(const char* path, const uint8_t* bufferPtr, size_t bufferSize) {
    if (FILE* fileHandle = fopen(path, "wb")) {
        fwrite(bufferPtr, bufferSize, 1, fileHandle);
        fclose(fileHandle);
    }
}

void giftools::fileBinaryWrite(const char* path, const Buffer* bufferObj) {
    return fileBinaryWrite(path, bufferObj->contents.data(), bufferObj->contents.size());
}

giftools::UniqueManagedObj<giftools::Buffer>
giftools::fileBinaryRead(const char* path) {
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
