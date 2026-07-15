#pragma once

#include "ECS/Entity.h"

#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace we::runtime::ecs {

inline constexpr std::uint32_t kSparseEmpty = 0xFFFFFFFFu;

// Classic sparse set: O(1) insert/remove/lookup, dense iteration for cache locality.
template <typename T>
class SparseSet {
public:
    bool Contains(Entity entity) const {
        const std::uint32_t index = entity.Index();
        if (index >= m_Sparse.size()) {
            return false;
        }
        const std::uint32_t dense = m_Sparse[index];
        return dense < m_Dense.size() && m_Dense[dense].Index() == index;
    }

    T* TryGet(Entity entity) {
        if (!Contains(entity)) {
            return nullptr;
        }
        return &m_Data[m_Sparse[entity.Index()]];
    }

    const T* TryGet(Entity entity) const {
        if (!Contains(entity)) {
            return nullptr;
        }
        return &m_Data[m_Sparse[entity.Index()]];
    }

    T& Get(Entity entity) { return *TryGet(entity); }
    const T& Get(Entity entity) const { return *TryGet(entity); }

    template <typename... Args>
    T& Emplace(Entity entity, Args&&... args) {
        const std::uint32_t index = entity.Index();
        EnsureSparse(index);
        if (Contains(entity)) {
            m_Data[m_Sparse[index]] = T(std::forward<Args>(args)...);
            return m_Data[m_Sparse[index]];
        }
        const std::uint32_t dense = static_cast<std::uint32_t>(m_Dense.size());
        m_Sparse[index] = dense;
        m_Dense.push_back(entity);
        m_Data.emplace_back(std::forward<Args>(args)...);
        m_Enabled.push_back(1);
        return m_Data.back();
    }

    T& Replace(Entity entity, T value) {
        if (Contains(entity)) {
            m_Data[m_Sparse[entity.Index()]] = std::move(value);
            return m_Data[m_Sparse[entity.Index()]];
        }
        return Emplace(entity, std::move(value));
    }

    bool Remove(Entity entity) {
        if (!Contains(entity)) {
            return false;
        }
        const std::uint32_t index = entity.Index();
        const std::uint32_t dense = m_Sparse[index];
        const std::uint32_t last = static_cast<std::uint32_t>(m_Dense.size() - 1);
        if (dense != last) {
            const Entity moved = m_Dense[last];
            m_Dense[dense] = moved;
            m_Data[dense] = std::move(m_Data[last]);
            m_Enabled[dense] = m_Enabled[last];
            m_Sparse[moved.Index()] = dense;
        }
        m_Dense.pop_back();
        m_Data.pop_back();
        m_Enabled.pop_back();
        m_Sparse[index] = kSparseEmpty;
        return true;
    }

    void SetEnabled(Entity entity, bool enabled) {
        if (!Contains(entity)) {
            return;
        }
        m_Enabled[m_Sparse[entity.Index()]] = enabled ? 1 : 0;
    }

    bool IsEnabled(Entity entity) const {
        if (!Contains(entity)) {
            return false;
        }
        return m_Enabled[m_Sparse[entity.Index()]] != 0;
    }

    void Clear() {
        m_Sparse.clear();
        m_Dense.clear();
        m_Data.clear();
        m_Enabled.clear();
    }

    std::size_t Size() const { return m_Dense.size(); }
    bool Empty() const { return m_Dense.empty(); }

    Entity* DenseEntities() { return m_Dense.data(); }
    const Entity* DenseEntities() const { return m_Dense.data(); }
    T* DenseData() { return m_Data.data(); }
    const T* DenseData() const { return m_Data.data(); }
    const std::uint8_t* EnabledMask() const { return m_Enabled.data(); }

private:
    void EnsureSparse(std::uint32_t index) {
        if (index >= m_Sparse.size()) {
            m_Sparse.resize(static_cast<std::size_t>(index) + 1u, kSparseEmpty);
        }
    }

    std::vector<std::uint32_t> m_Sparse;
    std::vector<Entity> m_Dense;
    std::vector<T> m_Data;
    std::vector<std::uint8_t> m_Enabled;
};

} // namespace we::runtime::ecs
