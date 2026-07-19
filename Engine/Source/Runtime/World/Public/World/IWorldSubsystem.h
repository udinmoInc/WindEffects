#pragma once

#include "World/Export.h"
#include "World/WorldTypes.h"

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::world {

class IWorld;
class IWorldContext;

/// Replaceable world subsystem (physics bridge, AI, navigation, gameplay modules, plugins).
class WORLD_API IWorldSubsystem {
public:
    virtual ~IWorldSubsystem() = default;

    [[nodiscard]] virtual std::string_view Name() const noexcept = 0;
    virtual void Initialize(IWorld& world) = 0;
    virtual void Deinitialize() = 0;
    virtual void BeginPlay(IWorldContext& context) = 0;
    virtual void EndPlay(IWorldContext& context, EndPlayReason reason) = 0;
    virtual void Tick(IWorldContext& context, float deltaSeconds) = 0;
    [[nodiscard]] virtual TickGroup PreferredTickGroup() const noexcept { return TickGroup::DuringUpdate; }
    [[nodiscard]] virtual float TickPriority() const noexcept { return 0.f; }
};

using WorldSubsystemFactory = std::unique_ptr<IWorldSubsystem> (*)();

class WORLD_API IWorldSubsystemRegistry {
public:
    virtual ~IWorldSubsystemRegistry() = default;

    virtual void Register(std::unique_ptr<IWorldSubsystem> subsystem) = 0;
    virtual bool Unregister(std::string_view name) = 0;
    [[nodiscard]] virtual IWorldSubsystem* Find(std::string_view name) noexcept = 0;
    [[nodiscard]] virtual std::span<IWorldSubsystem* const> All() const = 0;

    virtual void InitializeAll(IWorld& world) = 0;
    virtual void DeinitializeAll() = 0;
    virtual void BeginPlayAll(IWorldContext& context) = 0;
    virtual void EndPlayAll(IWorldContext& context, EndPlayReason reason) = 0;
};

/// Global factory registry for plugins to contribute subsystems without modifying World Runtime.
class WORLD_API WorldSubsystemFactoryRegistry {
public:
    static WorldSubsystemFactoryRegistry& Get() noexcept;

    void Register(std::string name, WorldSubsystemFactory factory);
    bool Unregister(std::string_view name);
    [[nodiscard]] std::vector<std::unique_ptr<IWorldSubsystem>> CreateAll() const;

private:
    struct Entry {
        std::string name;
        WorldSubsystemFactory factory = nullptr;
    };
    std::vector<Entry> m_Entries;
};

} // namespace we::runtime::world
