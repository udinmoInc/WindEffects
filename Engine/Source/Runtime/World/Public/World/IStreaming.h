#pragma once

#include "World/Export.h"
#include "World/WorldTypes.h"

#include <functional>
#include <future>
#include <span>
#include <string>
#include <vector>

namespace we::runtime::world {

struct WORLD_API StreamRequest {
    LevelHandle level{};
    WorldGuid levelGuid{};
    bool makeVisible = true;
    float priority = 0.f;
};

struct WORLD_API StreamProgress {
    LevelHandle level{};
    float fraction = 0.f;
    bool completed = false;
    bool failed = false;
    char message[128]{};
};

using StreamProgressCallback = std::function<void(const StreamProgress&)>;

/// Async level streaming foundation (AssetRuntime-backed when packages are mounted).
class WORLD_API IWorldStreamer {
public:
    virtual ~IWorldStreamer() = default;

    [[nodiscard]] virtual std::future<bool> LoadLevelAsync(const StreamRequest& request, StreamProgressCallback onProgress = {}) = 0;
    [[nodiscard]] virtual std::future<bool> UnloadLevelAsync(LevelHandle level, StreamProgressCallback onProgress = {}) = 0;

    [[nodiscard]] virtual bool LoadLevel(const StreamRequest& request) = 0;
    [[nodiscard]] virtual bool UnloadLevel(LevelHandle level) = 0;

    [[nodiscard]] virtual std::span<const LevelHandle> StreamingLevels() const = 0;
    virtual void Tick(float deltaSeconds) = 0;
    virtual void CancelAll() = 0;
};

struct WORLD_API PartitionCellId {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t z = 0;
    [[nodiscard]] constexpr bool operator==(const PartitionCellId& o) const noexcept {
        return x == o.x && y == o.y && z == o.z;
    }
};

struct WORLD_API PartitionSettings {
    float cellSize = 12800.f;
    std::uint32_t loadRadiusCells = 2;
    bool enabled = false;
};

/// World-partition foundation — cell grid + streaming policy hooks (replaceable strategy).
class WORLD_API IWorldPartition {
public:
    virtual ~IWorldPartition() = default;

    virtual void Configure(const PartitionSettings& settings) = 0;
    [[nodiscard]] virtual const PartitionSettings& Settings() const noexcept = 0;

    [[nodiscard]] virtual PartitionCellId WorldToCell(const Vec3f& worldPosition) const noexcept = 0;
    virtual void UpdateStreamingSource(const Vec3f& worldPosition) = 0;
    [[nodiscard]] virtual std::vector<PartitionCellId> LoadedCells() const = 0;

    virtual void Tick(float deltaSeconds) = 0;
};

} // namespace we::runtime::world
