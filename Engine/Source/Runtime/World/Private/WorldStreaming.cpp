#include "WorldInternal.h"

#include "ECS/Serialization/SceneSerializer.h"
#include "Serialization/ISerializer.h"
#include "Reflection/ITypeRegistry.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <cstring>

namespace we::runtime::world::detail {
namespace {

constexpr char kWorldMagic[4] = {'W', 'E', 'W', 'D'};
constexpr std::uint32_t kWorldFileVersion = 1;

struct WorldFileHeader {
    char magic[4]{};
    std::uint32_t version = kWorldFileVersion;
    std::uint32_t descriptorBytes = 0;
    std::uint32_t ecsBlobBytes = 0;
};

void AppendBytes(std::vector<std::uint8_t>& out, const void* data, std::size_t size) {
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    out.insert(out.end(), bytes, bytes + size);
}

} // namespace

WorldStreamerImpl::WorldStreamerImpl(WorldImpl& world) : m_World(world) {}

std::future<bool> WorldStreamerImpl::LoadLevelAsync(const StreamRequest& request, StreamProgressCallback onProgress) {
    return std::async(std::launch::async, [this, request, onProgress]() {
        if (onProgress) {
            StreamProgress p{};
            p.level = request.level;
            p.fraction = 0.f;
            onProgress(p);
        }
        const bool ok = LoadLevel(request);
        if (onProgress) {
            StreamProgress p{};
            p.level = request.level;
            p.fraction = 1.f;
            p.completed = ok;
            p.failed = !ok;
            onProgress(p);
        }
        return ok;
    });
}

std::future<bool> WorldStreamerImpl::UnloadLevelAsync(LevelHandle level, StreamProgressCallback onProgress) {
    return std::async(std::launch::async, [this, level, onProgress]() {
        const bool ok = UnloadLevel(level);
        if (onProgress) {
            StreamProgress p{};
            p.level = level;
            p.fraction = 1.f;
            p.completed = ok;
            p.failed = !ok;
            onProgress(p);
        }
        return ok;
    });
}

bool WorldStreamerImpl::LoadLevel(const StreamRequest& request) {
    LevelImpl* level = nullptr;
    if (request.level.IsValid()) {
        level = m_World.GetLevel(request.level);
    } else if (!request.levelGuid.IsNil()) {
        level = dynamic_cast<LevelImpl*>(m_World.FindLevelByGuid(request.levelGuid));
    }
    if (!level) {
        LevelDescriptor desc{};
        desc.guid = request.levelGuid.IsNil() ? WorldGuid::Generate() : request.levelGuid;
        CopyName(desc.name, "StreamedLevel");
        desc.worldGuid = m_World.Guid();
        desc.streamable = true;
        LevelHandle handle = m_World.CreateLevel(desc);
        level = m_World.GetLevel(handle);
    }
    if (!level) {
        return false;
    }
    m_World.m_State = WorldState::Streaming;
    const bool ok = level->Load();
    level->SetVisible(request.makeVisible);
    if (std::find(m_Streaming.begin(), m_Streaming.end(), level->handle) == m_Streaming.end()) {
        m_Streaming.push_back(level->handle);
    }
    m_World.m_State = WorldState::Ready;
    return ok;
}

bool WorldStreamerImpl::UnloadLevel(LevelHandle levelHandle) {
    LevelImpl* level = m_World.GetLevel(levelHandle);
    if (!level || level->persistent) {
        return false;
    }
    const bool ok = level->Unload();
    m_Streaming.erase(std::remove(m_Streaming.begin(), m_Streaming.end(), levelHandle), m_Streaming.end());
    return ok;
}

std::span<const LevelHandle> WorldStreamerImpl::StreamingLevels() const { return m_Streaming; }
void WorldStreamerImpl::Tick(float) {}
void WorldStreamerImpl::CancelAll() { m_Streaming.clear(); }

WorldPartitionImpl::WorldPartitionImpl(WorldImpl& world) : m_World(world) {}

void WorldPartitionImpl::Configure(const PartitionSettings& settings) {
    m_Settings = settings;
}

PartitionCellId WorldPartitionImpl::WorldToCell(const Vec3f& worldPosition) const noexcept {
    const float size = m_Settings.cellSize > 0.f ? m_Settings.cellSize : 1.f;
    PartitionCellId cell{};
    cell.x = static_cast<std::int32_t>(std::floor(worldPosition.x / size));
    cell.y = static_cast<std::int32_t>(std::floor(worldPosition.y / size));
    cell.z = static_cast<std::int32_t>(std::floor(worldPosition.z / size));
    return cell;
}

void WorldPartitionImpl::UpdateStreamingSource(const Vec3f& worldPosition) {
    m_Source = worldPosition;
    if (!m_Settings.enabled) {
        return;
    }
    m_Loaded.clear();
    const PartitionCellId center = WorldToCell(worldPosition);
    const std::int32_t r = static_cast<std::int32_t>(m_Settings.loadRadiusCells);
    for (std::int32_t z = center.z - r; z <= center.z + r; ++z) {
        for (std::int32_t y = center.y - r; y <= center.y + r; ++y) {
            for (std::int32_t x = center.x - r; x <= center.x + r; ++x) {
                m_Loaded.push_back(PartitionCellId{x, y, z});
            }
        }
    }
}

std::vector<PartitionCellId> WorldPartitionImpl::LoadedCells() const { return m_Loaded; }
void WorldPartitionImpl::Tick(float) {}

WorldPersistenceImpl::WorldPersistenceImpl(WorldImpl& world) : m_World(world) {}

void WorldPersistenceImpl::BindSerializer(serialization::ISerializer* serializer) {
    m_Serializer = serializer;
}

void WorldPersistenceImpl::BindTypeRegistry(reflection::ITypeRegistry* registry) {
    m_Registry = registry;
}

std::vector<std::uint8_t> WorldPersistenceImpl::SaveWorld(const WorldSaveOptions& options) {
    m_World.m_State = WorldState::Saving;
    std::vector<std::uint8_t> out;

    WorldFileHeader header{};
    std::memcpy(header.magic, kWorldMagic, 4);
    header.version = kWorldFileVersion;
    header.descriptorBytes = static_cast<std::uint32_t>(sizeof(WorldDescriptor));

    std::vector<std::uint8_t> ecsBlob;
    if (options.includeEcsScene) {
        if (scene::Scene* scene = m_World.ActiveScene()) {
            const ecs::SerializedScene captured = ecs::SceneSerializer::Capture(scene->Registry());
            // Encode a compact count + per-entity name lengths for foundation roundtrip.
            // Full binary fidelity remains SceneSerializer file path; here we persist entity count + descriptors.
            const std::uint32_t count = static_cast<std::uint32_t>(captured.entities.size());
            AppendBytes(ecsBlob, &count, sizeof(count));
            for (const ecs::SerializedEntity& ent : captured.entities) {
                const std::uint32_t nameLen = static_cast<std::uint32_t>(ent.name.size());
                AppendBytes(ecsBlob, &nameLen, sizeof(nameLen));
                if (nameLen > 0) {
                    AppendBytes(ecsBlob, ent.name.data(), nameLen);
                }
                AppendBytes(ecsBlob, ent.uuid.bytes.data(), ent.uuid.bytes.size());
            }
        }
    }
    header.ecsBlobBytes = static_cast<std::uint32_t>(ecsBlob.size());

    AppendBytes(out, &header, sizeof(header));
    AppendBytes(out, &m_World.m_Descriptor, sizeof(WorldDescriptor));
    out.insert(out.end(), ecsBlob.begin(), ecsBlob.end());

    // Optional Reflection/Serialization document for descriptor when serializer is bound.
    if (m_Serializer && m_Registry) {
        // Keep envelope primary; serializer path is available for future typed documents.
        (void)options;
    }

    WorldDiagnostics::Get().OnSave();
    m_World.m_State = WorldState::Ready;
    return out;
}

bool WorldPersistenceImpl::LoadWorld(std::span<const std::uint8_t> bytes, const WorldLoadOptions& options) {
    if (bytes.size() < sizeof(WorldFileHeader) + sizeof(WorldDescriptor)) {
        return false;
    }
    WorldFileHeader header{};
    std::memcpy(&header, bytes.data(), sizeof(header));
    if (std::memcmp(header.magic, kWorldMagic, 4) != 0 || header.version != kWorldFileVersion) {
        return false;
    }
    if (bytes.size() < sizeof(header) + header.descriptorBytes) {
        return false;
    }

    WorldDescriptor desc{};
    std::memcpy(&desc, bytes.data() + sizeof(header), sizeof(WorldDescriptor));
    if (options.replaceExisting) {
        m_World.m_Descriptor = desc;
    }

    WorldDiagnostics::Get().OnLoad();
    return true;
}

bool WorldPersistenceImpl::SaveWorldToFile(std::string_view path, const WorldSaveOptions& options) {
    const auto bytes = SaveWorld(options);
    std::ofstream file(std::string(path), std::ios::binary);
    if (!file) {
        return false;
    }
    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(file);
}

bool WorldPersistenceImpl::LoadWorldFromFile(std::string_view path, const WorldLoadOptions& options) {
    std::ifstream file(std::string(path), std::ios::binary | std::ios::ate);
    if (!file) {
        return false;
    }
    const auto size = file.tellg();
    if (size <= 0) {
        return false;
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(bytes.data()), size);
    return LoadWorld(bytes, options);
}

std::future<std::vector<std::uint8_t>> WorldPersistenceImpl::SaveWorldAsync(
    WorldSaveOptions options,
    PersistenceProgressCallback onProgress) {
    return std::async(std::launch::async, [this, options, onProgress]() {
        if (onProgress) {
            onProgress(0.f, "Saving");
        }
        auto bytes = SaveWorld(options);
        if (onProgress) {
            onProgress(1.f, "Saved");
        }
        return bytes;
    });
}

std::future<bool> WorldPersistenceImpl::LoadWorldAsync(
    std::vector<std::uint8_t> bytes,
    WorldLoadOptions options,
    PersistenceProgressCallback onProgress) {
    return std::async(std::launch::async, [this, bytes = std::move(bytes), options, onProgress]() {
        if (onProgress) {
            onProgress(0.f, "Loading");
        }
        const bool ok = LoadWorld(bytes, options);
        if (onProgress) {
            onProgress(1.f, ok ? "Loaded" : "Failed");
        }
        return ok;
    });
}

PrefabInstancerImpl::PrefabInstancerImpl(WorldImpl& world) : m_World(world) {}

ActorHandle PrefabInstancerImpl::Instantiate(const PrefabInstanceParams& params) {
    ActorSpawnParams spawn = params.spawn;
    if (spawn.guid.IsNil()) {
        spawn.guid = WorldGuid::Generate();
    }
    if (spawn.name[0] == '\0') {
        CopyName(spawn.name, "PrefabInstance");
    }
    return m_World.m_Actors.Spawn(spawn, params.level);
}

bool PrefabInstancerImpl::RegisterPrefabSource(const WorldGuid& prefabGuid, std::string_view assetPath) {
    if (prefabGuid.IsNil()) {
        return false;
    }
    m_Sources[prefabGuid] = std::string(assetPath);
    return true;
}

bool PrefabInstancerImpl::UnregisterPrefabSource(const WorldGuid& prefabGuid) {
    return m_Sources.erase(prefabGuid) > 0;
}

OriginRebasingImpl::OriginRebasingImpl(WorldImpl& world) : m_World(world) {}

void OriginRebasingImpl::SetEnabled(bool enabled) { m_Enabled = enabled; }
bool OriginRebasingImpl::IsEnabled() const noexcept { return m_Enabled; }
Vec3f OriginRebasingImpl::CurrentOrigin() const noexcept { return m_Origin; }

void OriginRebasingImpl::RequestRebase(const Vec3f& newOrigin) {
    if (!m_Enabled) {
        return;
    }
    m_Pending = newOrigin;
    m_HasPending = true;
}

void OriginRebasingImpl::ApplyPendingRebase() {
    if (!m_Enabled || !m_HasPending) {
        return;
    }
    const Vec3f oldOrigin = m_Origin;
    const Vec3f delta{
        m_Pending.x - m_Origin.x,
        m_Pending.y - m_Origin.y,
        m_Pending.z - m_Origin.z};
    for (ActorHandle h : m_World.m_Actors.All()) {
        if (ActorImpl* a = m_World.GetActor(h); a && !a->parent.IsValid()) {
            a->localPosition.x -= delta.x;
            a->localPosition.y -= delta.y;
            a->localPosition.z -= delta.z;
        }
    }
    m_Origin = m_Pending;
    m_HasPending = false;
    m_World.m_Transforms.SyncToEcs();
    if (m_Callback) {
        m_Callback(oldOrigin, m_Origin, m_UserData);
    }
}

void OriginRebasingImpl::SetRebaseCallback(RebaseCallback callback, void* userData) {
    m_Callback = callback;
    m_UserData = userData;
}

} // namespace we::runtime::world::detail
