#pragma once

#include "Reflection/ITypeRegistry.h"
#include "Reflection/SerializePlan.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace we::runtime::reflection {

struct TypeRecord {
    TypeInfo info;
    std::unordered_map<NameId, std::uint32_t> propertyByNameId;
    std::unordered_map<NameId, std::uint32_t> functionByNameId;
    std::vector<TypeId> ancestors;
    std::vector<TypeId> derived;
    SerializePlan serializePlan;
    TypeId resolvedId = kInvalidTypeId;
    std::uint64_t pluginToken = 0;
    bool cachesDirty = true;
};

struct PluginScope {
    std::uint64_t token = 0;
    std::string pluginId;
};

struct RegistrySnapshot {
    std::unordered_map<TypeId, std::shared_ptr<TypeRecord>, TypeIdHash> byId;
    std::unordered_map<NameId, TypeId> byQualifiedNameId;
    std::unordered_map<NameId, TypeId> byShortNameId;
    std::vector<TypeId> allIdsSorted;
    std::uint64_t fingerprint = 0;
};

class TypeRegistry final : public ITypeRegistry {
public:
    explicit TypeRegistry(TypeRegistryDependencies deps);
    ~TypeRegistry() override = default;

    bool RegisterType(TypeInfo info) override;
    bool RegisterType(TypeInfo info, RegisterMode mode) override;
    bool UnregisterType(TypeId typeId) override;
    void Clear(bool preserveBuiltins = true) override;

    void Seal() override;
    void Unseal() override;
    [[nodiscard]] bool IsSealed() const override;

    [[nodiscard]] const TypeInfo* Find(TypeId typeId) const override;
    [[nodiscard]] const TypeInfo* FindByName(std::string_view qualifiedName) const override;
    [[nodiscard]] const TypeInfo* FindByShortName(std::string_view shortName) const override;
    [[nodiscard]] const TypeInfo* FindByNameId(NameId qualifiedNameId) const override;

    [[nodiscard]] const PropertyInfo* FindProperty(TypeId typeId, std::string_view name) const override;
    [[nodiscard]] const PropertyInfo* FindProperty(TypeId typeId, NameId nameId) const override;
    [[nodiscard]] const FunctionInfo* FindFunction(TypeId typeId, std::string_view name) const override;
    [[nodiscard]] const FunctionInfo* FindFunction(TypeId typeId, NameId nameId) const override;

    [[nodiscard]] const SerializePlan* GetSerializePlan(TypeId typeId) const override;
    [[nodiscard]] const TypeId* GetAncestorChain(TypeId typeId, std::size_t& outCount) const override;
    [[nodiscard]] const TypeId* GetDerivedTypes(TypeId typeId, std::size_t& outCount) const override;

    [[nodiscard]] bool Contains(TypeId typeId) const override;
    [[nodiscard]] std::size_t GetTypeCount() const override;
    [[nodiscard]] std::vector<TypeId> GetAllTypeIds() const override;
    [[nodiscard]] std::vector<const TypeInfo*> Query(const TypeQuery& query) const override;

    [[nodiscard]] const TypeInfo* Resolve(TypeId typeId) const override;
    [[nodiscard]] bool IsA(TypeId typeId, TypeId candidateBase) const override;
    [[nodiscard]] std::uint64_t ComputeFingerprint() const override;
    [[nodiscard]] std::uint32_t GetSchemaVersion() const override;

    void MarkTypeUsed(TypeId typeId) override;
    [[nodiscard]] std::uint64_t BeginPluginRegistration(std::string_view pluginId) override;
    void EndPluginRegistration() override;
    [[nodiscard]] bool UnregisterByPluginToken(std::uint64_t token) override;
    [[nodiscard]] std::string GetPluginId(std::uint64_t token) const override;
    [[nodiscard]] std::vector<TypeId> GetTypesByPluginToken(std::uint64_t token) const override;
    [[nodiscard]] std::uint64_t GetOwningPluginToken(TypeId typeId) const override;

    [[nodiscard]] std::shared_ptr<const RegistrySnapshot> GetSnapshot() const;
    [[nodiscard]] bool IsTypeMarkedUsed(TypeId typeId) const;
    [[nodiscard]] double GetTotalRegistrationMs() const { return m_TotalRegistrationMs; }
    [[nodiscard]] double GetLastRegistrationMs() const { return m_LastRegistrationMs; }
    [[nodiscard]] double GetLastSealMs() const { return m_LastSealMs; }
    [[nodiscard]] std::uint64_t GetRegistrationCount() const { return m_RegistrationCount.load(); }
    [[nodiscard]] std::uint64_t GetLookupCount() const { return m_LookupCount.load(); }
    [[nodiscard]] std::uint64_t GetPropertyLookupCount() const { return m_PropertyLookupCount.load(); }
    [[nodiscard]] std::uint64_t GetFunctionLookupCount() const { return m_FunctionLookupCount.load(); }

private:
    void FinalizeType(TypeInfo& info) const;
    void BuildRecordLocalCaches(TypeRecord& record) const;
    void BuildSerializePlan(TypeRecord& record, const RegistrySnapshot& snap) const;
    void RebuildGlobalCaches(RegistrySnapshot& snap) const;
    void RebuildAncestorCache(TypeRecord& record, const RegistrySnapshot& snap) const;
    void PublishFrozenSnapshot();
    [[nodiscard]] std::shared_ptr<const RegistrySnapshot> LoadFrozenSnapshot() const;
    [[nodiscard]] const TypeRecord* FindRecord(const RegistrySnapshot& snap, TypeId typeId) const;
    [[nodiscard]] bool IsAInSnapshot(const RegistrySnapshot& snap, TypeId typeId, TypeId candidateBase) const;
    [[nodiscard]] std::uint64_t ComputeFingerprintUnlocked(const RegistrySnapshot& snap) const;
    void EnsureAncestorCache(TypeRecord& record, const RegistrySnapshot& snap) const;

    /// Unsealed path: shared locking. Sealed path: lock-free via frozen snapshot.
    mutable std::shared_mutex m_Mutex;
    std::shared_ptr<RegistrySnapshot> m_Working;
    std::shared_ptr<const RegistrySnapshot> m_Frozen;
    std::atomic<bool> m_Sealed{false};
    std::unordered_set<TypeId, TypeIdHash> m_BuiltinIds;
    std::uint32_t m_NextRegistrationIndex = 0;
    std::uint64_t m_ActivePluginToken = 0;
    mutable bool m_GlobalCachesDirty = true;

    /// Plugin token → plugin id (survives across seal; not part of frozen TypeRecord).
    std::unordered_map<std::uint64_t, std::string> m_PluginIds;
    /// Types marked used — separate from frozen snapshot so Seal remains immutable.
    mutable std::mutex m_UsedMutex;
    mutable std::unordered_set<TypeId, TypeIdHash> m_UsedTypes;

    double m_TotalRegistrationMs = 0.0;
    double m_LastRegistrationMs = 0.0;
    double m_LastSealMs = 0.0;
    std::atomic<std::uint64_t> m_RegistrationCount{0};
    mutable std::atomic<std::uint64_t> m_LookupCount{0};
    mutable std::atomic<std::uint64_t> m_PropertyLookupCount{0};
    mutable std::atomic<std::uint64_t> m_FunctionLookupCount{0};
};

} // namespace we::runtime::reflection
