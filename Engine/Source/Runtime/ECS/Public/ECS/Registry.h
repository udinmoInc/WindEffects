#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "ECS/Export.h"
#include "ECS/Entity.h"
#include "ECS/ComponentType.h"
#include "ECS/ComponentPool.h"
#include "ECS/View.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::runtime::ecs {

struct EntityMeta {
    Entity entity{};
    bool alive = false;
    std::uint32_t generation = 0;
};

// Central ECS store. Owns component pools as sparse sets; no behavior here.
class ECS_API Registry {
public:
    Registry();
    ~Registry();

    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;

    Entity Create();
    Entity Create(const std::string& name);
    void Destroy(Entity entity);
    bool Valid(Entity entity) const;
    void Clear();

    std::size_t LivingCount() const { return m_Living; }
    std::size_t Capacity() const { return m_Entities.size(); }

    template <typename T, typename... Args>
    T& Add(Entity entity, Args&&... args) {
        T& value = Pool<T>().Set().Emplace(entity, std::forward<Args>(args)...);
        NotifyChanged();
        return value;
    }

    template <typename T>
    T& Replace(Entity entity, T value) {
        T& stored = Pool<T>().Set().Replace(entity, std::move(value));
        NotifyChanged();
        return stored;
    }

    template <typename T>
    bool Remove(Entity entity) {
        const bool removed = Pool<T>().Set().Remove(entity);
        if (removed) {
            NotifyChanged();
        }
        return removed;
    }

    template <typename T>
    T* TryGet(Entity entity) {
        return Pool<T>().Set().TryGet(entity);
    }

    template <typename T>
    const T* TryGet(Entity entity) const {
        return const_cast<Registry*>(this)->Pool<T>().Set().TryGet(entity);
    }

    template <typename T>
    T& Get(Entity entity) {
        return Pool<T>().Set().Get(entity);
    }

    template <typename T>
    const T& Get(Entity entity) const {
        return const_cast<Registry*>(this)->Pool<T>().Set().Get(entity);
    }

    template <typename T>
    bool Has(Entity entity) const {
        return const_cast<Registry*>(this)->Pool<T>().Set().Contains(entity);
    }

    template <typename T>
    void SetEnabled(Entity entity, bool enabled) {
        Pool<T>().Set().SetEnabled(entity, enabled);
    }

    template <typename T>
    bool IsEnabled(Entity entity) const {
        return const_cast<Registry*>(this)->Pool<T>().Set().IsEnabled(entity);
    }

    template <typename... Ts>
    View<Ts...> ViewAll() {
        return View<Ts...>(*this);
    }

    template <typename T>
    ComponentPool<T>& Pool() {
        const ComponentTypeId id = ComponentTypeRegistry::Get().Id<T>();
        EnsurePoolCapacity(id);
        if (!m_Pools[id]) {
            m_Pools[id] = std::make_unique<ComponentPool<T>>(id);
        }
        return *static_cast<ComponentPool<T>*>(m_Pools[id].get());
    }

    template <typename T>
    const ComponentPool<T>& Pool() const {
        return const_cast<Registry*>(this)->Pool<T>();
    }

    IComponentPool* PoolById(ComponentTypeId id);
    const IComponentPool* PoolById(ComponentTypeId id) const;

    // Structural change generation — query caches invalidate when this bumps.
    std::uint64_t Version() const { return m_Version; }
    void NotifyChanged() { ++m_Version; }

    // Iterate living entities (not cache-friendly for components — prefer Views).
    void ForEachLiving(const std::function<void(Entity)>& fn) const;

private:
    void EnsurePoolCapacity(ComponentTypeId id);

    std::vector<EntityMeta> m_Entities;
    std::vector<std::uint32_t> m_FreeList;
    std::vector<std::unique_ptr<IComponentPool>> m_Pools;
    std::size_t m_Living = 0;
    std::uint64_t m_Version = 1;
};

} // namespace we::runtime::ecs

#include "ECS/View.inl"

#pragma warning(pop)
