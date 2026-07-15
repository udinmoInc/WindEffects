#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "ECS/Export.h"
#include "ECS/Registry.h"

#include <memory>
#include <string>
#include <vector>

namespace we::runtime::ecs {

enum class AccessMode : std::uint8_t {
    Read,
    Write
};

struct ComponentAccess {
    ComponentTypeId type = kInvalidComponentType;
    AccessMode mode = AccessMode::Read;
};

// Systems contain behavior only — no entity storage.
class ECS_API ISystem {
public:
    virtual ~ISystem() = default;
    virtual const char* Name() const = 0;
    virtual void OnCreate(Registry& /*registry*/) {}
    virtual void OnDestroy(Registry& /*registry*/) {}
    virtual void Update(Registry& registry, float deltaSeconds) = 0;
    virtual const std::vector<ComponentAccess>& Access() const {
        static const std::vector<ComponentAccess> empty;
        return empty;
    }
};

class ECS_API SystemScheduler {
public:
    SystemScheduler() = default;
    ~SystemScheduler() = default;
    SystemScheduler(const SystemScheduler&) = delete;
    SystemScheduler& operator=(const SystemScheduler&) = delete;
    SystemScheduler(SystemScheduler&&) noexcept = default;
    SystemScheduler& operator=(SystemScheduler&&) noexcept = default;

    void Add(std::unique_ptr<ISystem> system);
    void Clear();
    void OnCreate(Registry& registry);
    void OnDestroy(Registry& registry);

    // Sequential today; dependency graph prepared for parallel job dispatch.
    void Update(Registry& registry, float deltaSeconds);

    // Returns true if two systems have no Write conflicts (safe for parallel).
    static bool AreCompatible(const ISystem& a, const ISystem& b);

    const std::vector<std::unique_ptr<ISystem>>& Systems() const { return m_Systems; }

private:
    std::vector<std::unique_ptr<ISystem>> m_Systems;
};

// ---- Core systems ----

class ECS_API HierarchySystem : public ISystem {
public:
    const char* Name() const override { return "HierarchySystem"; }
    void Update(Registry& registry, float deltaSeconds) override;
    void SetParent(Registry& registry, Entity child, Entity parent);
    void Detach(Registry& registry, Entity child);

private:
    void RelinkRemove(Registry& registry, Entity child);
    void RelinkInsert(Registry& registry, Entity child, Entity parent);
};

class ECS_API TransformSystem : public ISystem {
public:
    const char* Name() const override { return "TransformSystem"; }
    const std::vector<ComponentAccess>& Access() const override;
    void Update(Registry& registry, float deltaSeconds) override;
    void MarkDirty(Registry& registry, Entity entity);
};

class ECS_API VisibilitySystem : public ISystem {
public:
    const char* Name() const override { return "VisibilitySystem"; }
    void Update(Registry& registry, float deltaSeconds) override;
};

class ECS_API CameraSystem : public ISystem {
public:
    const char* Name() const override { return "CameraSystem"; }
    void Update(Registry& registry, float deltaSeconds) override;
    Entity PrimaryCamera() const { return m_Primary; }

private:
    Entity m_Primary{};
};

class ECS_API LightingSystem : public ISystem {
public:
    const char* Name() const override { return "LightingSystem"; }
    void Update(Registry& registry, float deltaSeconds) override;
};

class ECS_API RenderSystem : public ISystem {
public:
    const char* Name() const override { return "RenderSystem"; }
    void Update(Registry& registry, float deltaSeconds) override;
    std::size_t LastDrawCount() const { return m_LastDrawCount; }

private:
    std::size_t m_LastDrawCount = 0;
};

class ECS_API SkyAtmosphereSystem : public ISystem {
public:
    const char* Name() const override { return "SkyAtmosphereSystem"; }
    void Update(Registry& registry, float deltaSeconds) override;
};

class ECS_API VolumetricCloudSystem : public ISystem {
public:
    const char* Name() const override { return "VolumetricCloudSystem"; }
    void Update(Registry& registry, float deltaSeconds) override;
};

class ECS_API TerrainEcsSystem : public ISystem {
public:
    const char* Name() const override { return "TerrainSystem"; }
    void Update(Registry& registry, float deltaSeconds) override;
};

class ECS_API PhysicsSystem : public ISystem {
public:
    const char* Name() const override { return "PhysicsSystem"; }
    void Update(Registry& registry, float deltaSeconds) override;
};

class ECS_API AnimationSystem : public ISystem {
public:
    const char* Name() const override { return "AnimationSystem"; }
    void Update(Registry& registry, float deltaSeconds) override;
};

class ECS_API AudioSystem : public ISystem {
public:
    const char* Name() const override { return "AudioSystem"; }
    void Update(Registry& registry, float deltaSeconds) override;
};

ECS_API void RegisterDefaultSystems(SystemScheduler& scheduler);

} // namespace we::runtime::ecs

#pragma warning(pop)
