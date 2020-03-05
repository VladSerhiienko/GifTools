#include "GifToolsManagedObject.h"
#include <numeric>

//
// ManagedObj
//

constexpr giftools::ManagedObjIdInt InvalidId = std::numeric_limits<giftools::ManagedObjIdInt>::max();
giftools::ManagedObjId giftools::managedObjIdMake() { constexpr auto z = ManagedObjId{.composite=InvalidId}; return z; }
bool giftools::managedObjIdEqual(const giftools::ManagedObjId& a, const giftools::ManagedObjId& b) { return a.composite == b.composite; }

giftools::ManagedObj::ManagedObj(ManagedObj&& other) : mObjId(other.mObjId) { other.mObjId = managedObjIdMake(); }
giftools::ManagedObj::ManagedObj(const ManagedObj& other) : mObjId(other.mObjId) { ++mObjId.generation; }
giftools::ManagedObjId giftools::ManagedObj::objId() const { return mObjId; }
giftools::ManagedObjId& giftools::ManagedObj::mutableObjId() { return mObjId; }
giftools::ManagedObj::~ManagedObj() = default;
giftools::ManagedObj::ManagedObj() = default;

//
// ManagedObjStoragePage
//

class giftools::ManagedObjStoragePage {
public:
    static constexpr size_t PageLength = 256;
    static constexpr size_t InvalidSlot = size_t(-1);
    using Slot = std::unique_ptr<ManagedObj>;
    
    size_t mCachedFreeSlotCount = PageLength;
    std::array<Slot, PageLength> mSlots = {};
    
    size_t freeSlotIndex() {
        if (mCachedFreeSlotCount) {
            auto it = std::find(mSlots.begin(), mSlots.end(), nullptr);
            if (it != mSlots.end()) { --mCachedFreeSlotCount; return std::distance(mSlots.begin(), it); }
        }
        return InvalidSlot;
    }
    
    ManagedObj* get(uint32_t identifier) const {
        size_t slotIndex = identifier % ManagedObjStoragePage::PageLength;
        return mSlots.at(slotIndex).get();
    }
    
    void store(ManagedObj* managedObj, size_t slotIndex) {
        assert(managedObj && slotIndex != InvalidSlot);
        mSlots.at(slotIndex).reset(managedObj);
    }
    
    size_t store(ManagedObj* managedObj) {
        assert(managedObj);
        if (auto slotIndex = freeSlotIndex(); slotIndex != InvalidSlot) {
            store(managedObj, slotIndex);
            return slotIndex;
        }
        
        return InvalidSlot;
    }
    
    bool free(const ManagedObj* managedObj) {
        uint64_t identifier = managedObj->objId().identifier % PageLength;
        
        const ManagedObj* storedManagedObj = mSlots.at(identifier).get();
        if (managedObj != storedManagedObj) { return false; }
        if (!managedObjIdEqual(storedManagedObj->objId(), managedObj->objId())) { return false; }
        
        mSlots.at(identifier).reset();
        ++mCachedFreeSlotCount;
        return true;
    }
};

constexpr size_t InitialPageCount = 4;
constexpr uint32_t managedIndex(size_t pageIndex, size_t slotIndex) { return pageIndex * giftools::ManagedObjStoragePage::PageLength + slotIndex; }
constexpr uint32_t pageIndex(uint32_t identifier) { return identifier / giftools::ManagedObjStoragePage::PageLength; }

//
// ManagedObjStorageDeleter
//

void giftools::ManagedObjStorageDeleter::operator()(ManagedObj* managedObj) const {
    managedObjStorageDefault().free(managedObj);
}

//
// ManagedObjStorage
//


giftools::ManagedObjStorage* DefaultManagedObjStorage = nullptr;
giftools::ManagedObjStorage& giftools::managedObjStorageDefault() { return *DefaultManagedObjStorage; }

giftools::ManagedObjStorage& giftools::ManagedObjStorage::instance() {
    static giftools::ManagedObjStorage Instance = {};
    return Instance;
}

void giftools::ManagedObjStorage::init() {
    DefaultManagedObjStorage = &ManagedObjStorage::instance();
}

giftools::ManagedObjStorage::~ManagedObjStorage() = default;
giftools::ManagedObjStorage::ManagedObjStorage() {
    mPages.reserve(InitialPageCount);
    assert(DefaultManagedObjStorage == nullptr);
}

void giftools::ManagedObjStorage::init(ManagedObj* managedObj, size_t pageIndex, size_t slotIndex, uint8_t type) {
    managedObj->mutableObjId().identifier = managedIndex(pageIndex, slotIndex);
    managedObj->mutableObjId().type = type;
    managedObj->mutableObjId().generation = 1;
    managedObj->mutableObjId().flags = 0;
}

std::optional<std::pair<size_t, size_t>> giftools::ManagedObjStorage::reserve() {
    for (auto& p : mPages) {
        if (size_t slotIndex = p->freeSlotIndex(); slotIndex != ManagedObjStoragePage::InvalidSlot) {
            return std::make_pair(std::distance(&mPages.front(), &p), slotIndex);
        }
    }
    
    if (auto newPage = std::make_unique<ManagedObjStoragePage>()) {
        size_t pageIndex = mPages.size();
        size_t slotIndex = newPage->freeSlotIndex();
        mPages.emplace_back(std::move(newPage));
        return std::make_pair(pageIndex, slotIndex);
    }
    
    return {};
}
    
giftools::ManagedObj* giftools::ManagedObjStorage::get(uint32_t identifier) const {
    const size_t p = pageIndex(identifier);
    return mPages.at(p)->get(identifier);
}

bool giftools::ManagedObjStorage::store(ManagedObj* managedObj, size_t pageIndex, size_t slotIndex, uint8_t type) {
    if (managedObj) {
        init(managedObj, pageIndex, slotIndex, type);
        if (ManagedObjStoragePage* page = mPages.at(pageIndex).get()) {
            page->store(managedObj, slotIndex);
            return true;
        }
    }
    
    return false;
}
    
void giftools::ManagedObjStorage::free(const ManagedObj* managedObj) {
    if (!managedObj) { return; }
    const size_t p = pageIndex(managedObj->objId().identifier);
    mPages.at(p)->free(managedObj);
}

