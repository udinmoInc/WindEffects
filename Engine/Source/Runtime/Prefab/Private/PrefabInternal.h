#pragma once

#include "Prefab/Prefab.h"
#include "Scene/Scene.h"
#include "World/IPrefabOrigin.h"
#include "World/WorldGuid.h"

#include <atomic>
#include <chrono>
#include <fstream>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace we::runtime::prefab {
namespace detail {

[[nodiscard]] inline PrefabGuid MakeGuid(std::uint64_t hi, std::uint64_t lo) noexcept {
    return PrefabGuid{hi, lo};
}

[[nodiscard]] PrefabGuid GeneratePrefabGuid();
[[nodiscard]] std::string EntityTypeToName(scene::EntityType type);
[[nodiscard]] scene::EntityType EntityTypeFromName(std::string_view name);
[[nodiscard]] PrefabGuid WorldGuidToPrefabGuid(const world::WorldGuid& guid);
[[nodiscard]] world::WorldGuid PrefabGuidToWorldGuid(const PrefabGuid& guid);

class PrefabOverrideImpl final : public IPrefabOverride {
public:
    PrefabOverrideImpl(PrefabOverrideKind kind, std::string path, serialization::BinaryDiff diff)
        : m_Kind(kind)
        , m_Path(std::move(path))
        , m_Diff(std::move(diff))
    {}

    [[nodiscard]] PrefabOverrideKind GetKind() const noexcept override { return m_Kind; }
    [[nodiscard]] std::string_view GetPath() const noexcept override { return m_Path; }
    [[nodiscard]] const serialization::BinaryDiff* GetPropertyDiff() const noexcept override {
        return &m_Diff;
    }

    PrefabOverrideKind m_Kind = PrefabOverrideKind::Property;
    std::string m_Path;
    serialization::BinaryDiff m_Diff;
};

class PrefabAssetImpl final : public IPrefabAsset {
public:
    PrefabGuid guid{};
    std::string name;
    std::string assetPath;
    std::uint32_t version = 1;
    PrefabNodeTemplate root{};
    std::vector<PrefabGuid> nested;
    bool dirty = false;

    [[nodiscard]] PrefabGuid GetGuid() const noexcept override { return guid; }
    [[nodiscard]] std::string_view GetName() const noexcept override { return name; }
    [[nodiscard]] std::string_view GetAssetPath() const noexcept override { return assetPath; }
    [[nodiscard]] std::uint32_t GetVersion() const noexcept override { return version; }
    [[nodiscard]] const PrefabNodeTemplate& GetRoot() const noexcept override { return root; }
    [[nodiscard]] std::span<const PrefabGuid> GetNestedPrefabs() const noexcept override {
        return nested;
    }
    [[nodiscard]] bool IsDirty() const noexcept override { return dirty; }
};

class PrefabInstanceImpl final : public IPrefabInstance {
public:
    PrefabInstanceId id{};
    PrefabGuid source{};
    PrefabLinkState link = PrefabLinkState::Linked;
    std::uint64_t rootEntityId = 0;
    PrefabInstanceId parentInstance{};
    std::vector<std::unique_ptr<IPrefabOverride>> overrides;
    std::vector<std::uint64_t> entityIds;

    [[nodiscard]] PrefabInstanceId GetId() const noexcept override { return id; }
    [[nodiscard]] PrefabGuid GetSourceGuid() const noexcept override { return source; }
    [[nodiscard]] PrefabLinkState GetLinkState() const noexcept override { return link; }
    [[nodiscard]] std::uint64_t GetRootEntityId() const noexcept override { return rootEntityId; }
    [[nodiscard]] std::span<const std::unique_ptr<IPrefabOverride>> GetOverrides() const noexcept override {
        return overrides;
    }
    [[nodiscard]] bool HasOverrides() const noexcept override { return !overrides.empty(); }
};

class PrefabVariantImpl final : public IPrefabVariant {
public:
    std::string name;
    PrefabGuid baseGuid{};
    PrefabGuid variantGuid{};

    [[nodiscard]] std::string_view GetName() const noexcept override { return name; }
    [[nodiscard]] PrefabGuid GetBaseGuid() const noexcept override { return baseGuid; }
    [[nodiscard]] PrefabGuid GetVariantGuid() const noexcept override { return variantGuid; }
};

} // namespace detail
} // namespace we::runtime::prefab
