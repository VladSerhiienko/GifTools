#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

#ifndef GIFTOOLS_LOG
#define GIFTOOLS_LOG(log_level, function_name, fmt, ...) printf("%s/%s:" fmt "\n", log_level, function_name, ## __VA_ARGS__)
#endif // #ifndef GIFTOOLS_LOG

#ifndef GIFTOOLS_FUNC_NAME
// Note, too verbose.
// #define GIFTOOLS_FUNC_NAME __PRETTY_FUNCTION__
#define GIFTOOLS_FUNC_NAME __FUNCTION__
#endif // #ifndef GIFTOOLS_FUNC_NAME

#ifndef GIFTOOLS_LOGE
#define GIFTOOLS_LOGE(fmt, ...) GIFTOOLS_LOG("E", GIFTOOLS_FUNC_NAME, fmt, ## __VA_ARGS__)
#define GIFTOOLS_LOGW(fmt, ...) GIFTOOLS_LOG("W", GIFTOOLS_FUNC_NAME, fmt, ## __VA_ARGS__)
#define GIFTOOLS_LOGI(fmt, ...) GIFTOOLS_LOG("I", GIFTOOLS_FUNC_NAME, fmt, ## __VA_ARGS__)
#define GIFTOOLS_LOGT(fmt, ...) GIFTOOLS_LOG("T", GIFTOOLS_FUNC_NAME, fmt, ## __VA_ARGS__)
#endif // #ifndef GIFTOOLS_LOGE

#if !defined(DEBUG) || !DEBUG
#undef GIFTOOLS_LOGI
#undef GIFTOOLS_LOGT
#define GIFTOOLS_LOGI(fmt, ...)
#define GIFTOOLS_LOGT(fmt, ...)
#endif // #if !defined(DEBUG) || !DEBUG

namespace giftools {

using ManagedObjIdInt = uint32_t;
struct ManagedObjId;
class ManagedObj;
class ManagedObjStorage;
class ManagedObjStoragePage;

struct ManagedObjId {
    union {
        struct {
            uint32_t identifier : 20;
            uint32_t generation : 8;
            uint32_t type : 4;
        };

        uint32_t composite;
    };
};

ManagedObjId managedObjIdMake();
bool managedObjIdEqual(const ManagedObjId& a, const ManagedObjId& b);
static_assert(sizeof(ManagedObjId) == sizeof(ManagedObjIdInt), "Caught size mismatch.");
static_assert(sizeof(ManagedObjId) == sizeof(int), "Caught size mismatch.");

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
    friend ManagedObjStoragePage;
    ManagedObjId& mutableObjId();
    ManagedObjId mObjId = managedObjIdMake();
};

struct ManagedObjStorageDeleter {
    void operator()(ManagedObj* managedObj) const;
};

template <typename T>
using UniqueManagedObj = std::unique_ptr<T, ManagedObjStorageDeleter>;

template <typename T, typename... Args>
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
inline uint8_t managedType<ManagedObj>() {
    return CustomType;
}
template <typename T>
T* managedCast(ManagedObj* managedObj) {
    if (!managedObj) { return nullptr; }
    if (managedObj->objId().type != managedType<T>()) { return nullptr; }
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
        if (auto reserved = reserve()) { return store<T>(reserved.value().first, reserved.value().second); }
        return {};
    }

    void free(const ManagedObj* managedObj);

protected:
    static ManagedObjStorage& instance();

    template <typename T>
    UniqueManagedObj<T> store(size_t pageIndex, size_t slotIndex) {
        auto managedObj = makeUniqueManagedObj<T>(this);
        if (store(managedObj.get(), pageIndex, slotIndex, managedType<T>())) { return managedObj; }

        return {};
    }

    std::optional<std::pair<size_t, size_t>> reserve();
    void init(ManagedObj* managedObj, size_t pageIndex, size_t slotIndex, uint8_t type);
    bool store(ManagedObj* managedObj, size_t pageIndex, size_t slotIndex, uint8_t type);

    friend ManagedObjStorageDeleter;

    std::vector<std::unique_ptr<ManagedObjStoragePage>> mPages;
};
} // namespace giftools
