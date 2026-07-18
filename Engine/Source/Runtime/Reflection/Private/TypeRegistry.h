#pragma once

#include "Reflection/ITypeRegistry.h"

#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace we::runtime::reflection {

class TypeRegistry final : public ITypeRegistry {
public:
    explicit TypeRegistry(TypeRegistryDependencies deps);
    ~TypeRegistry() override = default;

    bool RegisterType(TypeInfo info) override;
    bool UnregisterType(TypeId typeId) override;
    void Clear(bool preserveBuiltins = true) override;

    [[nodiscard]] const TypeInfo* Find(TypeId typeId) const override;
    [[nodiscard]] const TypeInfo* FindByName(std::string_view qualifiedName) const override;
    [[nodiscard]] const TypeInfo* FindByShortName(std::string_view shortName) const override;

    [[nodiscard]] bool Contains(TypeId typeId) const override;
    [[nodiscard]] std::size_t GetTypeCount() const override;
    [[nodiscard]] std::vector<TypeId> GetAllTypeIds() const override;
    [[nodiscard]] std::vector<const TypeInfo*> Query(const TypeQuery& query) const override;

    [[nodiscard]] const TypeInfo* Resolve(TypeId typeId) const override;
    [[nodiscard]] bool IsA(TypeId typeId, TypeId candidateBase) const override;
    [[nodiscard]] std::uint64_t ComputeFingerprint() const override;
    [[nodiscard]] std::uint32_t GetSchemaVersion() const override;

private:
    void FinalizeType(TypeInfo& info) const;
    [[nodiscard]] bool IsAUnlocked(TypeId typeId, TypeId candidateBase) const;
    [[nodiscard]] const TypeInfo* ResolveUnlocked(TypeId typeId) const;

    mutable std::shared_mutex m_Mutex;
    std::unordered_map<TypeId, TypeInfo, TypeIdHash> m_ById;
    std::unordered_map<std::string, TypeId> m_ByQualifiedName;
    std::unordered_map<std::string, TypeId> m_ByShortName;
    std::unordered_set<TypeId, TypeIdHash> m_BuiltinIds;
    bool m_BuiltinsRegistered = false;
};

} // namespace we::runtime::reflection
