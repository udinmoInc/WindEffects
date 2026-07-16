#include "ECS/SharedComponentStore.h"

#include <cstring>

namespace we::runtime::ecs {

SharedComponentStore& SharedComponentStore::Get() {
    static SharedComponentStore instance;
    return instance;
}

std::uint32_t SharedComponentStore::HashBytes(const void* data, std::size_t size) {
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    std::uint32_t hash = 2166136261u;
    for (std::size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

SharedComponentRef SharedComponentStore::Acquire(
    ComponentTypeId sharedTypeId,
    std::uint32_t contentHash,
    const void* data,
    std::size_t size)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    const Key key{ sharedTypeId, contentHash };
    if (const auto it = m_Lookup.find(key); it != m_Lookup.end()) {
        Slot& slot = m_Slots[it->second];
        if (slot.alive
            && slot.bytes.size() == size
            && (size == 0 || std::memcmp(slot.bytes.data(), data, size) == 0)) {
            ++slot.refCount;
            return SharedComponentRef{ sharedTypeId, contentHash, it->second };
        }
    }

    std::uint32_t index = 0;
    if (!m_FreeList.empty()) {
        index = m_FreeList.back();
        m_FreeList.pop_back();
    } else {
        index = static_cast<std::uint32_t>(m_Slots.size());
        m_Slots.emplace_back();
    }

    Slot& slot = m_Slots[index];
    slot.sharedTypeId = sharedTypeId;
    slot.contentHash = contentHash;
    slot.refCount = 1;
    ++slot.generation;
    slot.alive = true;
    slot.bytes.resize(size);
    if (size > 0 && data) {
        std::memcpy(slot.bytes.data(), data, size);
    }
    m_Lookup[key] = index;
    return SharedComponentRef{ sharedTypeId, contentHash, index };
}

void SharedComponentStore::AddRef(SharedComponentRef ref) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (ref.index >= m_Slots.size()) {
        return;
    }
    Slot& slot = m_Slots[ref.index];
    if (!slot.alive || slot.contentHash != ref.sharedHash) {
        return;
    }
    ++slot.refCount;
}

void SharedComponentStore::Release(SharedComponentRef ref) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (ref.index >= m_Slots.size()) {
        return;
    }
    Slot& slot = m_Slots[ref.index];
    if (!slot.alive || slot.contentHash != ref.sharedHash) {
        return;
    }
    if (slot.refCount == 0) {
        return;
    }
    if (--slot.refCount == 0) {
        m_Lookup.erase(Key{ slot.sharedTypeId, slot.contentHash });
        slot.alive = false;
        slot.bytes.clear();
        slot.bytes.shrink_to_fit();
        m_FreeList.push_back(ref.index);
    }
}

const void* SharedComponentStore::GetData(SharedComponentRef ref) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (ref.index >= m_Slots.size()) {
        return nullptr;
    }
    const Slot& slot = m_Slots[ref.index];
    if (!slot.alive || slot.contentHash != ref.sharedHash) {
        return nullptr;
    }
    return slot.bytes.data();
}

std::size_t SharedComponentStore::GetSize(SharedComponentRef ref) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (ref.index >= m_Slots.size()) {
        return 0;
    }
    const Slot& slot = m_Slots[ref.index];
    if (!slot.alive || slot.contentHash != ref.sharedHash) {
        return 0;
    }
    return slot.bytes.size();
}

std::uint32_t SharedComponentStore::GetRefCount(SharedComponentRef ref) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (ref.index >= m_Slots.size()) {
        return 0;
    }
    const Slot& slot = m_Slots[ref.index];
    if (!slot.alive || slot.contentHash != ref.sharedHash) {
        return 0;
    }
    return slot.refCount;
}

bool SharedComponentStore::Valid(SharedComponentRef ref) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (ref.index >= m_Slots.size()) {
        return false;
    }
    const Slot& slot = m_Slots[ref.index];
    return slot.alive && slot.contentHash == ref.sharedHash && slot.sharedTypeId == ref.sharedTypeId;
}

SharedComponentRef SharedComponentStore::Find(ComponentTypeId sharedTypeId, std::uint32_t contentHash) const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    const auto it = m_Lookup.find(Key{ sharedTypeId, contentHash });
    if (it == m_Lookup.end()) {
        return {};
    }
    return SharedComponentRef{ sharedTypeId, contentHash, it->second };
}

std::size_t SharedComponentStore::AliveCount() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    std::size_t count = 0;
    for (const Slot& slot : m_Slots) {
        if (slot.alive) {
            ++count;
        }
    }
    return count;
}

void SharedComponentStore::Clear() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Slots.clear();
    m_FreeList.clear();
    m_Lookup.clear();
}

} // namespace we::runtime::ecs
