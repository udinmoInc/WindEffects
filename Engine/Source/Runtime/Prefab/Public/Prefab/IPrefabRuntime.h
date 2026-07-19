#pragma once

#include "Prefab/Export.h"
#include "Prefab/PrefabTypes.h"
#include "Prefab/IPrefabAsset.h"
#include "Prefab/IPrefabCommands.h"

#include <functional>
#include <memory>
#include <string_view>

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
class IPrefabInstancer;
}

namespace we::runtime::prefab {

struct PREFAB_API PrefabEvent {
    PrefabEventKind kind = PrefabEventKind::AssetRegistered;
    PrefabGuid asset{};
    PrefabInstanceId instance{};
    std::string detail;
};

using PrefabEventListener = std::function<void(const PrefabEvent&)>;

class PREFAB_API IPrefabEventRouter {
public:
    virtual ~IPrefabEventRouter() = default;
    virtual void Publish(const PrefabEvent& event) = 0;
    virtual void Subscribe(PrefabEventListener listener) = 0;
};

class PREFAB_API IPrefabManager {
public:
    virtual ~IPrefabManager() = default;

    [[nodiscard]] virtual IPrefabRegistry& Registry() noexcept = 0;
    [[nodiscard]] virtual IPrefabFactory& Factory() noexcept = 0;
    [[nodiscard]] virtual IPrefabSerializer& Serializer() noexcept = 0;
    [[nodiscard]] virtual IPrefabSpawner& Spawner() noexcept = 0;
    [[nodiscard]] virtual IPrefabValidator& Validator() noexcept = 0;
    [[nodiscard]] virtual IPrefabHierarchy& Hierarchy() noexcept = 0;
    [[nodiscard]] virtual IPrefabDependencyProvider& Dependencies() noexcept = 0;
    [[nodiscard]] virtual IPrefabReferenceProvider& References() noexcept = 0;
    [[nodiscard]] virtual IPrefabImporter& Importer() noexcept = 0;
    [[nodiscard]] virtual IPrefabExporter& Exporter() noexcept = 0;
    [[nodiscard]] virtual IPrefabEventRouter& Events() noexcept = 0;
    [[nodiscard]] virtual IPrefabCommandRouter& Commands() noexcept = 0;
    [[nodiscard]] virtual IPrefabThumbnailProvider& Thumbnails() noexcept = 0;
    [[nodiscard]] virtual IPrefabPreviewProvider& Previews() noexcept = 0;

    /// Apply instance overrides back to the source asset and optionally refresh instances.
    [[nodiscard]] virtual bool ApplyInstanceToAsset(PrefabInstanceId id) = 0;
    [[nodiscard]] virtual bool RevertInstance(PrefabInstanceId id) = 0;
    [[nodiscard]] virtual bool UpdateAllInstances(const PrefabGuid& guid) = 0;
    [[nodiscard]] virtual bool BreakLink(PrefabInstanceId id) = 0;
    [[nodiscard]] virtual bool RestoreLink(PrefabInstanceId id, const PrefabGuid& guid) = 0;

    /// Record a property override on an instance (Reflection path + before/after via Serialization).
    [[nodiscard]] virtual bool RecordPropertyOverride(
        PrefabInstanceId id,
        std::string_view path,
        serialization::BinaryDiff diff) = 0;
};

class PREFAB_API IPrefabRuntime {
public:
    virtual ~IPrefabRuntime() = default;

    [[nodiscard]] virtual IPrefabManager& Manager() noexcept = 0;
    [[nodiscard]] virtual const IPrefabManager& Manager() const noexcept = 0;

    /// Optional World Runtime bridge (replaces stub IPrefabInstancer behavior when set).
    [[nodiscard]] virtual world::IPrefabInstancer* MakeWorldInstancer() = 0;

    virtual void Shutdown() = 0;
};

struct PREFAB_API PrefabDependencies {
    reflection::ITypeRegistry* typeRegistry = nullptr;
    serialization::ISerializer* serializer = nullptr;
    scene::Scene* scene = nullptr;                 // near-term Editor authority
    world::IWorldRuntime* world = nullptr;         // future actor authority
    PrefabConfig config{};
    std::function<void(std::string_view)> onLog;
};

[[nodiscard]] PREFAB_API std::unique_ptr<IPrefabRuntime> CreatePrefabRuntime(
    const PrefabDependencies& deps);

} // namespace we::runtime::prefab
