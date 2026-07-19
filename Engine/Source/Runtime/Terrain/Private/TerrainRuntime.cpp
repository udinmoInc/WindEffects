#include "TerrainInternal.h"
#include "Terrain/TerrainDiagnostics.h"
#include "ECS/Components/CoreComponents.h"
#include "ECS/Registry.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <sstream>
#include <string>

namespace we::runtime::terrain {
namespace detail {

class GeneratorRegistry final : public ITerrainGeneratorRegistry {
public:
    [[nodiscard]] bool Register(std::shared_ptr<ITerrainGenerator> generator) override {
        if (!generator) {
            return false;
        }
        std::lock_guard lock(m_Mutex);
        m_ById[std::string(generator->GetId())] = generator;
        m_ByBuiltin[generator->GetBuiltinId()] = generator;
        return true;
    }

    [[nodiscard]] bool Unregister(std::string_view id) override {
        std::lock_guard lock(m_Mutex);
        auto it = m_ById.find(std::string(id));
        if (it == m_ById.end()) {
            return false;
        }
        m_ByBuiltin.erase(it->second->GetBuiltinId());
        m_ById.erase(it);
        return true;
    }

    [[nodiscard]] ITerrainGenerator* Find(std::string_view id) const override {
        std::lock_guard lock(m_Mutex);
        auto it = m_ById.find(std::string(id));
        return it == m_ById.end() ? nullptr : it->second.get();
    }

    [[nodiscard]] ITerrainGenerator* FindBuiltin(TerrainGeneratorId id) const override {
        std::lock_guard lock(m_Mutex);
        auto it = m_ByBuiltin.find(id);
        return it == m_ByBuiltin.end() ? nullptr : it->second.get();
    }

    [[nodiscard]] std::vector<std::string> ListIds() const override {
        std::lock_guard lock(m_Mutex);
        std::vector<std::string> out;
        out.reserve(m_ById.size());
        for (const auto& [k, _] : m_ById) {
            out.push_back(k);
        }
        return out;
    }

private:
    mutable std::mutex m_Mutex;
    std::unordered_map<std::string, std::shared_ptr<ITerrainGenerator>> m_ById;
    std::unordered_map<TerrainGeneratorId, std::shared_ptr<ITerrainGenerator>> m_ByBuiltin;
};

class EventRouter final : public ITerrainEventRouter {
public:
    void Publish(const TerrainEvent& event) override {
        std::vector<TerrainEventListener> copy;
        {
            std::lock_guard lock(m_Mutex);
            copy = m_Listeners;
        }
        for (auto& listener : copy) {
            if (listener) {
                listener(event);
            }
        }
    }

    void Subscribe(TerrainEventListener listener) override {
        std::lock_guard lock(m_Mutex);
        m_Listeners.push_back(std::move(listener));
    }

private:
    std::mutex m_Mutex;
    std::vector<TerrainEventListener> m_Listeners;
};

class ImporterService final : public ITerrainImporter {
public:
    [[nodiscard]] bool ImportHeightmapFile(
        std::string_view path,
        TerrainHeightmap& outHeightfield,
        HeightmapFormat* detected) override
    {
        const bool ok = HeightmapIO::Import(std::filesystem::path(path), outHeightfield, detected);
        if (ok) {
            TerrainDiagnostics::Get().OnHeightmapImported();
        }
        return ok;
    }

    [[nodiscard]] bool ImportRaw16(
        std::string_view path,
        int width,
        int height,
        TerrainHeightmap& outHeightfield,
        bool littleEndian) override
    {
        return HeightmapIO::ImportRaw16(
            std::filesystem::path(path), width, height, outHeightfield, littleEndian);
    }
};

class ExporterService final : public ITerrainExporter {
public:
    [[nodiscard]] bool ExportHeightmapFile(
        std::string_view path,
        const TerrainHeightmap& heightfield,
        HeightmapFormat format) override
    {
        const bool ok = HeightmapIO::Export(std::filesystem::path(path), heightfield, format);
        if (ok) {
            TerrainDiagnostics::Get().OnHeightmapExported();
        }
        return ok;
    }

    [[nodiscard]] bool ExportEditedTerrain(
        std::string_view path,
        const ITerrain& terrain,
        HeightmapFormat format) override
    {
        return ExportHeightmapFile(path, terrain.Heightfield(), format);
    }
};

class ValidatorService final : public ITerrainValidator {
public:
    [[nodiscard]] std::vector<TerrainValidationIssue> ValidateCreateInfo(
        const TerrainCreateInfo& info) const override
    {
        std::vector<TerrainValidationIssue> issues;
        if (info.resolutionX < 2 || info.resolutionY < 2) {
            issues.push_back({TerrainValidationSeverity::Error, "Resolution must be >= 2"});
        }
        if (info.chunkQuads < 1) {
            issues.push_back({TerrainValidationSeverity::Error, "Chunk size must be >= 1"});
        }
        if (info.worldSizeX <= 0.f || info.worldSizeY <= 0.f) {
            issues.push_back({TerrainValidationSeverity::Error, "World size must be positive"});
        }
        if (info.creationMethod == TerrainCreationMethod::HeightmapImport) {
            issues.push_back({
                TerrainValidationSeverity::Info,
                "Heightmap import is optional; Flat creation does not require a heightmap file"});
        }
        return issues;
    }

    [[nodiscard]] std::vector<TerrainValidationIssue> Validate(const ITerrain& terrain) const override {
        auto issues = ValidateCreateInfo(terrain.GetInfo());
        if (!terrain.IsCreated()) {
            issues.push_back({TerrainValidationSeverity::Error, "Terrain is not created", terrain.GetId()});
        }
        if (terrain.Heightfield().Empty()) {
            issues.push_back({
                TerrainValidationSeverity::Error,
                "Heightfield is empty",
                terrain.GetId()});
        }
        return issues;
    }
};

class TerrainComponentImpl final : public ITerrainComponent {
public:
    TerrainId terrainId{};
    std::uint64_t entityId = 0;
    std::string displayName = "Landscape";
    we::math::Vec3 origin{};
    we::math::Vec3 scale{1.f, 1.f, 1.f};
    class TerrainInstanceImpl* owner = nullptr;

    [[nodiscard]] TerrainId GetTerrainId() const noexcept override { return terrainId; }
    [[nodiscard]] std::uint64_t GetEntityId() const noexcept override { return entityId; }
    [[nodiscard]] std::string_view GetDisplayName() const noexcept override { return displayName; }
    void SetWorldTransform(const we::math::Vec3& o, const we::math::Vec3& s) override;
};

class TerrainInstanceImpl final : public ITerrain {
public:
    TerrainId id{};
    TerrainGuid assetGuid{};
    TerrainCreateInfo info{};
    TerrainHeightmap heightfield;
    TerrainChunkManager chunks;
    TerrainLODManager lodManager;
    TerrainCollision collision;
    TerrainMaterialSystem materials;
    TerrainBrushSystem brushes;
    TerrainStreaming streaming;
    TerrainRenderer renderer;
    TerrainFoliageSystem foliage;
    TerrainComponentImpl component;
    TerrainCollisionOptions collisionOptions{};
    TerrainStreamingOptions streamingOptions{};
    TerrainLODSettings lodSettings{};
    bool created = false;
    bool editStrokeActive = false;
    bool collisionPending = false;

    std::vector<std::unique_ptr<TerrainChunkProxy>> chunkProxies;
    std::vector<std::unique_ptr<TerrainLayerProxy>> layerProxies;

    class LodFacade final : public ITerrainLOD {
    public:
        explicit LodFacade(TerrainInstanceImpl& owner)
            : m_Owner(owner)
        {}
        void SetSettings(const TerrainLODSettings& settings) override {
            m_Owner.lodSettings = settings;
            m_Owner.lodManager.SetMaxLod(settings.maxLod);
        }
        [[nodiscard]] TerrainLODSettings GetSettings() const noexcept override {
            return m_Owner.lodSettings;
        }
        void Update(
            const we::math::Vec3& cameraWorldPos,
            const TerrainCreateInfo& createInfo,
            int heightfieldWidth) override
        {
            if (!m_Owner.lodSettings.enabled) {
                return;
            }
            m_Owner.lodManager.UpdateChunkLods(
                m_Owner.chunks, cameraWorldPos, createInfo, heightfieldWidth);
        }

    private:
        TerrainInstanceImpl& m_Owner;
    };

    class MaterialFacade final : public ITerrainMaterial {
    public:
        explicit MaterialFacade(TerrainInstanceImpl& owner)
            : m_Owner(owner)
        {}
        [[nodiscard]] int GetLayerCount() const noexcept override { return kMaxPaintLayers; }
        [[nodiscard]] ITerrainLayer* GetLayer(int index) override {
            if (index < 0 || index >= kMaxPaintLayers) {
                return nullptr;
            }
            if (m_Owner.layerProxies.empty()) {
                for (int i = 0; i < kMaxPaintLayers; ++i) {
                    m_Owner.layerProxies.push_back(
                        std::make_unique<TerrainLayerProxy>(m_Owner.materials, i));
                }
            }
            return m_Owner.layerProxies[static_cast<std::size_t>(index)].get();
        }
        void PaintWeight(int x, int z, int layerIndex, float strength, float radius) override {
            m_Owner.materials.PaintWeight(x, z, layerIndex, strength, radius);
        }
        void SetVirtualTexturingEnabled(bool enabled) override {
            m_Owner.materials.SetVirtualTexturingEnabled(enabled);
        }
        [[nodiscard]] bool WantsVirtualTexturing() const noexcept override {
            return m_Owner.materials.WantsVirtualTexturing();
        }

    private:
        TerrainInstanceImpl& m_Owner;
    };

    class CollisionFacade final : public ITerrainCollision {
    public:
        explicit CollisionFacade(TerrainInstanceImpl& owner)
            : m_Owner(owner)
        {}
        void SetOptions(const TerrainCollisionOptions& options) override {
            m_Owner.collisionOptions = options;
        }
        [[nodiscard]] TerrainCollisionOptions GetOptions() const noexcept override {
            return m_Owner.collisionOptions;
        }
        [[nodiscard]] bool Raycast(
            const we::math::Vec3& origin,
            const we::math::Vec3& direction,
            float maxDistance,
            we::math::Vec3& outHit,
            we::math::Vec3& outNormal) const override
        {
            return m_Owner.collision.Raycast(origin, direction, maxDistance, outHit, outNormal);
        }
        [[nodiscard]] bool SampleHeightNormal(
            float worldX,
            float worldZ,
            float& outHeight,
            we::math::Vec3& outNormal) const override
        {
            return m_Owner.collision.SampleHeightNormal(worldX, worldZ, outHeight, outNormal);
        }
        void RebuildDirty() override { m_Owner.collision.RebuildDirty(m_Owner.chunks); }

    private:
        TerrainInstanceImpl& m_Owner;
    };

    class StreamingFacade final : public ITerrainStreaming {
    public:
        explicit StreamingFacade(TerrainInstanceImpl& owner)
            : m_Owner(owner)
        {}
        void SetOptions(const TerrainStreamingOptions& options) override {
            m_Owner.streamingOptions = options;
            m_Owner.streaming.SetEnabled(options.enabled);
            m_Owner.streaming.SetLoadRadiusChunks(options.loadRadiusChunks);
        }
        [[nodiscard]] TerrainStreamingOptions GetOptions() const noexcept override {
            return m_Owner.streamingOptions;
        }
        void Update(const we::math::Vec3& cameraWorldPos, const TerrainFrustump* frustum) override {
            m_Owner.streaming.Update(m_Owner.chunks, cameraWorldPos, m_Owner.info, frustum);
        }
        void RequestLoad(TerrainChunkId chunkId) override { m_Owner.streaming.RequestLoad(chunkId); }
        void RequestUnload(TerrainChunkId chunkId) override {
            m_Owner.streaming.RequestUnload(chunkId);
        }

    private:
        TerrainInstanceImpl& m_Owner;
    };

    class RendererFacade final : public ITerrainRenderer {
    public:
        explicit RendererFacade(TerrainInstanceImpl& owner)
            : m_Owner(owner)
        {}
        void BindDevice(rhi::IRHIDevice* device) override { m_Owner.renderer.Init(device); }
        void SyncChunks() override { m_Owner.renderer.SyncChunks(m_Owner.chunks); }
        void DrawViewport(
            rhi::IRHICommandList& cmd,
            rhi::RHITextureHandle color,
            rhi::RHITextureHandle depth,
            rhi::Extent2D extent,
            const we::math::Mat4& view,
            const we::math::Mat4& proj,
            const we::math::Vec3& cameraPos) override
        {
            m_Owner.renderer.Draw(cmd, color, depth, extent, view, proj, cameraPos);
        }
        void SetClipmapsEnabled(bool enabled) override { m_Owner.renderer.SetClipmapsEnabled(enabled); }
        [[nodiscard]] std::uint32_t UploadedChunkCount() const noexcept override {
            return m_Owner.renderer.UploadedChunkCount();
        }
        [[nodiscard]] const TerrainRenderStats& LastStats() const noexcept override {
            return m_Owner.renderer.LastStats();
        }
        [[nodiscard]] bool IsReady() const noexcept override {
            return m_Owner.renderer.IsReady();
        }
        [[nodiscard]] bool HasDevice() const noexcept override {
            return m_Owner.renderer.HasDevice();
        }

    private:
        TerrainInstanceImpl& m_Owner;
    };

    class BrushFacade final : public ITerrainBrush {
    public:
        explicit BrushFacade(TerrainInstanceImpl& owner)
            : m_Owner(owner)
        {}
        void SetSettings(const TerrainBrushSettings& settings) override {
            m_Owner.brushes.SetSettings(settings);
        }
        [[nodiscard]] const TerrainBrushSettings& GetSettings() const noexcept override {
            return m_Owner.brushes.Settings();
        }
        void BeginStroke() override {
            m_Owner.editStrokeActive = true;
            m_Owner.collisionPending = false;
        }
        void EndStroke() override {
            m_Owner.editStrokeActive = false;
            if (m_Owner.collisionPending || m_Owner.collisionOptions.rebuildOnEdit) {
                m_Owner.RebuildDirtyCollision();
                m_Owner.collisionPending = false;
            }
        }
        [[nodiscard]] bool IsStrokeActive() const noexcept override {
            return m_Owner.editStrokeActive;
        }
        [[nodiscard]] bool ApplyAtSample(float sampleX, float sampleZ) override {
            const auto t0 = std::chrono::steady_clock::now();
            if (!m_Owner.brushes.Apply(m_Owner.heightfield, sampleX, sampleZ)) {
                return false;
            }
            int minX = 0, minZ = 0, maxX = 0, maxZ = 0;
            if (m_Owner.heightfield.ConsumeDirtyRect(minX, minZ, maxX, maxZ)) {
                m_Owner.chunks.MarkDirtyRect(minX, minZ, maxX, maxZ);
                m_Owner.materials.NotifyVirtualTexturePagesDirty(minX, minZ, maxX, maxZ);
            }
            // During stroke: dirty mesh + GPU only. Collision waits for EndStroke.
            m_Owner.RebuildDirtyMeshesAndGpu();
            if (!m_Owner.editStrokeActive) {
                m_Owner.RebuildDirtyCollision();
            } else {
                m_Owner.collisionPending = true;
            }
            const auto micros = static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - t0)
                    .count());
            TerrainDiagnostics::Get().OnBrush(micros);
            return true;
        }
        [[nodiscard]] bool ApplyAtWorld(float worldX, float worldZ) override {
            const float localX = worldX - m_Owner.info.worldOrigin.x;
            const float localZ = worldZ - m_Owner.info.worldOrigin.z;
            const float sx = (m_Owner.info.worldSizeX > 1e-6f)
                ? (localX / m_Owner.info.worldSizeX)
                    * static_cast<float>(m_Owner.info.resolutionX - 1)
                : 0.0f;
            const float sz = (m_Owner.info.worldSizeY > 1e-6f)
                ? (localZ / m_Owner.info.worldSizeY)
                    * static_cast<float>(m_Owner.info.resolutionY - 1)
                : 0.0f;
            return ApplyAtSample(sx, sz);
        }
        void SetCustomAlpha(std::span<const std::uint8_t> mask, int width, int height) override {
            m_Owner.brushes.SetCustomAlpha(mask.data(), width, height);
        }

    private:
        TerrainInstanceImpl& m_Owner;
    };

    LodFacade lodFacade{*this};
    MaterialFacade materialFacade{*this};
    CollisionFacade collisionFacade{*this};
    StreamingFacade streamingFacade{*this};
    RendererFacade rendererFacade{*this};
    BrushFacade brushFacade{*this};

    void NormalizeInfo() {
        info.resolutionX = std::max(2, info.resolutionX);
        info.resolutionY = std::max(2, info.resolutionY);
        info.chunkQuads = std::max(1, info.chunkQuads);
        const int qx = info.chunkQuads;
        if (((info.resolutionX - 1) % qx) != 0) {
            info.resolutionX = ((info.resolutionX - 1) / qx) * qx + 1;
        }
        if (((info.resolutionY - 1) % qx) != 0) {
            info.resolutionY = ((info.resolutionY - 1) / qx) * qx + 1;
        }
    }

    bool Initialize(const TerrainCreateInfo& createInfo, ITerrainGeneratorRegistry& generators) {
        info = createInfo;
        NormalizeInfo();
        lodSettings = info.lod;
        streamingOptions = info.streaming;
        collisionOptions = info.collision;
        lodManager.SetMaxLod(lodSettings.maxLod);
        streaming.SetEnabled(streamingOptions.enabled);
        streaming.SetLoadRadiusChunks(streamingOptions.loadRadiusChunks);

        const float elev = std::clamp(info.initialElevation, 0.f, 1.f);
        const auto fill = static_cast<std::uint16_t>(elev * 65535.f + 0.5f);
        heightfield.Create(info.resolutionX, info.resolutionY, fill);
        chunks.Initialize(info);
        materials.Initialize(info.resolutionX, info.resolutionY);
        collision.Bind(&heightfield, &info);

        if (!info.materialSlot0.empty()) {
            materials.Layer(0).albedoPath = info.materialSlot0;
        }
        if (!info.materialSlot1.empty()) {
            materials.Layer(1).albedoPath = info.materialSlot1;
        }
        if (!info.materialSlot2.empty()) {
            materials.Layer(2).albedoPath = info.materialSlot2;
        }
        if (!info.materialSlot3.empty()) {
            materials.Layer(3).albedoPath = info.materialSlot3;
        }

        TerrainGeneratorParams gen = info.generator;
        switch (info.creationMethod) {
        case TerrainCreationMethod::Flat:
            gen.generator = TerrainGeneratorId::Flat;
            gen.baseHeight = info.initialElevation;
            break;
        case TerrainCreationMethod::Empty:
            gen.generator = TerrainGeneratorId::Empty;
            break;
        case TerrainCreationMethod::Noise:
            gen.generator = TerrainGeneratorId::PerlinNoise;
            break;
        case TerrainCreationMethod::Fractal:
            gen.generator = TerrainGeneratorId::Fbm;
            break;
        case TerrainCreationMethod::Procedural:
        case TerrainCreationMethod::PluginGenerator:
            break;
        case TerrainCreationMethod::HeightmapImport:
            // Elevation starts flat; caller may import afterward. Never blocks creation.
            gen.generator = TerrainGeneratorId::Flat;
            gen.baseHeight = info.initialElevation;
            break;
        }
        (void)TerrainGenerators::Generate(heightfield, info, gen, &generators);

        component.terrainId = id;
        component.displayName = info.displayName.empty() ? "Landscape" : info.displayName;
        component.origin = info.worldOrigin;
        component.scale = we::math::Vec3(
            info.worldSizeX * info.worldScale.x,
            info.heightScale * info.worldScale.y,
            info.worldSizeY * info.worldScale.z);

        created = true;
        RebuildDirty();
        RebuildChunkProxies();
        return true;
    }

    void RebuildChunkProxies() {
        chunkProxies.clear();
        chunkProxies.reserve(chunks.Chunks().size());
        for (TerrainChunk& chunk : chunks.Chunks()) {
            chunkProxies.push_back(std::make_unique<TerrainChunkProxy>(chunk));
        }
    }

    void RebuildDirty() {
        std::uint64_t rebuilt = 0;
        for (TerrainChunk& chunk : chunks.Chunks()) {
            if (!chunk.meshDirty) {
                continue;
            }
            TerrainMeshCPU mesh;
            if (TerrainLODManager::BuildChunkMesh(heightfield, info, chunk, chunk.lod, mesh)) {
                chunk.mesh = std::move(mesh);
                if (!chunk.mesh.positions.empty()) {
                    we::math::Vec3 bmin = chunk.mesh.positions.front();
                    we::math::Vec3 bmax = bmin;
                    for (const we::math::Vec3& p : chunk.mesh.positions) {
                        bmin = we::math::Vec3(
                            std::min(bmin.x, p.x), std::min(bmin.y, p.y), std::min(bmin.z, p.z));
                        bmax = we::math::Vec3(
                            std::max(bmax.x, p.x), std::max(bmax.y, p.y), std::max(bmax.z, p.z));
                    }
                    chunk.bounds.min = bmin;
                    chunk.bounds.max = bmax;
                }
                chunk.meshDirty = false;
                chunk.gpuDirty = true;
                ++rebuilt;
            }
        }
        if (collisionOptions.enabled) {
            collision.RebuildDirty(chunks);
        }
        renderer.SyncChunks(chunks);
        if (rebuilt > 0) {
            TerrainDiagnostics::Get().OnChunksRebuilt(rebuilt);
        }
    }

    // ITerrain
    [[nodiscard]] TerrainId GetId() const noexcept override { return id; }
    [[nodiscard]] TerrainGuid GetAssetGuid() const noexcept override { return assetGuid; }
    [[nodiscard]] const TerrainCreateInfo& GetInfo() const noexcept override { return info; }
    [[nodiscard]] bool IsCreated() const noexcept override { return created; }
    [[nodiscard]] ITerrainComponent* GetComponent() noexcept override { return &component; }
    [[nodiscard]] ITerrainLOD& Lod() noexcept override { return lodFacade; }
    [[nodiscard]] ITerrainMaterial& Materials() noexcept override { return materialFacade; }
    [[nodiscard]] ITerrainCollision& Collision() noexcept override { return collisionFacade; }
    [[nodiscard]] ITerrainStreaming& Streaming() noexcept override { return streamingFacade; }
    [[nodiscard]] ITerrainRenderer& Renderer() noexcept override { return rendererFacade; }
    [[nodiscard]] ITerrainBrush& Brush() noexcept override { return brushFacade; }
    [[nodiscard]] TerrainHeightmap& Heightfield() noexcept override { return heightfield; }
    [[nodiscard]] const TerrainHeightmap& Heightfield() const noexcept override { return heightfield; }

    [[nodiscard]] int GetChunkCount() const noexcept override {
        return static_cast<int>(chunks.Chunks().size());
    }
    [[nodiscard]] ITerrainChunk* GetChunk(int index) noexcept override {
        if (index < 0 || index >= GetChunkCount()) {
            return nullptr;
        }
        if (chunkProxies.size() != chunks.Chunks().size()) {
            const_cast<TerrainInstanceImpl*>(this)->RebuildChunkProxies();
        }
        return chunkProxies[static_cast<std::size_t>(index)].get();
    }

    [[nodiscard]] bool ApplyGenerator(const TerrainGeneratorParams& params) override {
        if (!created) {
            return false;
        }
        const auto t0 = std::chrono::steady_clock::now();
        const bool ok = TerrainGenerators::Generate(heightfield, info, params, nullptr);
        chunks.MarkAllDirty();
        RebuildDirty();
        const auto micros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - t0)
                .count());
        TerrainDiagnostics::Get().OnGenerated(micros);
        return ok;
    }

    [[nodiscard]] std::shared_ptr<ITerrainAsset> CaptureAsset(std::string_view displayName) const override {
        TerrainAssetDesc desc;
        desc.displayName = std::string(displayName);
        desc.createInfo = info;
        desc.guidHi = assetGuid.hi;
        desc.guidLo = assetGuid.lo;
        desc.streamingEnabled = streamingOptions.enabled;
        desc.maxResidentChunks = streamingOptions.maxResidentChunks;
        desc.materialSlot0 = materials.Layer(0).albedoPath;
        TerrainAsset asset(std::move(desc));
        if (!heightfield.Empty()) {
            std::vector<std::uint16_t> samples(
                heightfield.Data(), heightfield.Data() + heightfield.SampleCount());
            asset.SetHeightSamples(std::move(samples), heightfield.Width(), heightfield.Height());
        }
        return std::make_shared<TerrainAsset>(std::move(asset));
    }

    [[nodiscard]] bool ApplyAsset(const ITerrainAsset& asset) override {
        if (!asset.IsValid()) {
            return false;
        }
        info = asset.GetCreateInfo();
        NormalizeInfo();
        heightfield.Create(info.resolutionX, info.resolutionY, kMidHeightSample);
        chunks.Initialize(info);
        materials.Initialize(info.resolutionX, info.resolutionY);
        collision.Bind(&heightfield, &info);
        const auto samples = asset.GetElevationSamples();
        if (samples.size() == heightfield.SampleCount()) {
            std::copy(samples.begin(), samples.end(), heightfield.Data());
            heightfield.MarkRegionDirty(0, 0, heightfield.Width() - 1, heightfield.Height() - 1);
            chunks.MarkAllDirty();
        }
        created = true;
        RebuildDirty();
        RebuildChunkProxies();
        assetGuid = asset.GetGuid();
        return true;
    }

    [[nodiscard]] std::vector<std::uint16_t> CaptureElevationSamples() const override {
        if (heightfield.Empty()) {
            return {};
        }
        return std::vector<std::uint16_t>(
            heightfield.Data(), heightfield.Data() + heightfield.SampleCount());
    }

    [[nodiscard]] bool RestoreElevationSamples(const std::vector<std::uint16_t>& samples) override {
        if (!created || samples.size() != heightfield.SampleCount()) {
            return false;
        }
        std::copy(samples.begin(), samples.end(), heightfield.Data());
        heightfield.MarkRegionDirty(0, 0, heightfield.Width() - 1, heightfield.Height() - 1);
        chunks.MarkAllDirty();
        RebuildDirty();
        return true;
    }

    void Tick(
        float /*deltaSeconds*/,
        const we::math::Vec3& cameraWorldPos,
        const we::math::Mat4* viewProj) override
    {
        if (!created) {
            return;
        }
        TerrainFrustump frustum{};
        const TerrainFrustump* frustumPtr = nullptr;
        if (viewProj) {
            frustum = TerrainFrustump::FromViewProj(*viewProj);
            frustumPtr = &frustum;
        }
        lodFacade.Update(cameraWorldPos, info, heightfield.Width());
        streamingFacade.Update(cameraWorldPos, frustumPtr);
        bool anyDirty = false;
        for (const TerrainChunk& chunk : chunks.Chunks()) {
            if (chunk.meshDirty) {
                anyDirty = true;
                break;
            }
        }
        if (anyDirty) {
            RebuildDirty();
        } else {
            renderer.SyncChunks(chunks);
        }
    }
};

class SessionImpl final : public ITerrainSession {
public:
    void BindManager(ITerrainManager* manager) override { m_Manager = manager; }
    [[nodiscard]] ITerrainManager* Manager() noexcept override { return m_Manager; }
    void SetTransactionRecorder(TransactionFn recorder) override {
        m_Recorder = std::move(recorder);
    }

    [[nodiscard]] bool BeginEditStroke(TerrainId id) override {
        if (!m_Manager) {
            return false;
        }
        ITerrain* terrain = m_Manager->Find(id);
        if (!terrain) {
            return false;
        }
        m_StrokeId = id;
        m_StrokeBefore = terrain->CaptureElevationSamples();
        m_StrokeActive = !m_StrokeBefore.empty();
        return m_StrokeActive;
    }

    [[nodiscard]] bool EndEditStroke(TerrainId id) override {
        if (!m_StrokeActive || m_StrokeId.value != id.value || !m_Manager) {
            m_StrokeActive = false;
            return false;
        }
        m_StrokeActive = false;
        ITerrain* terrain = m_Manager->Find(id);
        if (!terrain) {
            return false;
        }
        auto after = terrain->CaptureElevationSamples();
        if (after == m_StrokeBefore) {
            m_StrokeBefore.clear();
            return false;
        }
        auto before = std::move(m_StrokeBefore);
        m_StrokeBefore.clear();
        if (!m_Recorder) {
            return true;
        }
        return m_Recorder(
            "Terrain Brush",
            [before, id, mgr = m_Manager]() {
                ITerrain* t = mgr->Find(id);
                return t && t->RestoreElevationSamples(before);
            },
            [after = std::move(after), id, mgr = m_Manager]() {
                ITerrain* t = mgr->Find(id);
                return t && t->RestoreElevationSamples(after);
            });
    }

    [[nodiscard]] bool ApplyBrushWithUndo(TerrainId id, float worldX, float worldZ) override {
        if (!m_Manager) {
            return false;
        }
        ITerrain* terrain = m_Manager->Find(id);
        if (!terrain) {
            return false;
        }
        if (!m_StrokeActive) {
            if (!BeginEditStroke(id)) {
                return false;
            }
            const bool changed = terrain->Brush().ApplyAtWorld(worldX, worldZ);
            (void)EndEditStroke(id);
            return changed;
        }
        return terrain->Brush().ApplyAtWorld(worldX, worldZ);
    }

private:
    ITerrainManager* m_Manager = nullptr;
    TransactionFn m_Recorder;
    TerrainId m_StrokeId{};
    std::vector<std::uint16_t> m_StrokeBefore;
    bool m_StrokeActive = false;
};

class ManagerImpl final : public ITerrainManager {
public:
    explicit ManagerImpl(TerrainDependencies deps)
        : m_Deps(std::move(deps))
    {
        TerrainGenerators::RegisterBuiltins(m_Generators);
        m_Scene = m_Deps.scene;
        if (m_Deps.device) {
            m_Device = m_Deps.device;
        }
    }

    [[nodiscard]] ITerrainGeneratorRegistry& Generators() noexcept override { return m_Generators; }
    [[nodiscard]] ITerrainImporter& Importer() noexcept override { return m_Importer; }
    [[nodiscard]] ITerrainExporter& Exporter() noexcept override { return m_Exporter; }
    [[nodiscard]] ITerrainValidator& Validator() noexcept override { return m_Validator; }
    [[nodiscard]] ITerrainEventRouter& Events() noexcept override { return m_Events; }

    [[nodiscard]] TerrainId CreateLandscape(const TerrainCreateInfo& info) override {
        const auto issues = m_Validator.ValidateCreateInfo(info);
        for (const auto& issue : issues) {
            if (issue.severity == TerrainValidationSeverity::Error) {
                if (m_Deps.onLog) {
                    m_Deps.onLog(issue.message);
                }
                return {};
            }
        }

        auto instance = std::make_unique<TerrainInstanceImpl>();
        instance->id = NextTerrainId();
        instance->assetGuid = GenerateTerrainGuid();
        if (!instance->Initialize(info, m_Generators)) {
            return {};
        }
        if (m_Device) {
            instance->renderer.Init(m_Device);
        }

        const TerrainId id = instance->id;
        {
            std::lock_guard lock(m_Mutex);
            if (m_Terrains.size() >= m_Deps.config.maxTerrains) {
                return {};
            }
            m_Terrains[id] = std::move(instance);
            m_Active = id;
        }
        TerrainDiagnostics::Get().OnCreated();
        m_Events.Publish(TerrainEvent{TerrainEventKind::Created, id, {}, "CreateLandscape"});
        return id;
    }

    [[nodiscard]] bool DestroyLandscape(TerrainId id) override {
        std::lock_guard lock(m_Mutex);
        auto it = m_Terrains.find(id);
        if (it == m_Terrains.end()) {
            return false;
        }
        m_Terrains.erase(it);
        if (m_Active.value == id.value) {
            m_Active = m_Terrains.empty() ? TerrainId{} : m_Terrains.begin()->first;
        }
        TerrainDiagnostics::Get().OnDestroyed();
        m_Events.Publish(TerrainEvent{TerrainEventKind::Destroyed, id, {}, "DestroyLandscape"});
        return true;
    }

    [[nodiscard]] ITerrain* Find(TerrainId id) noexcept override {
        std::lock_guard lock(m_Mutex);
        auto it = m_Terrains.find(id);
        return it == m_Terrains.end() ? nullptr : it->second.get();
    }

    [[nodiscard]] const ITerrain* Find(TerrainId id) const noexcept override {
        std::lock_guard lock(m_Mutex);
        auto it = m_Terrains.find(id);
        return it == m_Terrains.end() ? nullptr : it->second.get();
    }

    [[nodiscard]] ITerrain* GetActive() noexcept override { return Find(m_Active); }

    void SetActive(TerrainId id) override {
        std::lock_guard lock(m_Mutex);
        if (m_Terrains.contains(id)) {
            m_Active = id;
        }
    }

    [[nodiscard]] std::vector<TerrainId> ListAll() const override {
        std::lock_guard lock(m_Mutex);
        std::vector<TerrainId> out;
        out.reserve(m_Terrains.size());
        for (const auto& [id, _] : m_Terrains) {
            out.push_back(id);
        }
        return out;
    }

    [[nodiscard]] std::uint64_t SpawnLandscapeActor(TerrainId id, const char* name) override {
        if (!m_Scene) {
            HE_ERROR("[Terrain] Cannot spawn Landscape — scene is not bound.");
            return 0;
        }
        ITerrain* terrain = Find(id);
        if (!terrain) {
            HE_ERROR("[Terrain] Cannot spawn Landscape — terrain id is invalid.");
            return 0;
        }
        auto* impl = static_cast<TerrainInstanceImpl*>(terrain);
        using we::runtime::scene::EntityType;
        const char* actorName = name ? name : impl->component.displayName.c_str();
        if (m_Scene->HasEntityOfType(EntityType::Landscape)) {
            const int index = m_Scene->FindEntityIndexByName(actorName);
            if (index >= 0) {
                we::runtime::scene::Entity& e =
                    m_Scene->GetEntities()[static_cast<std::size_t>(index)];
                BindLandscapeEntity(*impl, e);
                return impl->component.entityId;
            }
        }
        m_Scene->CreateEntity(actorName, EntityType::Landscape);
        auto& entities = m_Scene->GetEntities();
        if (entities.empty()) {
            HE_ERROR("[Terrain] Landscape entity creation failed.");
            return 0;
        }
        we::runtime::scene::Entity& e = entities.back();
        BindLandscapeEntity(*impl, e);
        m_Scene->SetSelectedEntityIndex(static_cast<int>(entities.size()) - 1);
        m_Scene->SyncViewToEcs();
        return impl->component.entityId;
    }

    void BindLandscapeEntity(TerrainInstanceImpl& impl, we::runtime::scene::Entity& e) {
        // Mesh vertices are already world-space; keep actor transform as identity scale
        // centered on the terrain surface so Focus/FrameSelected lands on visible geometry.
        const float midY = impl.info.heightOffset
            + impl.info.initialElevation * impl.info.heightScale * impl.info.worldScale.y;
        e.Position = impl.info.worldOrigin
            + we::math::Vec3(
                impl.info.worldSizeX * 0.5f,
                midY,
                impl.info.worldSizeY * 0.5f);
        e.Scale = we::math::Vec3(1.0f, 1.0f, 1.0f);
        e.Color = we::math::Vec4(0.35f, 0.55f, 0.28f, 1.0f);
        impl.component.entityId = e.Id;
        impl.component.origin = impl.info.worldOrigin;
        impl.component.scale = we::math::Vec3(1.0f, 1.0f, 1.0f);

        // Keep every chunk resident/visible immediately after create.
        for (TerrainChunk& chunk : impl.chunks.Chunks()) {
            chunk.visible = true;
            impl.streaming.RequestLoad(chunk.id);
        }
        impl.RebuildDirty();

        we::runtime::ecs::TerrainComponent terrainComp{};
        terrainComp.enabled = true;
        terrainComp.landscapeId = impl.id.value;
        terrainComp.resolutionX = impl.info.resolutionX;
        terrainComp.resolutionY = impl.info.resolutionY;
        m_Scene->Registry().Replace(
            we::runtime::ecs::Entity{e.Id},
            terrainComp);

        we::runtime::ecs::TransformComponent transform{};
        transform.localPosition = e.Position;
        transform.localRotation = e.Rotation;
        transform.localScale = e.Scale;
        transform.dirty = true;
        m_Scene->Registry().Replace(we::runtime::ecs::Entity{e.Id}, transform);
    }

    void BindScene(scene::Scene* scene) override { m_Scene = scene; }

    void BindRenderer(rhi::IRHIDevice* device) override {
        m_Device = device;
        std::lock_guard lock(m_Mutex);
        for (auto& [_, terrain] : m_Terrains) {
            terrain->renderer.Init(device);
        }
    }

    void Tick(
        float deltaSeconds,
        const we::math::Vec3& cameraWorldPos,
        const we::math::Mat4* viewProj) override
    {
        std::vector<TerrainInstanceImpl*> copy;
        {
            std::lock_guard lock(m_Mutex);
            copy.reserve(m_Terrains.size());
            for (auto& [_, t] : m_Terrains) {
                copy.push_back(t.get());
            }
        }
        for (TerrainInstanceImpl* t : copy) {
            t->Tick(deltaSeconds, cameraWorldPos, viewProj);
        }
    }

    [[nodiscard]] TerrainInstanceImpl* FindImpl(TerrainId id) {
        return static_cast<TerrainInstanceImpl*>(Find(id));
    }

private:
    TerrainDependencies m_Deps{};
    GeneratorRegistry m_Generators;
    ImporterService m_Importer;
    ExporterService m_Exporter;
    ValidatorService m_Validator;
    EventRouter m_Events;
    mutable std::mutex m_Mutex;
    std::unordered_map<TerrainId, std::unique_ptr<TerrainInstanceImpl>, TerrainIdHash> m_Terrains;
    TerrainId m_Active{};
    scene::Scene* m_Scene = nullptr;
    rhi::IRHIDevice* m_Device = nullptr;
};

class RuntimeImpl final : public ITerrainRuntime {
public:
    explicit RuntimeImpl(TerrainDependencies deps)
        : m_Manager(std::move(deps))
    {
        m_Session.BindManager(&m_Manager);
    }

    [[nodiscard]] ITerrainManager& Manager() noexcept override { return m_Manager; }
    [[nodiscard]] const ITerrainManager& Manager() const noexcept override { return m_Manager; }
    [[nodiscard]] ITerrainSession& Session() noexcept override { return m_Session; }

    void Shutdown() override {
        for (TerrainId id : m_Manager.ListAll()) {
            (void)m_Manager.DestroyLandscape(id);
        }
    }

    ManagerImpl& ManagerImplRef() { return m_Manager; }

private:
    ManagerImpl m_Manager;
    SessionImpl m_Session;
};

ActiveTerrainAccess QueryActiveTerrainAccess() {
    ActiveTerrainAccess access{};
    auto* terrain = GetDefaultTerrainRuntime().Manager().GetActive();
    if (!terrain) {
        return access;
    }
    auto* impl = static_cast<TerrainInstanceImpl*>(terrain);
    access.id = impl->id;
    access.created = impl->created;
    access.info = &impl->info;
    access.heightfield = &impl->heightfield;
    access.chunks = &impl->chunks;
    access.lod = &impl->lodManager;
    access.collision = &impl->collision;
    access.materials = &impl->materials;
    access.brushes = &impl->brushes;
    access.streaming = &impl->streaming;
    access.renderer = &impl->renderer;
    access.foliage = &impl->foliage;
    access.entityId = &impl->component.entityId;
    access.instance = impl;
    access.rebuildDirty = [](void* ctx) {
        static_cast<TerrainInstanceImpl*>(ctx)->RebuildDirty();
    };
    access.applyGenerator = [](void* ctx, const TerrainGeneratorParams& params) {
        return static_cast<TerrainInstanceImpl*>(ctx)->ApplyGenerator(params);
    };
    access.captureAsset = [](void* ctx, std::string_view name) {
        return static_cast<TerrainInstanceImpl*>(ctx)->CaptureAsset(name);
    };
    access.applyAsset = [](void* ctx, const ITerrainAsset& asset) {
        return static_cast<TerrainInstanceImpl*>(ctx)->ApplyAsset(asset);
    };
    access.captureSamples = [](void* ctx) {
        return static_cast<TerrainInstanceImpl*>(ctx)->CaptureElevationSamples();
    };
    access.restoreSamples = [](void* ctx, const std::vector<std::uint16_t>& samples) {
        return static_cast<TerrainInstanceImpl*>(ctx)->RestoreElevationSamples(samples);
    };
    access.brushWorld = [](void* ctx, float x, float z) {
        return static_cast<TerrainInstanceImpl*>(ctx)->Brush().ApplyAtWorld(x, z);
    };
    return access;
}

} // namespace detail

namespace {
std::unique_ptr<ITerrainRuntime> g_DefaultRuntime;
std::mutex g_DefaultMutex;
} // namespace

std::unique_ptr<ITerrainRuntime> CreateTerrainRuntime(const TerrainDependencies& deps) {
    return std::make_unique<detail::RuntimeImpl>(deps);
}

ITerrainRuntime& GetDefaultTerrainRuntime() {
    std::lock_guard lock(g_DefaultMutex);
    if (!g_DefaultRuntime) {
        g_DefaultRuntime = CreateTerrainRuntime({});
    }
    return *g_DefaultRuntime;
}

void SetDefaultTerrainRuntime(std::unique_ptr<ITerrainRuntime> runtime) {
    std::lock_guard lock(g_DefaultMutex);
    g_DefaultRuntime = std::move(runtime);
}

} // namespace we::runtime::terrain
