#pragma once
#include "GifToolsManagedObject.h"

namespace giftools {

class Buffer;
template <>
uint8_t managedType<Buffer>();

class Buffer : public ManagedObj {
public:
    ~Buffer() override = default;
    
    virtual uint8_t* mutableData() = 0;
    virtual const uint8_t* data() const = 0;
    virtual size_t size() const = 0;
    virtual void resize(size_t value) = 0;
    virtual void reserve(size_t value) = 0;
    virtual void wipe() = 0;
    virtual bool zeroTerminated() const = 0;
    virtual bool empty() const = 0;
    
    virtual void initFrom(std::vector<uint8_t>&& data) = 0;
    virtual std::vector<uint8_t> copyToByteVector(const Buffer* bufferObj) const = 0;
    
protected:
    Buffer() = default;
};

UniqueManagedObj<Buffer> bufferCopyFromMemory(const uint8_t* bufferPtr, size_t bufferSize);
UniqueManagedObj<Buffer> bufferCopyFromVector(const std::vector<uint8_t>& buffer);
UniqueManagedObj<Buffer> bufferToStringBase64(const Buffer* bufferObj);
UniqueManagedObj<Buffer> bufferFromVector(std::vector<uint8_t>&& buffer);
UniqueManagedObj<Buffer> bufferFromStringBase64(const Buffer* bufferObj);
UniqueManagedObj<Buffer> bufferWithSize(size_t size);

} // namespace giftools
