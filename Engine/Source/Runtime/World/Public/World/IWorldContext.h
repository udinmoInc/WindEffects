#pragma once

#include "World/Export.h"
#include "World/WorldTypes.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <typeindex>
#include <typeinfo>

namespace we::runtime::world {

class IWorld;
class IWorldManager;

/// Per-frame / per-world execution context. Injected into systems and extension points.
/// Stateless services may read context; mutable world state stays on IWorld.
class WORLD_API IWorldContext {
public:
    virtual ~IWorldContext() = default;

    [[nodiscard]] virtual IWorld& World() noexcept = 0;
    [[nodiscard]] virtual const IWorld& World() const noexcept = 0;
    [[nodiscard]] virtual IWorldManager* Manager() noexcept = 0;

    [[nodiscard]] virtual float DeltaSeconds() const noexcept = 0;
    [[nodiscard]] virtual float FixedDeltaSeconds() const noexcept = 0;
    [[nodiscard]] virtual double WorldTimeSeconds() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t FrameIndex() const noexcept = 0;
    [[nodiscard]] virtual WorldNetMode NetMode() const noexcept = 0;
    [[nodiscard]] virtual bool IsPlaying() const noexcept = 0;
    [[nodiscard]] virtual bool IsEditor() const noexcept = 0;
    [[nodiscard]] virtual bool IsDedicatedServer() const noexcept = 0;

    /// Typed service bag for DI (physics, audio, net, editor hooks, custom plugins).
    virtual void RegisterService(std::type_index type, std::shared_ptr<void> service) = 0;
    virtual void UnregisterService(std::type_index type) = 0;
    [[nodiscard]] virtual std::shared_ptr<void> TryGetService(std::type_index type) const = 0;

    template <typename T>
    void RegisterService(std::shared_ptr<T> service) {
        RegisterService(std::type_index(typeid(T)), std::static_pointer_cast<void>(std::move(service)));
    }

    template <typename T>
    [[nodiscard]] std::shared_ptr<T> TryGetService() const {
        return std::static_pointer_cast<T>(TryGetService(std::type_index(typeid(T))));
    }

    virtual void SetTickParams(const WorldTickParams& params) = 0;
};

} // namespace we::runtime::world
