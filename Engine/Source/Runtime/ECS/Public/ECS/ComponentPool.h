#pragma once

#include "ECS/Entity.h"
#include "ECS/ComponentType.h"
#include "ECS/SparseSet.h"

#include <cstdint>
#include <memory>
#include <utility>

namespace we::runtime::ecs {

class IComponentPool {
public:
    virtual ~IComponentPool() = default;
    virtual ComponentTypeId TypeId() const = 0;
    virtual bool Contains(Entity entity) const = 0;
    virtual bool Remove(Entity entity) = 0;
    virtual void SetEnabled(Entity entity, bool enabled) = 0;
    virtual bool IsEnabled(Entity entity) const = 0;
    virtual void Clear() = 0;
    virtual std::size_t Size() const = 0;
    virtual const Entity* DenseEntities() const = 0;
    virtual const std::uint8_t* EnabledMask() const = 0;
};

template <typename T>
class ComponentPool final : public IComponentPool {
public:
    explicit ComponentPool(ComponentTypeId typeId) : m_TypeId(typeId) {}

    ComponentTypeId TypeId() const override { return m_TypeId; }
    bool Contains(Entity entity) const override { return m_Set.Contains(entity); }
    bool Remove(Entity entity) override { return m_Set.Remove(entity); }
    void SetEnabled(Entity entity, bool enabled) override { m_Set.SetEnabled(entity, enabled); }
    bool IsEnabled(Entity entity) const override { return m_Set.IsEnabled(entity); }
    void Clear() override { m_Set.Clear(); }
    std::size_t Size() const override { return m_Set.Size(); }
    const Entity* DenseEntities() const override { return m_Set.DenseEntities(); }
    const std::uint8_t* EnabledMask() const override { return m_Set.EnabledMask(); }

    SparseSet<T>& Set() { return m_Set; }
    const SparseSet<T>& Set() const { return m_Set; }

private:
    ComponentTypeId m_TypeId = kInvalidComponentType;
    SparseSet<T> m_Set;
};

} // namespace we::runtime::ecs
