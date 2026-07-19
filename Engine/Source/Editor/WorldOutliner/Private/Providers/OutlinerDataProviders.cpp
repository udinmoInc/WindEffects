#include "OutlinerInternal.h"

#include "KindUI/Core/Icon.h"

#include <algorithm>

namespace we::editor::outliner {
namespace detail {
namespace {

std::string IconForEntityType(scene::EntityType type) {
    using scene::EntityType;
    switch (type) {
    case EntityType::DirectionalLight: return we::runtime::kindui::Icons::SunName;
    case EntityType::SkyLight: return we::runtime::kindui::Icons::LightName;
    case EntityType::SkyAtmosphere:
    case EntityType::VolumetricClouds: return we::runtime::kindui::Icons::SphereName;
    case EntityType::HeightFog: return we::runtime::kindui::Icons::LayersName;
    case EntityType::EmptyActor: return we::runtime::kindui::Icons::FolderName;
    case EntityType::Landscape: return we::runtime::kindui::Icons::MountainName;
    case EntityType::CameraIcon: return we::runtime::kindui::Icons::CameraName;
    case EntityType::AudioSource: return we::runtime::kindui::Icons::Volume2Name;
    default: return we::runtime::kindui::Icons::CubeName;
    }
}

std::string TypeNameFor(scene::EntityType type) {
    using scene::EntityType;
    switch (type) {
    case EntityType::EmptyActor: return "Folder";
    case EntityType::Character: return "Character";
    case EntityType::Blueprint: return "Blueprint";
    case EntityType::Cube: return "Cube";
    case EntityType::Sphere: return "Sphere";
    case EntityType::Cylinder: return "Cylinder";
    case EntityType::Plane: return "Plane";
    case EntityType::DirectionalLight: return "Directional Light";
    case EntityType::PointLight: return "Point Light";
    case EntityType::SpotLight: return "Spot Light";
    case EntityType::SkyLight: return "Sky Light";
    case EntityType::SkyAtmosphere: return "Sky Atmosphere";
    case EntityType::HeightFog: return "Height Fog";
    case EntityType::VolumetricClouds: return "Volumetric Clouds";
    case EntityType::Landscape: return "Landscape";
    case EntityType::GroundPlane: return "Ground Plane";
    case EntityType::CameraIcon: return "Camera";
    case EntityType::AudioSource: return "Audio";
    case EntityType::Volume: return "Volume";
    default: return "Actor";
    }
}

class SceneOutlinerNode final : public IOutlinerNode {
public:
    OutlinerNodeId id{};
    OutlinerNodeId parent{};
    OutlinerNodeKind kind = OutlinerNodeKind::Actor;
    std::string name;
    std::string typeName;
    std::string icon;
    std::string layer;
    std::vector<std::string> tags;
    OutlinerNodeFlags flags{};
    std::vector<OutlinerNodeId> children;

    [[nodiscard]] OutlinerNodeId GetId() const noexcept override { return id; }
    [[nodiscard]] OutlinerNodeId GetParentId() const noexcept override { return parent; }
    [[nodiscard]] OutlinerNodeKind GetKind() const noexcept override { return kind; }
    [[nodiscard]] std::string_view GetDisplayName() const noexcept override { return name; }
    [[nodiscard]] std::string_view GetTypeName() const noexcept override { return typeName; }
    [[nodiscard]] std::string_view GetIconName() const noexcept override { return icon; }
    [[nodiscard]] std::string_view GetLayer() const noexcept override { return layer; }
    [[nodiscard]] std::span<const std::string> GetTags() const noexcept override { return tags; }
    [[nodiscard]] const OutlinerNodeFlags& GetFlags() const noexcept override { return flags; }
    [[nodiscard]] std::span<const OutlinerNodeId> GetChildren() const noexcept override { return children; }
};

class SceneDataProvider final : public IOutlinerDataProvider {
public:
    explicit SceneDataProvider(scene::Scene* scene)
        : m_Scene(scene)
    {}

    [[nodiscard]] std::string_view GetName() const noexcept override { return "Scene"; }

    [[nodiscard]] std::vector<OutlinerNodeId> GetRootIds() const override { return m_Roots; }

    [[nodiscard]] const IOutlinerNode* FindNode(OutlinerNodeId id) const override {
        auto it = m_Nodes.find(id.value);
        return it != m_Nodes.end() ? it->second.get() : nullptr;
    }

    [[nodiscard]] std::vector<OutlinerNodeId> GetChildren(OutlinerNodeId id) const override {
        if (const auto* node = FindNode(id)) {
            return std::vector<OutlinerNodeId>(node->GetChildren().begin(), node->GetChildren().end());
        }
        return {};
    }

    void EnsureChildrenLoaded(OutlinerNodeId) override {}

    void Rebuild() override {
        m_Nodes.clear();
        m_Roots.clear();
        if (!m_Scene) {
            return;
        }

        for (const auto& entity : m_Scene->GetEntities()) {
            auto node = std::make_unique<SceneOutlinerNode>();
            node->id = OutlinerNodeId{entity.Id};
            node->parent = OutlinerNodeId{entity.ParentId};
            node->kind = (entity.Type == scene::EntityType::EmptyActor)
                ? OutlinerNodeKind::Folder
                : OutlinerNodeKind::Actor;
            node->name = entity.Name;
            node->typeName = TypeNameFor(entity.Type);
            node->icon = IconForEntityType(entity.Type);
            node->flags.expandable = true;
            node->flags.expanded = (node->kind == OutlinerNodeKind::Folder);
            node->flags.visible = true;
            m_Nodes[entity.Id] = std::move(node);
        }

        for (auto& [id, node] : m_Nodes) {
            (void)id;
            const auto parentId = node->parent.value;
            if (parentId != 0 && m_Nodes.contains(parentId)) {
                m_Nodes[parentId]->children.push_back(node->id);
            } else {
                node->parent = {};
                m_Roots.push_back(node->id);
            }
        }

        auto sortIds = [this](std::vector<OutlinerNodeId>& ids) {
            std::sort(ids.begin(), ids.end(), [this](OutlinerNodeId a, OutlinerNodeId b) {
                const auto* na = FindNode(a);
                const auto* nb = FindNode(b);
                if (!na || !nb) {
                    return a.value < b.value;
                }
                if (na->GetKind() != nb->GetKind()) {
                    return static_cast<int>(na->GetKind()) < static_cast<int>(nb->GetKind());
                }
                return na->GetDisplayName() < nb->GetDisplayName();
            });
        };
        sortIds(m_Roots);
        for (auto& [_, node] : m_Nodes) {
            sortIds(node->children);
        }
    }

private:
    scene::Scene* m_Scene = nullptr;
    std::unordered_map<std::uint64_t, std::unique_ptr<SceneOutlinerNode>> m_Nodes;
    std::vector<OutlinerNodeId> m_Roots;
};

class WorldDataProviderStub final : public IOutlinerDataProvider {
public:
    explicit WorldDataProviderStub(world::IWorldRuntime* world)
        : m_World(world)
    {}

    [[nodiscard]] std::string_view GetName() const noexcept override { return "WorldRuntime"; }
    [[nodiscard]] std::vector<OutlinerNodeId> GetRootIds() const override { return {}; }
    [[nodiscard]] const IOutlinerNode* FindNode(OutlinerNodeId) const override { return nullptr; }
    [[nodiscard]] std::vector<OutlinerNodeId> GetChildren(OutlinerNodeId) const override { return {}; }
    void EnsureChildrenLoaded(OutlinerNodeId) override {}
    void Rebuild() override {
        // Foundation: when Editor hosts CreateWorldRuntime, map IWorldManager worlds/levels/actors here.
        (void)m_World;
    }

private:
    world::IWorldRuntime* m_World = nullptr;
};

} // namespace

std::unique_ptr<IOutlinerDataProvider> CreateSceneDataProvider(scene::Scene* scene) {
    return std::make_unique<SceneDataProvider>(scene);
}

std::unique_ptr<IOutlinerDataProvider> CreateWorldDataProvider(world::IWorldRuntime* world) {
    return std::make_unique<WorldDataProviderStub>(world);
}

} // namespace detail
} // namespace we::editor::outliner
