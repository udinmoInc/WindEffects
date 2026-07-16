#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "ECS/ComponentType.h"
#include "ECS/DynamicBuffer.h"
#include "ECS/Export.h"
#include "ECS/Types.h"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace we::runtime::ecs {

// Hash-keyed shared component storage with deduplication and reference counting.
// Used for Materials, Mesh refs, Terrain/Environment settings, Physics materials, etc.
class ECS_API SharedComponentStore {
public:
    static SharedComponentStore& Get();

    // Insert or reuse an existing instance with identical typeId+hash+payload.
    // On reuse, increments refcount. Returns a stable SharedComponentRef.
    [[nodiscard]] SharedComponentRef Acquire(
        ComponentTypeId sharedTypeId,
        std::uint32_t contentHash,
        const void* data,
        std::size_t size);

    // Increment refcount for an existing handle.
    void AddRef(SharedComponentRef ref);

    // Decrement refcount; frees storage when it reaches zero.
    void Release(SharedComponentRef ref);

    [[nodiscard]] const void* GetData(SharedComponentRef ref) const;
    [[nodiscard]] std::size_t GetSize(SharedComponentRef ref) const;
    [[nodiscard]] std::uint32_t GetRefCount(SharedComponentRef ref) const;
    [[nodiscard]] bool Valid(SharedComponentRef ref) const;

    [[nodiscard]] SharedComponentRef Find(ComponentTypeId sharedTypeId, std::uint32_t contentHash) const;

    [[nodiscard]] std::size_t AliveCount() const;
    void Clear();

    // FNV-1a hash helper for POD payloads.
    [[nodiscard]] static std::uint32_t HashBytes(const void* data, std::size_t size);
    template <typename T>
    [[nodiscard]] static std::uint32_t HashValue(const T& value) {
        return HashBytes(&value, sizeof(T));
    }

private:
    struct Slot {
        ComponentTypeId sharedTypeId = kInvalidComponentType;
        std::uint32_t contentHash = 0;
        std::uint32_t refCount = 0;
        std::uint32_t generation = 0;
        std::vector<std::uint8_t> bytes;
        bool alive = false;
    };

    struct Key {
        ComponentTypeId typeId = kInvalidComponentType;
        std::uint32_t hash = 0;
        bool operator==(const Key& o) const { return typeId == o.typeId && hash == o.hash; }
    };
    struct KeyHash {
        std::size_t operator()(const Key& k) const noexcept {
            return (static_cast<std::size_t>(k.typeId) << 32) ^ k.hash;
        }
    };

    SharedComponentStore() = default;

    mutable std::mutex m_Mutex;
    std::vector<Slot> m_Slots;
    std::vector<std::uint32_t> m_FreeList;
    std::unordered_map<Key, std::uint32_t, KeyHash> m_Lookup;
};

} // namespace we::runtime::ecs

#pragma warning(pop)
