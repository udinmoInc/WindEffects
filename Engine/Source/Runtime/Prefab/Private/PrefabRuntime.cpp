#include "PrefabInternal.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <iomanip>

namespace we::runtime::prefab {
namespace detail {
namespace {

[[nodiscard]] std::uint64_t NowMicros() {
    using clock = std::chrono::steady_clock;
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(clock::now().time_since_epoch()).count());
}

[[nodiscard]] std::string Escape(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '\\' || c == '|') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
    return out;
}

[[nodiscard]] std::string Unescape(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            out.push_back(s[++i]);
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}

void WriteNode(std::ostringstream& oss, const PrefabNodeTemplate& node, int depth) {
    const std::string indent(static_cast<std::size_t>(depth * 2), ' ');
    oss << indent << "NODE|" << Escape(node.name) << '|' << Escape(node.typeName) << '|'
        << node.position.x << ',' << node.position.y << ',' << node.position.z << '|'
        << node.rotation.x << ',' << node.rotation.y << ',' << node.rotation.z << '|'
        << node.scale.x << ',' << node.scale.y << ',' << node.scale.z << '|'
        << node.propertySnapshot.size() << '\n';
    if (!node.propertySnapshot.empty()) {
        oss << indent << "SNAP|";
        for (std::uint8_t b : node.propertySnapshot) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
        }
        oss << std::dec << '\n';
    }
    for (const auto& child : node.children) {
        WriteNode(oss, child, depth + 1);
    }
    oss << indent << "ENDNODE\n";
}

[[nodiscard]] bool ParseVec3(std::string_view text, we::math::Vec3& out) {
    float x = 0.f, y = 0.f, z = 0.f;
    if (std::sscanf(std::string(text).c_str(), "%f,%f,%f", &x, &y, &z) != 3) {
        return false;
    }
    out = we::math::Vec3{x, y, z};
    return true;
}

[[nodiscard]] std::vector<std::string> SplitPipe(std::string_view line) {
    std::vector<std::string> parts;
    std::string cur;
    for (std::size_t i = 0; i < line.size(); ++i) {
        if (line[i] == '\\' && i + 1 < line.size()) {
            cur.push_back(line[++i]);
            continue;
        }
        if (line[i] == '|') {
            parts.push_back(cur);
            cur.clear();
            continue;
        }
        cur.push_back(line[i]);
    }
    parts.push_back(cur);
    return parts;
}

bool ReadNodeBody(std::istream& in, PrefabNodeTemplate& node);

bool FillNodeFromHeader(std::istream& in, const std::string& trimmed, PrefabNodeTemplate& node) {
    auto parts = SplitPipe(trimmed);
    if (parts.size() < 7) {
        return false;
    }
    node.name = parts[1];
    node.typeName = parts[2];
    if (!ParseVec3(parts[3], node.position) || !ParseVec3(parts[4], node.rotation) ||
        !ParseVec3(parts[5], node.scale)) {
        return false;
    }
    const auto snapSize = static_cast<std::size_t>(std::strtoull(parts[6].c_str(), nullptr, 10));
    if (snapSize == 0) {
        return true;
    }
    std::string snapLine;
    if (!std::getline(in, snapLine)) {
        return false;
    }
    const auto snapFirst = snapLine.find_first_not_of(' ');
    if (snapFirst == std::string::npos || snapLine.substr(snapFirst).rfind("SNAP|", 0) != 0) {
        return false;
    }
    const std::string hex = snapLine.substr(snapFirst + 5);
    node.propertySnapshot.reserve(snapSize);
    for (std::size_t i = 0; i + 1 < hex.size() && node.propertySnapshot.size() < snapSize; i += 2) {
        unsigned int v = 0;
        std::sscanf(hex.substr(i, 2).c_str(), "%02x", &v);
        node.propertySnapshot.push_back(static_cast<std::uint8_t>(v));
    }
    return true;
}

bool ReadNodeBody(std::istream& in, PrefabNodeTemplate& node) {
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        const auto first = line.find_first_not_of(' ');
        if (first == std::string::npos) {
            continue;
        }
        const std::string trimmed = line.substr(first);
        if (trimmed.rfind("NODE|", 0) == 0) {
            PrefabNodeTemplate child;
            if (!FillNodeFromHeader(in, trimmed, child) || !ReadNodeBody(in, child)) {
                return false;
            }
            node.children.push_back(std::move(child));
            continue;
        }
        if (trimmed == "ENDNODE") {
            return true;
        }
        return false;
    }
    return false;
}

bool ReadCompleteNode(std::istream& in, PrefabNodeTemplate& node) {
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        const auto first = line.find_first_not_of(' ');
        if (first == std::string::npos) {
            continue;
        }
        const std::string trimmed = line.substr(first);
        if (trimmed.rfind("NODE|", 0) != 0) {
            return false;
        }
        return FillNodeFromHeader(in, trimmed, node) && ReadNodeBody(in, node);
    }
    return false;
}

} // namespace

class EventRouterImpl final : public IPrefabEventRouter {
public:
    void Publish(const PrefabEvent& event) override {
        std::vector<PrefabEventListener> copy;
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

    void Subscribe(PrefabEventListener listener) override {
        std::lock_guard lock(m_Mutex);
        m_Listeners.push_back(std::move(listener));
    }

private:
    std::mutex m_Mutex;
    std::vector<PrefabEventListener> m_Listeners;
};

class RegistryImpl final : public IPrefabRegistry {
public:
    bool Register(std::shared_ptr<IPrefabAsset> asset) override {
        if (!asset || !asset->GetGuid().IsValid()) {
            return false;
        }
        std::lock_guard lock(m_Mutex);
        m_ByGuid[asset->GetGuid()] = asset;
        if (!asset->GetAssetPath().empty()) {
            m_ByPath[std::string(asset->GetAssetPath())] = asset->GetGuid();
        }
        PrefabDiagnostics::Get().OnAssetRegistered();
        return true;
    }

    bool Unregister(const PrefabGuid& guid) override {
        std::lock_guard lock(m_Mutex);
        auto it = m_ByGuid.find(guid);
        if (it == m_ByGuid.end()) {
            return false;
        }
        if (!it->second->GetAssetPath().empty()) {
            m_ByPath.erase(std::string(it->second->GetAssetPath()));
        }
        m_ByGuid.erase(it);
        return true;
    }

    std::shared_ptr<IPrefabAsset> Find(const PrefabGuid& guid) const override {
        std::lock_guard lock(m_Mutex);
        const auto it = m_ByGuid.find(guid);
        return it == m_ByGuid.end() ? nullptr : it->second;
    }

    std::shared_ptr<IPrefabAsset> FindByPath(std::string_view path) const override {
        std::lock_guard lock(m_Mutex);
        const auto it = m_ByPath.find(std::string(path));
        if (it == m_ByPath.end()) {
            return nullptr;
        }
        const auto git = m_ByGuid.find(it->second);
        return git == m_ByGuid.end() ? nullptr : git->second;
    }

    std::vector<PrefabGuid> ListAll() const override {
        std::lock_guard lock(m_Mutex);
        std::vector<PrefabGuid> out;
        out.reserve(m_ByGuid.size());
        for (const auto& [guid, _] : m_ByGuid) {
            out.push_back(guid);
        }
        return out;
    }

private:
    mutable std::mutex m_Mutex;
    std::unordered_map<PrefabGuid, std::shared_ptr<IPrefabAsset>, PrefabGuidHash> m_ByGuid;
    std::unordered_map<std::string, PrefabGuid> m_ByPath;
};

class SerializerImplV2 final : public IPrefabSerializer {
public:
    bool SaveToFile(const IPrefabAsset& asset, std::string_view path) override {
        const auto t0 = NowMicros();
        const auto bytes = SerializeBytes(asset);
        std::ofstream out(std::string(path), std::ios::binary | std::ios::trunc);
        if (!out) {
            return false;
        }
        out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        PrefabDiagnostics::Get().OnSerialize(NowMicros() - t0);
        return static_cast<bool>(out);
    }

    std::shared_ptr<IPrefabAsset> LoadFromFile(std::string_view path) override {
        std::ifstream in(std::string(path), std::ios::binary);
        if (!in) {
            return nullptr;
        }
        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        return DeserializeBytes(bytes, path);
    }

    std::vector<std::uint8_t> SerializeBytes(const IPrefabAsset& asset) override {
        std::ostringstream oss;
        oss << "WEPREFAB 1\n";
        oss << "GUID|" << asset.GetGuid().hi << '|' << asset.GetGuid().lo << '\n';
        oss << "NAME|" << Escape(asset.GetName()) << '\n';
        oss << "PATH|" << Escape(asset.GetAssetPath()) << '\n';
        oss << "VERSION|" << asset.GetVersion() << '\n';
        for (const auto& nested : asset.GetNestedPrefabs()) {
            oss << "NESTED|" << nested.hi << '|' << nested.lo << '\n';
        }
        oss << "BEGINROOT\n";
        WriteNode(oss, asset.GetRoot(), 0);
        oss << "ENDROOT\n";
        const std::string text = oss.str();
        return std::vector<std::uint8_t>(text.begin(), text.end());
    }

    std::shared_ptr<IPrefabAsset> DeserializeBytes(
        std::span<const std::uint8_t> bytes,
        std::string_view pathHint) override
    {
        std::string text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        std::istringstream in(text);
        std::string line;
        if (!std::getline(in, line) || line.rfind("WEPREFAB", 0) != 0) {
            return nullptr;
        }

        auto asset = std::make_shared<PrefabAssetImpl>();
        if (!pathHint.empty()) {
            asset->assetPath = std::string(pathHint);
        }

        while (std::getline(in, line)) {
            if (line.rfind("GUID|", 0) == 0) {
                auto parts = SplitPipe(line);
                if (parts.size() >= 3) {
                    asset->guid.hi = std::strtoull(parts[1].c_str(), nullptr, 10);
                    asset->guid.lo = std::strtoull(parts[2].c_str(), nullptr, 10);
                }
            } else if (line.rfind("NAME|", 0) == 0) {
                asset->name = Unescape(line.substr(5));
            } else if (line.rfind("PATH|", 0) == 0) {
                const auto path = Unescape(line.substr(5));
                if (!path.empty()) {
                    asset->assetPath = path;
                }
            } else if (line.rfind("VERSION|", 0) == 0) {
                asset->version = static_cast<std::uint32_t>(std::strtoul(line.c_str() + 8, nullptr, 10));
            } else if (line.rfind("NESTED|", 0) == 0) {
                auto parts = SplitPipe(line);
                if (parts.size() >= 3) {
                    PrefabGuid g;
                    g.hi = std::strtoull(parts[1].c_str(), nullptr, 10);
                    g.lo = std::strtoull(parts[2].c_str(), nullptr, 10);
                    asset->nested.push_back(g);
                }
            } else if (line == "BEGINROOT") {
                if (!ReadCompleteNode(in, asset->root)) {
                    return nullptr;
                }
            } else if (line == "ENDROOT") {
                break;
            }
        }

        if (!asset->guid.IsValid()) {
            return nullptr;
        }
        return asset;
    }
};

class FactoryImpl final : public IPrefabFactory {
public:
    explicit FactoryImpl(scene::Scene* scene)
        : m_Scene(scene)
    {}

    PrefabGuid GenerateGuid() const override { return GeneratePrefabGuid(); }

    std::shared_ptr<IPrefabAsset> CreateFromEntity(
        std::uint64_t rootEntityId,
        std::string_view name,
        std::string_view assetPath) override
    {
        if (!m_Scene || rootEntityId == 0) {
            return nullptr;
        }
        const scene::Entity* root = m_Scene->FindEntityById(rootEntityId);
        if (!root) {
            return nullptr;
        }

        auto asset = std::make_shared<PrefabAssetImpl>();
        asset->guid = GeneratePrefabGuid();
        asset->name = name.empty() ? root->Name : std::string(name);
        asset->assetPath = std::string(assetPath);
        asset->version = 1;
        asset->root = CaptureNode(*root);
        asset->dirty = true;
        return asset;
    }

private:
    PrefabNodeTemplate CaptureNode(const scene::Entity& entity) const {
        PrefabNodeTemplate node;
        node.name = entity.Name;
        node.typeName = EntityTypeToName(entity.Type);
        node.position = entity.Position;
        node.rotation = entity.Rotation;
        node.scale = entity.Scale;

        const auto children = m_Scene->FindChildIndices(entity.Id);
        for (int idx : children) {
            const auto& entities = m_Scene->GetEntities();
            if (idx < 0 || static_cast<std::size_t>(idx) >= entities.size()) {
                continue;
            }
            node.children.push_back(CaptureNode(entities[static_cast<std::size_t>(idx)]));
        }
        return node;
    }

    scene::Scene* m_Scene = nullptr;
};

class SpawnerImpl final : public IPrefabSpawner {
public:
    SpawnerImpl(scene::Scene* scene, IPrefabRegistry& registry, IPrefabEventRouter& events)
        : m_Scene(scene)
        , m_Registry(registry)
        , m_Events(events)
    {}

    PrefabInstanceId Spawn(const PrefabSpawnParams& params) override {
        if (!m_Scene || !params.prefab.IsValid()) {
            return {};
        }
        auto asset = m_Registry.Find(params.prefab);
        if (!asset) {
            return {};
        }

        const auto t0 = NowMicros();
        auto instance = std::make_unique<PrefabInstanceImpl>();
        instance->id = PrefabInstanceId{++m_NextId};
        instance->source = params.prefab;
        instance->link = PrefabLinkState::Linked;

        const auto& rootTemplate = asset->GetRoot();
        PrefabNodeTemplate spawnRoot = rootTemplate;
        if (!params.instanceName.empty()) {
            spawnRoot.name = params.instanceName;
        }
        spawnRoot.position = params.position;
        spawnRoot.rotation = params.rotation;
        spawnRoot.scale = params.scale;

        const std::uint64_t rootId = SpawnNode(spawnRoot, params.parentEntityId, *instance);
        instance->rootEntityId = rootId;

        const PrefabInstanceId id = instance->id;
        {
            std::lock_guard lock(m_Mutex);
            m_Instances[id] = std::move(instance);
            m_ByAsset[params.prefab].push_back(id);
        }

        PrefabDiagnostics::Get().OnSpawn(NowMicros() - t0);
        PrefabEvent ev;
        ev.kind = PrefabEventKind::InstanceSpawned;
        ev.asset = params.prefab;
        ev.instance = id;
        m_Events.Publish(ev);
        return id;
    }

    bool Despawn(PrefabInstanceId id) override {
        std::unique_ptr<PrefabInstanceImpl> owned;
        {
            std::lock_guard lock(m_Mutex);
            auto it = m_Instances.find(id);
            if (it == m_Instances.end()) {
                return false;
            }
            owned = std::move(it->second);
            m_Instances.erase(it);
            auto& list = m_ByAsset[owned->source];
            list.erase(std::remove(list.begin(), list.end(), id), list.end());
        }

        if (m_Scene) {
            for (auto it = owned->entityIds.rbegin(); it != owned->entityIds.rend(); ++it) {
                const int idx = m_Scene->FindEntityIndexById(*it);
                if (idx >= 0) {
                    m_Scene->DestroyEntity(static_cast<std::size_t>(idx));
                }
            }
        }
        PrefabDiagnostics::Get().OnDespawn();
        PrefabEvent ev;
        ev.kind = PrefabEventKind::InstanceDestroyed;
        ev.asset = owned->source;
        ev.instance = id;
        m_Events.Publish(ev);
        return true;
    }

    IPrefabInstance* FindInstance(PrefabInstanceId id) override {
        std::lock_guard lock(m_Mutex);
        auto it = m_Instances.find(id);
        return it == m_Instances.end() ? nullptr : it->second.get();
    }

    std::vector<PrefabInstanceId> ListInstances(const PrefabGuid& guid) const override {
        std::lock_guard lock(m_Mutex);
        const auto it = m_ByAsset.find(guid);
        return it == m_ByAsset.end() ? std::vector<PrefabInstanceId>{} : it->second;
    }

    PrefabInstanceImpl* FindMutable(PrefabInstanceId id) {
        std::lock_guard lock(m_Mutex);
        auto it = m_Instances.find(id);
        return it == m_Instances.end() ? nullptr : it->second.get();
    }

    std::vector<PrefabInstanceId> AllIds() const {
        std::lock_guard lock(m_Mutex);
        std::vector<PrefabInstanceId> out;
        out.reserve(m_Instances.size());
        for (const auto& [id, _] : m_Instances) {
            out.push_back(id);
        }
        return out;
    }

private:
    std::uint64_t SpawnNode(
        const PrefabNodeTemplate& node,
        std::uint64_t parentId,
        PrefabInstanceImpl& instance)
    {
        const auto type = EntityTypeFromName(node.typeName);
        m_Scene->CreateEntity(node.name, type);
        auto& entities = m_Scene->GetEntities();
        if (entities.empty()) {
            return 0;
        }
        scene::Entity& created = entities.back();
        created.Position = node.position;
        created.Rotation = node.rotation;
        created.Scale = node.scale;
        created.ParentId = parentId;
        instance.entityIds.push_back(created.Id);
        const std::uint64_t id = created.Id;
        for (const auto& child : node.children) {
            SpawnNode(child, id, instance);
        }
        return id;
    }

    scene::Scene* m_Scene = nullptr;
    IPrefabRegistry& m_Registry;
    IPrefabEventRouter& m_Events;
    mutable std::mutex m_Mutex;
    std::uint64_t m_NextId = 0;
    std::unordered_map<PrefabInstanceId, std::unique_ptr<PrefabInstanceImpl>, PrefabInstanceIdHash> m_Instances;
    std::unordered_map<PrefabGuid, std::vector<PrefabInstanceId>, PrefabGuidHash> m_ByAsset;
};

class HierarchyImpl final : public IPrefabHierarchy {
public:
    explicit HierarchyImpl(SpawnerImpl& spawner)
        : m_Spawner(spawner)
    {}

    PrefabInstanceId GetParentInstance(PrefabInstanceId id) const noexcept override {
        if (auto* inst = m_Spawner.FindInstance(id)) {
            return static_cast<const PrefabInstanceImpl*>(inst)->parentInstance;
        }
        return {};
    }

    std::vector<PrefabInstanceId> GetChildInstances(PrefabInstanceId id) const override {
        std::vector<PrefabInstanceId> out;
        for (const auto childId : m_Spawner.AllIds()) {
            if (GetParentInstance(childId) == id) {
                out.push_back(childId);
            }
        }
        return out;
    }

    std::uint16_t GetNestingDepth(PrefabInstanceId id) const noexcept override {
        std::uint16_t depth = 0;
        PrefabInstanceId cur = id;
        while (true) {
            const auto parent = GetParentInstance(cur);
            if (!parent.IsValid()) {
                break;
            }
            ++depth;
            cur = parent;
            if (depth > 1024) {
                break;
            }
        }
        return depth;
    }

private:
    SpawnerImpl& m_Spawner;
};

class DependencyProviderImpl final : public IPrefabDependencyProvider {
public:
    explicit DependencyProviderImpl(IPrefabRegistry& registry)
        : m_Registry(registry)
    {}

    std::vector<PrefabGuid> GetDependencies(const PrefabGuid& guid) const override {
        auto asset = m_Registry.Find(guid);
        if (!asset) {
            return {};
        }
        std::vector<PrefabGuid> out(asset->GetNestedPrefabs().begin(), asset->GetNestedPrefabs().end());
        return out;
    }

private:
    IPrefabRegistry& m_Registry;
};

class ReferenceProviderImpl final : public IPrefabReferenceProvider {
public:
    explicit ReferenceProviderImpl(SpawnerImpl& spawner)
        : m_Spawner(spawner)
    {}

    std::vector<PrefabInstanceId> GetReferencers(const PrefabGuid& guid) const override {
        return m_Spawner.ListInstances(guid);
    }

private:
    SpawnerImpl& m_Spawner;
};

class ValidatorImpl final : public IPrefabValidator {
public:
    ValidatorImpl(
        IPrefabRegistry& registry,
        SpawnerImpl& spawner,
        IPrefabDependencyProvider& deps,
        const PrefabConfig& config)
        : m_Registry(registry)
        , m_Spawner(spawner)
        , m_Deps(deps)
        , m_Config(config)
    {}

    std::vector<PrefabValidationIssue> ValidateAsset(const PrefabGuid& guid) const override {
        std::vector<PrefabValidationIssue> issues;
        auto asset = m_Registry.Find(guid);
        if (!asset) {
            PrefabValidationIssue issue;
            issue.severity = PrefabValidationIssue::Severity::Error;
            issue.message = "Missing prefab asset";
            issue.asset = guid;
            issues.push_back(std::move(issue));
            return issues;
        }
        if (asset->GetRoot().name.empty()) {
            PrefabValidationIssue issue;
            issue.severity = PrefabValidationIssue::Severity::Warning;
            issue.message = "Prefab root has empty name";
            issue.asset = guid;
            issues.push_back(std::move(issue));
        }
        for (const auto& nested : asset->GetNestedPrefabs()) {
            if (!m_Registry.Find(nested)) {
                PrefabValidationIssue issue;
                issue.severity = PrefabValidationIssue::Severity::Error;
                issue.message = "Broken nested prefab reference";
                issue.asset = guid;
                issues.push_back(std::move(issue));
            }
        }
        if (m_Config.detectCircularDependencies && HasCircularDependency(guid)) {
            PrefabValidationIssue issue;
            issue.severity = PrefabValidationIssue::Severity::Error;
            issue.message = "Circular prefab dependency detected";
            issue.asset = guid;
            issues.push_back(std::move(issue));
        }
        return issues;
    }

    std::vector<PrefabValidationIssue> ValidateInstance(PrefabInstanceId id) const override {
        std::vector<PrefabValidationIssue> issues;
        auto* inst = m_Spawner.FindInstance(id);
        if (!inst) {
            PrefabValidationIssue issue;
            issue.severity = PrefabValidationIssue::Severity::Error;
            issue.message = "Missing prefab instance";
            issue.instance = id;
            issues.push_back(std::move(issue));
            return issues;
        }
        if (!m_Registry.Find(inst->GetSourceGuid())) {
            PrefabValidationIssue issue;
            issue.severity = PrefabValidationIssue::Severity::Error;
            issue.message = "Instance source asset missing";
            issue.instance = id;
            issue.asset = inst->GetSourceGuid();
            issues.push_back(std::move(issue));
        }
        return issues;
    }

    bool HasCircularDependency(const PrefabGuid& guid) const override {
        std::unordered_set<PrefabGuid, PrefabGuidHash> visiting;
        std::unordered_set<PrefabGuid, PrefabGuidHash> visited;
        return Dfs(guid, visiting, visited);
    }

private:
    bool Dfs(
        const PrefabGuid& guid,
        std::unordered_set<PrefabGuid, PrefabGuidHash>& visiting,
        std::unordered_set<PrefabGuid, PrefabGuidHash>& visited) const
    {
        if (visited.contains(guid)) {
            return false;
        }
        if (visiting.contains(guid)) {
            return true;
        }
        visiting.insert(guid);
        for (const auto& dep : m_Deps.GetDependencies(guid)) {
            if (Dfs(dep, visiting, visited)) {
                return true;
            }
        }
        visiting.erase(guid);
        visited.insert(guid);
        return false;
    }

    IPrefabRegistry& m_Registry;
    SpawnerImpl& m_Spawner;
    IPrefabDependencyProvider& m_Deps;
    PrefabConfig m_Config;
};

class ImporterImpl final : public IPrefabImporter {
public:
    ImporterImpl(IPrefabSerializer& serializer, IPrefabRegistry& registry)
        : m_Serializer(serializer)
        , m_Registry(registry)
    {}

    std::shared_ptr<IPrefabAsset> Import(std::string_view sourcePath) override {
        auto asset = m_Serializer.LoadFromFile(sourcePath);
        if (asset) {
            (void)m_Registry.Register(asset);
        }
        return asset;
    }

private:
    IPrefabSerializer& m_Serializer;
    IPrefabRegistry& m_Registry;
};

class ExporterImpl final : public IPrefabExporter {
public:
    ExporterImpl(IPrefabSerializer& serializer, IPrefabRegistry& registry)
        : m_Serializer(serializer)
        , m_Registry(registry)
    {}

    bool Export(const PrefabGuid& guid, std::string_view destinationPath) override {
        auto asset = m_Registry.Find(guid);
        return asset && m_Serializer.SaveToFile(*asset, destinationPath);
    }

private:
    IPrefabSerializer& m_Serializer;
    IPrefabRegistry& m_Registry;
};

class ThumbnailProviderImpl final : public IPrefabThumbnailProvider {
public:
    explicit ThumbnailProviderImpl(IPrefabRegistry& registry)
        : m_Registry(registry)
    {}

    PrefabThumbnailResult Generate(const PrefabThumbnailRequest& request) override {
        PrefabThumbnailResult result;
        result.width = request.width;
        result.height = request.height;
        auto asset = m_Registry.Find(request.asset);
        if (!asset) {
            return result;
        }
        const std::size_t pixels = static_cast<std::size_t>(request.width) * request.height;
        result.rgba.assign(pixels * 4, 0);
        // Deterministic procedural tint from guid — full renderer integration later.
        const std::uint8_t r = static_cast<std::uint8_t>((request.asset.hi >> 0) & 0xFF);
        const std::uint8_t g = static_cast<std::uint8_t>((request.asset.hi >> 8) & 0xFF);
        const std::uint8_t b = static_cast<std::uint8_t>((request.asset.lo >> 0) & 0xFF);
        for (std::size_t i = 0; i < pixels; ++i) {
            result.rgba[i * 4 + 0] = r;
            result.rgba[i * 4 + 1] = g;
            result.rgba[i * 4 + 2] = b;
            result.rgba[i * 4 + 3] = 255;
        }
        result.success = true;
        return result;
    }

private:
    IPrefabRegistry& m_Registry;
};

class PreviewProviderImpl final : public IPrefabPreviewProvider {
public:
    PreviewProviderImpl(IPrefabRegistry& registry, IPrefabValidator& validator)
        : m_Registry(registry)
        , m_Validator(validator)
    {}

    PrefabPreviewResult BuildPreview(const PrefabPreviewRequest& request) override {
        PrefabPreviewResult result;
        auto asset = m_Registry.Find(request.asset);
        if (!asset) {
            return result;
        }
        result.root = asset->GetRoot();
        if (!request.includeChildren) {
            result.root.children.clear();
        }
        result.issues = m_Validator.ValidateAsset(request.asset);
        result.success = true;
        return result;
    }

private:
    IPrefabRegistry& m_Registry;
    IPrefabValidator& m_Validator;
};

class WorldPrefabInstancerBridge final : public world::IPrefabInstancer {
public:
    WorldPrefabInstancerBridge(IPrefabSpawner& spawner, IPrefabRegistry& registry)
        : m_Spawner(spawner)
        , m_Registry(registry)
    {}

    world::ActorHandle Instantiate(const world::PrefabInstanceParams& params) override {
        PrefabSpawnParams spawn;
        spawn.prefab = WorldGuidToPrefabGuid(params.prefabGuid);
        spawn.position = we::math::Vec3{
            params.spawn.localPosition[0],
            params.spawn.localPosition[1],
            params.spawn.localPosition[2]};
        spawn.rotation = we::math::Vec3{
            params.spawn.localRotation[0],
            params.spawn.localRotation[1],
            params.spawn.localRotation[2]};
        spawn.scale = we::math::Vec3{
            params.spawn.localScale[0],
            params.spawn.localScale[1],
            params.spawn.localScale[2]};
        if (params.spawn.name[0] != '\0') {
            spawn.instanceName = params.spawn.name;
        }
        const auto id = m_Spawner.Spawn(spawn);
        world::ActorHandle handle{};
        if (id.IsValid()) {
            handle.index = static_cast<std::uint32_t>(id.value & 0xFFFFFFFFu);
            handle.generation = 1;
        }
        return handle;
    }

    bool RegisterPrefabSource(const world::WorldGuid& prefabGuid, std::string_view assetPath) override {
        if (auto existing = m_Registry.FindByPath(assetPath)) {
            return existing->GetGuid() == WorldGuidToPrefabGuid(prefabGuid);
        }
        return false;
    }

    bool UnregisterPrefabSource(const world::WorldGuid& prefabGuid) override {
        return m_Registry.Unregister(WorldGuidToPrefabGuid(prefabGuid));
    }

private:
    IPrefabSpawner& m_Spawner;
    IPrefabRegistry& m_Registry;
};

class ManagerImpl;

class CommandRouterImpl final : public IPrefabCommandRouter {
public:
    explicit CommandRouterImpl(ManagerImpl& manager)
        : m_Manager(manager)
    {}

    bool Execute(PrefabCommandId id, const PrefabCommandContext& ctx) override;
    bool CanExecute(PrefabCommandId id, const PrefabCommandContext& ctx) const override;

private:
    ManagerImpl& m_Manager;
};

class ManagerImpl final : public IPrefabManager {
public:
    ManagerImpl(PrefabDependencies deps, EventRouterImpl& events)
        : m_Deps(std::move(deps))
        , m_Events(events)
        , m_Registry()
        , m_Serializer()
        , m_Factory(m_Deps.scene)
        , m_Spawner(m_Deps.scene, m_Registry, m_Events)
        , m_Hierarchy(m_Spawner)
        , m_Dependencies(m_Registry)
        , m_References(m_Spawner)
        , m_Validator(m_Registry, m_Spawner, m_Dependencies, m_Deps.config)
        , m_Importer(m_Serializer, m_Registry)
        , m_Exporter(m_Serializer, m_Registry)
        , m_Commands(*this)
        , m_Thumbnails(m_Registry)
        , m_Previews(m_Registry, m_Validator)
    {}

    IPrefabRegistry& Registry() noexcept override { return m_Registry; }
    IPrefabFactory& Factory() noexcept override { return m_Factory; }
    IPrefabSerializer& Serializer() noexcept override { return m_Serializer; }
    IPrefabSpawner& Spawner() noexcept override { return m_Spawner; }
    IPrefabValidator& Validator() noexcept override { return m_Validator; }
    IPrefabHierarchy& Hierarchy() noexcept override { return m_Hierarchy; }
    IPrefabDependencyProvider& Dependencies() noexcept override { return m_Dependencies; }
    IPrefabReferenceProvider& References() noexcept override { return m_References; }
    IPrefabImporter& Importer() noexcept override { return m_Importer; }
    IPrefabExporter& Exporter() noexcept override { return m_Exporter; }
    IPrefabEventRouter& Events() noexcept override { return m_Events; }
    IPrefabCommandRouter& Commands() noexcept override { return m_Commands; }
    IPrefabThumbnailProvider& Thumbnails() noexcept override { return m_Thumbnails; }
    IPrefabPreviewProvider& Previews() noexcept override { return m_Previews; }

    bool ApplyInstanceToAsset(PrefabInstanceId id) override {
        auto* inst = m_Spawner.FindMutable(id);
        if (!inst || inst->link != PrefabLinkState::Linked) {
            return false;
        }
        auto asset = std::dynamic_pointer_cast<PrefabAssetImpl>(m_Registry.Find(inst->source));
        if (!asset || !m_Deps.scene) {
            return false;
        }
        const scene::Entity* root = m_Deps.scene->FindEntityById(inst->rootEntityId);
        if (!root) {
            return false;
        }
        asset->root = CaptureRecursive(*root);
        asset->version += 1;
        asset->dirty = true;
        if (!asset->assetPath.empty()) {
            m_Serializer.SaveToFile(*asset, asset->assetPath);
            asset->dirty = false;
        }
        PrefabDiagnostics::Get().OnApply();
        PrefabEvent ev;
        ev.kind = PrefabEventKind::AppliedToAsset;
        ev.asset = asset->guid;
        ev.instance = id;
        m_Events.Publish(ev);
        if (m_Deps.config.autoUpdateInstancesOnApply) {
            UpdateAllInstances(asset->guid);
        }
        return true;
    }

    bool RevertInstance(PrefabInstanceId id) override {
        auto* inst = m_Spawner.FindMutable(id);
        if (!inst) {
            return false;
        }
        auto asset = m_Registry.Find(inst->source);
        if (!asset) {
            return false;
        }
        const PrefabGuid guid = inst->source;
        const auto parent = inst->parentInstance;
        PrefabSpawnParams params;
        params.prefab = guid;
        params.position = asset->GetRoot().position;
        params.rotation = asset->GetRoot().rotation;
        params.scale = asset->GetRoot().scale;
        params.instanceName = std::string(asset->GetName());
        m_Spawner.Despawn(id);
        const auto newId = m_Spawner.Spawn(params);
        if (auto* neu = m_Spawner.FindMutable(newId)) {
            neu->parentInstance = parent;
        }
        PrefabEvent ev;
        ev.kind = PrefabEventKind::Reverted;
        ev.asset = guid;
        ev.instance = newId;
        m_Events.Publish(ev);
        return newId.IsValid();
    }

    bool UpdateAllInstances(const PrefabGuid& guid) override {
        auto ids = m_Spawner.ListInstances(guid);
        bool ok = true;
        for (const auto id : ids) {
            auto* inst = m_Spawner.FindMutable(id);
            if (!inst || inst->HasOverrides()) {
                continue; // preserve overrides; full merge is a later incremental pass
            }
            ok = RevertInstance(id) && ok;
        }
        return ok;
    }

    bool BreakLink(PrefabInstanceId id) override {
        auto* inst = m_Spawner.FindMutable(id);
        if (!inst) {
            return false;
        }
        inst->link = PrefabLinkState::Broken;
        PrefabEvent ev;
        ev.kind = PrefabEventKind::LinkBroken;
        ev.asset = inst->source;
        ev.instance = id;
        m_Events.Publish(ev);
        return true;
    }

    bool RestoreLink(PrefabInstanceId id, const PrefabGuid& guid) override {
        auto* inst = m_Spawner.FindMutable(id);
        if (!inst || !m_Registry.Find(guid)) {
            return false;
        }
        inst->source = guid;
        inst->link = PrefabLinkState::Linked;
        PrefabEvent ev;
        ev.kind = PrefabEventKind::LinkRestored;
        ev.asset = guid;
        ev.instance = id;
        m_Events.Publish(ev);
        return true;
    }

    bool RecordPropertyOverride(
        PrefabInstanceId id,
        std::string_view path,
        serialization::BinaryDiff diff) override
    {
        auto* inst = m_Spawner.FindMutable(id);
        if (!inst || inst->link != PrefabLinkState::Linked) {
            return false;
        }
        inst->overrides.push_back(
            std::make_unique<PrefabOverrideImpl>(PrefabOverrideKind::Property, std::string(path), std::move(diff)));
        PrefabDiagnostics::Get().OnOverride();
        PrefabEvent ev;
        ev.kind = PrefabEventKind::OverrideChanged;
        ev.asset = inst->source;
        ev.instance = id;
        ev.detail = std::string(path);
        m_Events.Publish(ev);
        return true;
    }

    PrefabNodeTemplate CaptureRecursive(const scene::Entity& entity) const {
        PrefabNodeTemplate node;
        node.name = entity.Name;
        node.typeName = EntityTypeToName(entity.Type);
        node.position = entity.Position;
        node.rotation = entity.Rotation;
        node.scale = entity.Scale;
        if (!m_Deps.scene) {
            return node;
        }
        for (int idx : m_Deps.scene->FindChildIndices(entity.Id)) {
            const auto& entities = m_Deps.scene->GetEntities();
            if (idx >= 0 && static_cast<std::size_t>(idx) < entities.size()) {
                node.children.push_back(CaptureRecursive(entities[static_cast<std::size_t>(idx)]));
            }
        }
        return node;
    }

    PrefabDependencies m_Deps;
    EventRouterImpl& m_Events;
    RegistryImpl m_Registry;
    SerializerImplV2 m_Serializer;
    FactoryImpl m_Factory;
    SpawnerImpl m_Spawner;
    HierarchyImpl m_Hierarchy;
    DependencyProviderImpl m_Dependencies;
    ReferenceProviderImpl m_References;
    ValidatorImpl m_Validator;
    ImporterImpl m_Importer;
    ExporterImpl m_Exporter;
    CommandRouterImpl m_Commands;
    ThumbnailProviderImpl m_Thumbnails;
    PreviewProviderImpl m_Previews;
};

bool CommandRouterImpl::Execute(PrefabCommandId id, const PrefabCommandContext& ctx) {
    switch (id) {
    case PrefabCommandId::CreateFromSelection: {
        auto asset = m_Manager.Factory().CreateFromEntity(ctx.rootEntityId, ctx.name, ctx.path);
        if (!asset) {
            return false;
        }
        if (!m_Manager.Registry().Register(asset)) {
            return false;
        }
        if (!ctx.path.empty()) {
            return m_Manager.Serializer().SaveToFile(*asset, ctx.path);
        }
        return true;
    }
    case PrefabCommandId::SpawnAtOrigin:
        return m_Manager.Spawner().Spawn(ctx.spawn).IsValid();
    case PrefabCommandId::ApplyInstance:
        return m_Manager.ApplyInstanceToAsset(ctx.instance);
    case PrefabCommandId::RevertInstance:
        return m_Manager.RevertInstance(ctx.instance);
    case PrefabCommandId::UpdateAllInstances:
        return m_Manager.UpdateAllInstances(ctx.asset);
    case PrefabCommandId::BreakLink:
        return m_Manager.BreakLink(ctx.instance);
    case PrefabCommandId::RestoreLink:
        return m_Manager.RestoreLink(ctx.instance, ctx.asset);
    case PrefabCommandId::DuplicateAsset: {
        auto src = m_Manager.Registry().Find(ctx.asset);
        if (!src) {
            return false;
        }
        auto bytes = m_Manager.Serializer().SerializeBytes(*src);
        auto copy = m_Manager.Serializer().DeserializeBytes(bytes, ctx.path);
        if (!copy) {
            return false;
        }
        auto impl = std::dynamic_pointer_cast<PrefabAssetImpl>(copy);
        if (!impl) {
            return false;
        }
        impl->guid = GeneratePrefabGuid();
        impl->name = ctx.name.empty() ? std::string(src->GetName()) + "_Copy" : ctx.name;
        impl->assetPath = ctx.path;
        impl->dirty = true;
        if (!m_Manager.Registry().Register(impl)) {
            return false;
        }
        if (!ctx.path.empty()) {
            return m_Manager.Serializer().SaveToFile(*impl, ctx.path);
        }
        return true;
    }
    case PrefabCommandId::ConvertToActors:
        return m_Manager.BreakLink(ctx.instance);
    case PrefabCommandId::SelectSourceAsset:
        return m_Manager.Registry().Find(ctx.asset) != nullptr;
    case PrefabCommandId::ValidateAsset:
        return m_Manager.Validator().ValidateAsset(ctx.asset).empty();
    }
    return false;
}

bool CommandRouterImpl::CanExecute(PrefabCommandId id, const PrefabCommandContext& ctx) const {
    switch (id) {
    case PrefabCommandId::CreateFromSelection:
        return ctx.rootEntityId != 0;
    case PrefabCommandId::SpawnAtOrigin:
        return ctx.spawn.prefab.IsValid() && m_Manager.Registry().Find(ctx.spawn.prefab) != nullptr;
    case PrefabCommandId::ApplyInstance:
    case PrefabCommandId::RevertInstance:
    case PrefabCommandId::BreakLink:
    case PrefabCommandId::ConvertToActors:
        return ctx.instance.IsValid() && m_Manager.Spawner().FindInstance(ctx.instance) != nullptr;
    case PrefabCommandId::UpdateAllInstances:
    case PrefabCommandId::DuplicateAsset:
    case PrefabCommandId::SelectSourceAsset:
    case PrefabCommandId::ValidateAsset:
        return ctx.asset.IsValid() && m_Manager.Registry().Find(ctx.asset) != nullptr;
    case PrefabCommandId::RestoreLink:
        return ctx.instance.IsValid() && ctx.asset.IsValid();
    }
    return false;
}

class PrefabRuntimeImpl final : public IPrefabRuntime {
public:
    explicit PrefabRuntimeImpl(PrefabDependencies deps)
        : m_Deps(std::move(deps))
        , m_Events()
        , m_Manager(m_Deps, m_Events)
    {
        m_WorldBridge = std::make_unique<WorldPrefabInstancerBridge>(
            m_Manager.Spawner(), m_Manager.Registry());
        if (m_Deps.onLog) {
            m_Deps.onLog("PrefabRuntime initialized");
        }
    }

    IPrefabManager& Manager() noexcept override { return m_Manager; }
    const IPrefabManager& Manager() const noexcept override { return m_Manager; }

    world::IPrefabInstancer* MakeWorldInstancer() override { return m_WorldBridge.get(); }

    void Shutdown() override {
        if (m_Deps.onLog) {
            m_Deps.onLog("PrefabRuntime shutdown");
        }
    }

private:
    PrefabDependencies m_Deps;
    EventRouterImpl m_Events;
    ManagerImpl m_Manager;
    std::unique_ptr<WorldPrefabInstancerBridge> m_WorldBridge;
};

} // namespace detail

std::unique_ptr<IPrefabRuntime> CreatePrefabRuntime(const PrefabDependencies& deps) {
    return std::make_unique<detail::PrefabRuntimeImpl>(deps);
}

} // namespace we::runtime::prefab
