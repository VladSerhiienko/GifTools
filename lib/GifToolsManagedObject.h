#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include <array>
#include <algorithm>
#include <optional>
#include <cassert>
#include <type_traits>

namespace giftools {

using ManagedObjIdInt = uint64_t;
struct ManagedObjId;
class ManagedObj;
class ManagedObjStorage;
class ManagedObjStoragePage;

struct ManagedObjId {
    union {
        struct {
            uint64_t identifier : 32;
            uint64_t generation : 16;
            uint64_t type : 8;
            uint64_t flags : 8;
        };
        
        uint64_t composite;
    };
};

ManagedObjId managedObjIdMake();
bool managedObjIdEqual(const ManagedObjId& a, const ManagedObjId& b);
static_assert(sizeof(ManagedObjId) == sizeof(ManagedObjIdInt), "Caught size mismatch.");

class ManagedObj;
class ManagedObjStorage;
class ManagedObjStoragePage;

class ManagedObj {
public:
    ManagedObj(ManagedObj&& other);
    ManagedObj(const ManagedObj& other);
    ManagedObjId objId() const;
    virtual ~ManagedObj();

protected:
    ManagedObj();
    
private:
    friend ManagedObjStorage;
    ManagedObjId& mutableObjId();
    ManagedObjId mObjId = managedObjIdMake();
};

struct ManagedObjStorageDeleter {
    void operator()(ManagedObj* managedObj) const;
};

template <typename T>
using UniqueManagedObj = std::unique_ptr<T, ManagedObjStorageDeleter>;

template<typename T, typename... Args>
UniqueManagedObj<T> makeUniqueManagedObj(ManagedObjStorage* storage, Args&&... args) {
    static_assert(std::is_base_of_v<ManagedObj, T>, "T must inherit ManagedObj.");
    return UniqueManagedObj<T>(new T(std::forward<Args>(args)...), ManagedObjStorageDeleter{});
}

template <typename T>
uint8_t managedType();
template <typename T>
T* managedCast(ManagedObj* managedObj);

constexpr uint8_t CustomType = 0;

template <>
inline uint8_t managedType<ManagedObj>(){ return CustomType; }
template <typename T>
T* managedCast(ManagedObj* managedObj) {
    if (!managedObj) {return nullptr;}
    if (managedObj->objId().type != managedType<T>()) {return nullptr;}
    return static_cast<T*>(managedObj);
    // assert(!managedObj || managedObj->objId().type == CustomType);
    // return managedObj ? dynamic_cast<T*>(managedObj) : nullptr;
}

ManagedObjStorage& managedObjStorageDefault();

class ManagedObjStorage {
public:
    ManagedObjStorage();
    ~ManagedObjStorage();

    ManagedObj* get(uint32_t identifier) const;
    
    template <typename T>
    T* get(uint32_t identifier) const {
        return managedCast<T>(get(identifier));
    }

    template <typename T>
    UniqueManagedObj<T> make() {
        auto reserved = reserve();
        return store<T>(reserved.value().first, reserved.value().second);
    }
    
    void free(const ManagedObj* managedObj);

protected:
    static ManagedObjStorage& instance();

    template <typename T>
    UniqueManagedObj<T> store(size_t pageIndex, size_t slotIndex) {
        auto managedObj = makeUniqueManagedObj<T>(this);
        if (store(managedObj.get(), pageIndex, slotIndex, managedType<T>())) {
            return managedObj;
        }
        
        return {};
    }

    std::optional<std::pair<size_t, size_t>> reserve();
    void init(ManagedObj* managedObj, size_t pageIndex, size_t slotIndex, uint8_t type);
    bool store(ManagedObj* managedObj, size_t pageIndex, size_t slotIndex, uint8_t type);
    
    friend ManagedObjStorageDeleter;

    std::vector<std::unique_ptr<ManagedObjStoragePage>> mPages;
};
}

