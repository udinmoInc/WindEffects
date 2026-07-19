#pragma once

#include "World/Export.h"
#include "World/WorldTypes.h"

#include <functional>
#include <span>

namespace we::runtime::world {

/// Parent-child actor hierarchy (mirrors ECS HierarchyComponent; World Runtime is authority for ActorHandles).
class WORLD_API IHierarchyService {
public:
    virtual ~IHierarchyService() = default;

    [[nodiscard]] virtual bool SetParent(ActorHandle child, ActorHandle parent) = 0;
    [[nodiscard]] virtual bool Detach(ActorHandle child) = 0;
    [[nodiscard]] virtual ActorHandle GetParent(ActorHandle actor) const noexcept = 0;
    [[nodiscard]] virtual std::span<const ActorHandle> GetChildren(ActorHandle actor) const = 0;
    [[nodiscard]] virtual bool IsAncestorOf(ActorHandle ancestor, ActorHandle descendant) const noexcept = 0;
    [[nodiscard]] virtual std::uint16_t Depth(ActorHandle actor) const noexcept = 0;

    using VisitFn = std::function<void(ActorHandle)>;
    virtual void TraverseDepthFirst(ActorHandle root, const VisitFn& visit) const = 0;
    virtual void TraverseBreadthFirst(ActorHandle root, const VisitFn& visit) const = 0;
};

/// Scene-graph facade over actor hierarchy + transform roots.
class WORLD_API ISceneGraph {
public:
    virtual ~ISceneGraph() = default;

    [[nodiscard]] virtual ActorHandle Root() const noexcept = 0;
    [[nodiscard]] virtual IHierarchyService& Hierarchy() noexcept = 0;
    [[nodiscard]] virtual std::span<const ActorHandle> Roots() const = 0;
    virtual void Rebuild() = 0;
};

} // namespace we::runtime::world
