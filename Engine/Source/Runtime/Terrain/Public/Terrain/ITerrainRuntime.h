#pragma once

#include "Terrain/Export.h"
#include "Terrain/TerrainTypes.h"
#include "Terrain/ITerrain.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::serialization {
class ISerializer;
}

namespace we::runtime::reflection {
class ITypeRegistry;
}

namespace we::runtime::scene {
class Scene;
}

namespace we::runtime::world {
class IWorldRuntime;
}

namespace we::rhi {
class IRHIDevice;
}

namespace we::runtime::terrain {

struct TERRAIN_API TerrainEvent {
    TerrainEventKind kind = TerrainEventKind::Created;
    TerrainId terrain{};
    TerrainGuid asset{};
    std::string detail;
};

using TerrainEventListener = std::function<void(const TerrainEvent&)>;

class TERRAIN_API ITerrainEventRouter {
public:
    virtual ~ITerrainEventRouter() = default;
    virtual void Publish(const TerrainEvent& event) = 0;
    virtual void Subscribe(TerrainEventListener listener) = 0;
};

class TERRAIN_API ITerrainGeneratorRegistry {
public:
    virtual ~ITerrainGeneratorRegistry() = default;

    [[nodiscard]] virtual bool Register(std::shared_ptr<ITerrainGenerator> generator) = 0;
    [[nodiscard]] virtual bool Unregister(std::string_view id) = 0;
    [[nodiscard]] virtual ITerrainGenerator* Find(std::string_view id) const = 0;
    [[nodiscard]] virtual ITerrainGenerator* FindBuiltin(TerrainGeneratorId id) const = 0;
    [[nodiscard]] virtual std::vector<std::string> ListIds() const = 0;
};

/// Facade owning terrain instances and cross-cutting services.
class TERRAIN_API ITerrainManager {
public:
    virtual ~ITerrainManager() = default;

    [[nodiscard]] virtual ITerrainGeneratorRegistry& Generators() noexcept = 0;
    [[nodiscard]] virtual ITerrainImporter& Importer() noexcept = 0;
    [[nodiscard]] virtual ITerrainExporter& Exporter() noexcept = 0;
    [[nodiscard]] virtual ITerrainValidator& Validator() noexcept = 0;
    [[nodiscard]] virtual ITerrainEventRouter& Events() noexcept = 0;

    /// Create a landscape. Default method is Flat — no heightmap file required.
    [[nodiscard]] virtual TerrainId CreateLandscape(const TerrainCreateInfo& info) = 0;
    [[nodiscard]] virtual bool DestroyLandscape(TerrainId id) = 0;
    [[nodiscard]] virtual ITerrain* Find(TerrainId id) noexcept = 0;
    [[nodiscard]] virtual const ITerrain* Find(TerrainId id) const noexcept = 0;
    [[nodiscard]] virtual ITerrain* GetActive() noexcept = 0;
    virtual void SetActive(TerrainId id) = 0;
    [[nodiscard]] virtual std::vector<TerrainId> ListAll() const = 0;

    [[nodiscard]] virtual std::uint64_t SpawnLandscapeActor(
        TerrainId id,
        const char* name = "Landscape") = 0;

    virtual void BindScene(scene::Scene* scene) = 0;
    virtual void BindRenderer(rhi::IRHIDevice* device) = 0;

    virtual void Tick(
        float deltaSeconds,
        const we::math::Vec3& cameraWorldPos,
        const we::math::Mat4* viewProj) = 0;
};

/// Editor / host session binding surface (Undo injected by host to avoid cycles).
class TERRAIN_API ITerrainSession {
public:
    virtual ~ITerrainSession() = default;

    virtual void BindManager(ITerrainManager* manager) = 0;
    [[nodiscard]] virtual ITerrainManager* Manager() noexcept = 0;

    /// Host-provided Undo callback: (label, undoFn, redoFn) → success.
    using TransactionFn = std::function<bool(
        std::string_view label,
        std::function<bool()> undo,
        std::function<bool()> redo)>;

    virtual void SetTransactionRecorder(TransactionFn recorder) = 0;

    [[nodiscard]] virtual bool BeginEditStroke(TerrainId id) = 0;
    [[nodiscard]] virtual bool EndEditStroke(TerrainId id) = 0;
    [[nodiscard]] virtual bool ApplyBrushWithUndo(
        TerrainId id,
        float worldX,
        float worldZ) = 0;
};

class TERRAIN_API ITerrainRuntime {
public:
    virtual ~ITerrainRuntime() = default;

    [[nodiscard]] virtual ITerrainManager& Manager() noexcept = 0;
    [[nodiscard]] virtual const ITerrainManager& Manager() const noexcept = 0;
    [[nodiscard]] virtual ITerrainSession& Session() noexcept = 0;

    virtual void Shutdown() = 0;
};

struct TERRAIN_API TerrainDependencies {
    reflection::ITypeRegistry* typeRegistry = nullptr;
    serialization::ISerializer* serializer = nullptr;
    scene::Scene* scene = nullptr;
    world::IWorldRuntime* world = nullptr;
    rhi::IRHIDevice* device = nullptr;
    TerrainConfig config{};
    std::function<void(std::string_view)> onLog;
};

[[nodiscard]] TERRAIN_API std::unique_ptr<ITerrainRuntime> CreateTerrainRuntime(
    const TerrainDependencies& deps);

/// Optional process-wide default runtime for editor singletons / legacy TerrainSystem.
[[nodiscard]] TERRAIN_API ITerrainRuntime& GetDefaultTerrainRuntime();
TERRAIN_API void SetDefaultTerrainRuntime(std::unique_ptr<ITerrainRuntime> runtime);

} // namespace we::runtime::terrain
