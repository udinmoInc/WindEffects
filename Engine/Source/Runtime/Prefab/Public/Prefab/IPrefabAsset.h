#pragma once

#include "Prefab/Export.h"
#include "Prefab/PrefabTypes.h"
#include "Serialization/Delta.h"

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::prefab {

class PREFAB_API IPrefabAsset {
public:
    virtual ~IPrefabAsset() = default;

    [[nodiscard]] virtual PrefabGuid GetGuid() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetName() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetAssetPath() const noexcept = 0;
    [[nodiscard]] virtual std::uint32_t GetVersion() const noexcept = 0;
    [[nodiscard]] virtual const PrefabNodeTemplate& GetRoot() const noexcept = 0;
    [[nodiscard]] virtual std::span<const PrefabGuid> GetNestedPrefabs() const noexcept = 0;
    [[nodiscard]] virtual bool IsDirty() const noexcept = 0;
};

class PREFAB_API IPrefabOverride {
public:
    virtual ~IPrefabOverride() = default;

    [[nodiscard]] virtual PrefabOverrideKind GetKind() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetPath() const noexcept = 0;
    [[nodiscard]] virtual const serialization::BinaryDiff* GetPropertyDiff() const noexcept = 0;
};

class PREFAB_API IPrefabInstance {
public:
    virtual ~IPrefabInstance() = default;

    [[nodiscard]] virtual PrefabInstanceId GetId() const noexcept = 0;
    [[nodiscard]] virtual PrefabGuid GetSourceGuid() const noexcept = 0;
    [[nodiscard]] virtual PrefabLinkState GetLinkState() const noexcept = 0;
    [[nodiscard]] virtual std::uint64_t GetRootEntityId() const noexcept = 0;
    [[nodiscard]] virtual std::span<const std::unique_ptr<IPrefabOverride>> GetOverrides() const noexcept = 0;
    [[nodiscard]] virtual bool HasOverrides() const noexcept = 0;
};

class PREFAB_API IPrefabVariant {
public:
    virtual ~IPrefabVariant() = default;

    [[nodiscard]] virtual std::string_view GetName() const noexcept = 0;
    [[nodiscard]] virtual PrefabGuid GetBaseGuid() const noexcept = 0;
    [[nodiscard]] virtual PrefabGuid GetVariantGuid() const noexcept = 0;
};

class PREFAB_API IPrefabHierarchy {
public:
    virtual ~IPrefabHierarchy() = default;

    [[nodiscard]] virtual PrefabInstanceId GetParentInstance(PrefabInstanceId id) const noexcept = 0;
    [[nodiscard]] virtual std::vector<PrefabInstanceId> GetChildInstances(PrefabInstanceId id) const = 0;
    [[nodiscard]] virtual std::uint16_t GetNestingDepth(PrefabInstanceId id) const noexcept = 0;
};

class PREFAB_API IPrefabRegistry {
public:
    virtual ~IPrefabRegistry() = default;

    [[nodiscard]] virtual bool Register(std::shared_ptr<IPrefabAsset> asset) = 0;
    [[nodiscard]] virtual bool Unregister(const PrefabGuid& guid) = 0;
    [[nodiscard]] virtual std::shared_ptr<IPrefabAsset> Find(const PrefabGuid& guid) const = 0;
    [[nodiscard]] virtual std::shared_ptr<IPrefabAsset> FindByPath(std::string_view path) const = 0;
    [[nodiscard]] virtual std::vector<PrefabGuid> ListAll() const = 0;
};

class PREFAB_API IPrefabFactory {
public:
    virtual ~IPrefabFactory() = default;

    /// Capture Scene entity subtree into a new prefab asset document.
    [[nodiscard]] virtual std::shared_ptr<IPrefabAsset> CreateFromEntity(
        std::uint64_t rootEntityId,
        std::string_view name,
        std::string_view assetPath) = 0;

    [[nodiscard]] virtual PrefabGuid GenerateGuid() const = 0;
};

class PREFAB_API IPrefabSerializer {
public:
    virtual ~IPrefabSerializer() = default;

    [[nodiscard]] virtual bool SaveToFile(const IPrefabAsset& asset, std::string_view path) = 0;
    [[nodiscard]] virtual std::shared_ptr<IPrefabAsset> LoadFromFile(std::string_view path) = 0;
    [[nodiscard]] virtual std::vector<std::uint8_t> SerializeBytes(const IPrefabAsset& asset) = 0;
    [[nodiscard]] virtual std::shared_ptr<IPrefabAsset> DeserializeBytes(
        std::span<const std::uint8_t> bytes,
        std::string_view pathHint = {}) = 0;
};

class PREFAB_API IPrefabSpawner {
public:
    virtual ~IPrefabSpawner() = default;

    [[nodiscard]] virtual PrefabInstanceId Spawn(const PrefabSpawnParams& params) = 0;
    [[nodiscard]] virtual bool Despawn(PrefabInstanceId id) = 0;
    [[nodiscard]] virtual IPrefabInstance* FindInstance(PrefabInstanceId id) = 0;
    [[nodiscard]] virtual std::vector<PrefabInstanceId> ListInstances(const PrefabGuid& guid) const = 0;
};

class PREFAB_API IPrefabValidator {
public:
    virtual ~IPrefabValidator() = default;

    [[nodiscard]] virtual std::vector<PrefabValidationIssue> ValidateAsset(const PrefabGuid& guid) const = 0;
    [[nodiscard]] virtual std::vector<PrefabValidationIssue> ValidateInstance(PrefabInstanceId id) const = 0;
    [[nodiscard]] virtual bool HasCircularDependency(const PrefabGuid& guid) const = 0;
};

class PREFAB_API IPrefabDependencyProvider {
public:
    virtual ~IPrefabDependencyProvider() = default;
    [[nodiscard]] virtual std::vector<PrefabGuid> GetDependencies(const PrefabGuid& guid) const = 0;
};

class PREFAB_API IPrefabReferenceProvider {
public:
    virtual ~IPrefabReferenceProvider() = default;
    [[nodiscard]] virtual std::vector<PrefabInstanceId> GetReferencers(const PrefabGuid& guid) const = 0;
};

class PREFAB_API IPrefabImporter {
public:
    virtual ~IPrefabImporter() = default;
    [[nodiscard]] virtual std::shared_ptr<IPrefabAsset> Import(std::string_view sourcePath) = 0;
};

class PREFAB_API IPrefabExporter {
public:
    virtual ~IPrefabExporter() = default;
    [[nodiscard]] virtual bool Export(const PrefabGuid& guid, std::string_view destinationPath) = 0;
};

} // namespace we::runtime::prefab
