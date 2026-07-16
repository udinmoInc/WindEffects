#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "ECS/Export.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

namespace we::runtime::ecs {

// Dynamic buffer component: growable contiguous array stored as a component.
// Suitable for vertices, bone weights, path points, etc. Kept SoA-friendly by
// living in the entity's component column as a small header + heap payload.
template <typename T>
class DynamicBuffer {
public:
    using value_type = T;

    DynamicBuffer() = default;
    DynamicBuffer(const DynamicBuffer& other) = default;
    DynamicBuffer& operator=(const DynamicBuffer& other) = default;
    DynamicBuffer(DynamicBuffer&& other) noexcept = default;
    DynamicBuffer& operator=(DynamicBuffer&& other) noexcept = default;

    [[nodiscard]] T* Data() { return m_Data.data(); }
    [[nodiscard]] const T* Data() const { return m_Data.data(); }
    [[nodiscard]] std::size_t Size() const { return m_Data.size(); }
    [[nodiscard]] std::size_t Capacity() const { return m_Data.capacity(); }
    [[nodiscard]] bool Empty() const { return m_Data.empty(); }

    void Resize(std::size_t count) { m_Data.resize(count); }
    void Reserve(std::size_t count) { m_Data.reserve(count); }
    void Clear() { m_Data.clear(); }

    void Push(const T& value) { m_Data.push_back(value); }
    void Push(T&& value) { m_Data.push_back(std::move(value)); }

    T& operator[](std::size_t index) { return m_Data[index]; }
    const T& operator[](std::size_t index) const { return m_Data[index]; }

    T* begin() { return m_Data.data(); }
    T* end() { return m_Data.data() + m_Data.size(); }
    const T* begin() const { return m_Data.data(); }
    const T* end() const { return m_Data.data() + m_Data.size(); }

private:
    std::vector<T> m_Data;
};

// Shared component handle: entities with the same SharedRef share one instance
// keyed by hash. The archetype's sharedComponentHash partitions storage.
struct SharedComponentRef {
    std::uint32_t sharedTypeId = 0;
    std::uint32_t sharedHash = 0;
    std::uint32_t index = 0;
};

} // namespace we::runtime::ecs

#pragma warning(pop)
