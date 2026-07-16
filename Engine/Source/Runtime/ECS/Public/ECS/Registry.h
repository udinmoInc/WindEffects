#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "ECS/Export.h"
#include "ECS/Entity.h"
#include "ECS/ComponentType.h"
#include "ECS/View.h"
#include "ECS/World.h"

#include <cstdint>
#include <functional>
#include <string>

namespace we::runtime::ecs {

// Backward-compatible facade over the archetype/chunk World.
class ECS_API Registry {
public:
    Registry();
    ~Registry();

    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;

    [[nodiscard]] World& GetWorld() { return m_World; }
    [[nodiscard]] const World& GetWorld() const { return m_World; }

    Entity Create();
    Entity Create(const std::string& name);
    void Destroy(Entity entity);
    [[nodiscard]] bool Valid(Entity entity) const;
    void Clear();

    [[nodiscard]] std::size_t LivingCount() const { return m_World.Entities().LivingCount(); }
    [[nodiscard]] std::size_t Capacity() const { return m_World.Entities().Capacity(); }

    template <typename T, typename... Args>
    T& Add(Entity entity, Args&&... args) {
        return m_World.AddComponent<T>(entity, std::forward<Args>(args)...);
    }

    template <typename T>
    T& Replace(Entity entity, T value) {
        if (T* existing = m_World.TryGet<T>(entity)) {
            *existing = std::move(value);
            m_World.BumpChangeVersion();
            return *existing;
        }
        return m_World.AddComponent<T>(entity, std::move(value));
    }

    template <typename T>
    bool Remove(Entity entity) {
        return m_World.RemoveComponent<T>(entity);
    }

    template <typename T>
    T* TryGet(Entity entity) {
        return m_World.TryGet<T>(entity);
    }

    template <typename T>
    const T* TryGet(Entity entity) const {
        return m_World.TryGet<T>(entity);
    }

    template <typename T>
    T& Get(Entity entity) {
        return m_World.Get<T>(entity);
    }

    template <typename T>
    const T& Get(Entity entity) const {
        return m_World.Get<T>(entity);
    }

    template <typename T>
    bool Has(Entity entity) const {
        return m_World.Has<T>(entity);
    }

    template <typename T>
    void SetEnabled(Entity entity, bool enabled) {
        m_World.SetComponentEnabled<T>(entity, enabled);
    }

    template <typename T>
    bool IsEnabled(Entity entity) const {
        return m_World.IsComponentEnabled<T>(entity);
    }

    template <typename... Ts>
    View<Ts...> ViewAll() {
        return View<Ts...>(*this);
    }

    [[nodiscard]] std::uint64_t Version() const { return m_World.ChangeVersion(); }
    void NotifyChanged() { m_World.BumpChangeVersion(); }

    void ForEachLiving(const std::function<void(Entity)>& fn) const;

private:
    World m_World;
};

} // namespace we::runtime::ecs

#include "ECS/View.inl"

#pragma warning(pop)
