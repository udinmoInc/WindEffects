#include "Terrain/TerrainSystem.h"
#include "TerrainInternal.h"
#include "Core/Logger.h"

#include <algorithm>
#include <string>

namespace we::runtime::terrain {

TerrainSystem& TerrainSystem::Get() {
    static TerrainSystem instance;
    return instance;
}

bool TerrainSystem::Create(const TerrainCreateInfo& info) {
    auto& mgr = GetDefaultTerrainRuntime().Manager();
    if (const TerrainId active = ActiveTerrainId(); active.IsValid()) {
        (void)mgr.DestroyLandscape(active);
    }
    TerrainCreateInfo create = info;
    if (create.creationMethod == TerrainCreationMethod::HeightmapImport) {
        // Still create an editable landscape first; heightmap import is optional afterward.
        create.creationMethod = TerrainCreationMethod::Flat;
    }
    const TerrainId id = mgr.CreateLandscape(create);
    if (!id.IsValid()) {
        return false;
    }
    mgr.SetActive(id);
    HE_INFO(
        "[Terrain] Landscape created "
        + std::to_string(create.resolutionX) + "x" + std::to_string(create.resolutionY)
        + " without requiring a heightmap file.");
    return true;
}

void TerrainSystem::Destroy() {
    if (const TerrainId id = ActiveTerrainId(); id.IsValid()) {
        (void)GetDefaultTerrainRuntime().Manager().DestroyLandscape(id);
    }
}

bool TerrainSystem::IsCreated() const {
    return detail::QueryActiveTerrainAccess().created;
}

void TerrainSystem::BindScene(we::runtime::scene::Scene* scene) {
    GetDefaultTerrainRuntime().Manager().BindScene(scene);
}

void TerrainSystem::BindRenderer(we::rhi::IRHIDevice* device) {
    GetDefaultTerrainRuntime().Manager().BindRenderer(device);
}

std::uint64_t TerrainSystem::SpawnLandscapeActor(const char* name) {
    auto& mgr = GetDefaultTerrainRuntime().Manager();
    TerrainId id = ActiveTerrainId();
    if (!id.IsValid()) {
        id = mgr.CreateLandscape(TerrainCreateInfo{});
        mgr.SetActive(id);
    }
    return mgr.SpawnLandscapeActor(id, name);
}

const TerrainCreateInfo& TerrainSystem::Info() const {
    static TerrainCreateInfo empty{};
    auto access = detail::QueryActiveTerrainAccess();
    return access.info ? *access.info : empty;
}

TerrainHeightmap& TerrainSystem::Heightmap() {
    return *detail::QueryActiveTerrainAccess().heightfield;
}

const TerrainHeightmap& TerrainSystem::Heightmap() const {
    return *detail::QueryActiveTerrainAccess().heightfield;
}

TerrainChunkManager& TerrainSystem::Chunks() {
    return *detail::QueryActiveTerrainAccess().chunks;
}
TerrainLODManager& TerrainSystem::Lod() {
    return *detail::QueryActiveTerrainAccess().lod;
}
TerrainCollision& TerrainSystem::Collision() {
    return *detail::QueryActiveTerrainAccess().collision;
}
TerrainMaterialSystem& TerrainSystem::Materials() {
    return *detail::QueryActiveTerrainAccess().materials;
}
TerrainBrushSystem& TerrainSystem::Brushes() {
    return *detail::QueryActiveTerrainAccess().brushes;
}
TerrainStreaming& TerrainSystem::Streaming() {
    return *detail::QueryActiveTerrainAccess().streaming;
}
TerrainRenderer& TerrainSystem::Renderer() {
    return *detail::QueryActiveTerrainAccess().renderer;
}
TerrainFoliageSystem& TerrainSystem::Foliage() {
    return *detail::QueryActiveTerrainAccess().foliage;
}

bool TerrainSystem::ImportHeightmap(const std::filesystem::path& path) {
    auto& mgr = GetDefaultTerrainRuntime().Manager();
    TerrainHeightmap imported;
    if (!mgr.Importer().ImportHeightmapFile(path.string(), imported, nullptr)) {
        return false;
    }
    TerrainCreateInfo info = IsCreated() ? Info() : TerrainCreateInfo{};
    info.resolutionX = imported.Width();
    info.resolutionY = imported.Height();
    info.creationMethod = TerrainCreationMethod::Flat;
    if (!Create(info)) {
        return false;
    }
    auto access = detail::QueryActiveTerrainAccess();
    *access.heightfield = std::move(imported);
    if (access.heightfield->Width() != access.info->resolutionX
        || access.heightfield->Height() != access.info->resolutionY)
    {
        TerrainHeightmap resized;
        resized.Create(access.info->resolutionX, access.info->resolutionY, kMidHeightSample);
        for (int z = 0; z < resized.Height(); ++z) {
            for (int x = 0; x < resized.Width(); ++x) {
                const float u =
                    static_cast<float>(x) / static_cast<float>(std::max(1, resized.Width() - 1));
                const float v =
                    static_cast<float>(z) / static_cast<float>(std::max(1, resized.Height() - 1));
                const float n = access.heightfield->SampleNormalized(u, v);
                resized.Set(
                    x, z, static_cast<std::uint16_t>(std::clamp(n * 65535.0f, 0.0f, 65535.0f)));
            }
        }
        *access.heightfield = std::move(resized);
    }
    access.collision->Bind(access.heightfield, access.info);
    access.heightfield->MarkRegionDirty(
        0, 0, access.heightfield->Width() - 1, access.heightfield->Height() - 1);
    access.chunks->MarkAllDirty();
    if (access.rebuildDirty) {
        access.rebuildDirty(access.instance);
    }
    return true;
}

bool TerrainSystem::ExportHeightmap(const std::filesystem::path& path, HeightmapFormat format) {
    if (!IsCreated()) {
        return false;
    }
    return GetDefaultTerrainRuntime().Manager().Exporter().ExportHeightmapFile(
        path.string(), Heightmap(), format);
}

bool TerrainSystem::GenerateProcedural(const ProceduralHeightmapParams& params) {
    return ApplyGenerator(params);
}

bool TerrainSystem::ApplyGenerator(const TerrainGeneratorParams& params) {
    if (!IsCreated() && !Create(TerrainCreateInfo{})) {
        return false;
    }
    auto access = detail::QueryActiveTerrainAccess();
    return access.applyGenerator && access.applyGenerator(access.instance, params);
}

TerrainAsset TerrainSystem::CaptureAsset(std::string_view displayName) const {
    if (!IsCreated()) {
        return TerrainAsset{};
    }
    auto access = detail::QueryActiveTerrainAccess();
    if (!access.captureAsset) {
        return TerrainAsset{};
    }
    auto shared = access.captureAsset(access.instance, displayName);
    if (auto* concrete = dynamic_cast<TerrainAsset*>(shared.get())) {
        return *concrete;
    }
    return TerrainAsset{};
}

bool TerrainSystem::ApplyAsset(const TerrainAsset& asset) {
    if (!asset.IsValid()) {
        return false;
    }
    if (!Create(asset.Desc().createInfo)) {
        return false;
    }
    auto access = detail::QueryActiveTerrainAccess();
    return access.applyAsset && access.applyAsset(access.instance, asset);
}

std::vector<std::uint16_t> TerrainSystem::CaptureHeightSamples() const {
    if (!IsCreated()) {
        return {};
    }
    auto access = detail::QueryActiveTerrainAccess();
    return access.captureSamples ? access.captureSamples(access.instance)
                                 : std::vector<std::uint16_t>{};
}

bool TerrainSystem::RestoreHeightSamples(const std::vector<std::uint16_t>& samples) {
    if (!IsCreated()) {
        return false;
    }
    auto access = detail::QueryActiveTerrainAccess();
    return access.restoreSamples && access.restoreSamples(access.instance, samples);
}

bool TerrainSystem::ApplyBrushWorld(float worldX, float worldZ) {
    if (!IsCreated()) {
        return false;
    }
    auto access = detail::QueryActiveTerrainAccess();
    return access.brushWorld && access.brushWorld(access.instance, worldX, worldZ);
}

void TerrainSystem::Tick(
    float deltaSeconds,
    const we::math::Vec3& cameraWorldPos,
    const we::math::Mat4* viewProj)
{
    GetDefaultTerrainRuntime().Manager().Tick(deltaSeconds, cameraWorldPos, viewProj);
}

std::uint64_t TerrainSystem::LandscapeEntityId() const {
    auto access = detail::QueryActiveTerrainAccess();
    return access.entityId ? *access.entityId : 0;
}

TerrainId TerrainSystem::ActiveTerrainId() const noexcept {
    return detail::QueryActiveTerrainAccess().id;
}

} // namespace we::runtime::terrain



