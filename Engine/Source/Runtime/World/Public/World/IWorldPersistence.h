#pragma once

#include "World/Export.h"
#include "World/WorldTypes.h"

#include <cstdint>
#include <functional>
#include <future>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::serialization {
class ISerializer;
}

namespace we::runtime::reflection {
class ITypeRegistry;
}

namespace we::runtime::world {

struct WORLD_API WorldSaveOptions {
    bool includeLevels = true;
    bool includeActors = true;
    bool includeEcsScene = true;
    bool compress = false;
};

struct WORLD_API WorldLoadOptions {
    bool migrateOnVersionMismatch = true;
    bool replaceExisting = true;
};

using PersistenceProgressCallback = std::function<void(float fraction, std::string_view message)>;

/// Async world save/load through Serialization Runtime (+ ECS SceneSerializer for entity blobs).
class WORLD_API IWorldPersistence {
public:
    virtual ~IWorldPersistence() = default;

    virtual void BindSerializer(serialization::ISerializer* serializer) = 0;
    virtual void BindTypeRegistry(reflection::ITypeRegistry* registry) = 0;

    [[nodiscard]] virtual std::vector<std::uint8_t> SaveWorld(const WorldSaveOptions& options = {}) = 0;
    [[nodiscard]] virtual bool LoadWorld(std::span<const std::uint8_t> bytes, const WorldLoadOptions& options = {}) = 0;

    [[nodiscard]] virtual bool SaveWorldToFile(std::string_view path, const WorldSaveOptions& options = {}) = 0;
    [[nodiscard]] virtual bool LoadWorldFromFile(std::string_view path, const WorldLoadOptions& options = {}) = 0;

    [[nodiscard]] virtual std::future<std::vector<std::uint8_t>> SaveWorldAsync(
        WorldSaveOptions options = {},
        PersistenceProgressCallback onProgress = {}) = 0;

    [[nodiscard]] virtual std::future<bool> LoadWorldAsync(
        std::vector<std::uint8_t> bytes,
        WorldLoadOptions options = {},
        PersistenceProgressCallback onProgress = {}) = 0;
};

} // namespace we::runtime::world
