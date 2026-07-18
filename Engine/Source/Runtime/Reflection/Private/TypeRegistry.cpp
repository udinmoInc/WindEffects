#include "TypeRegistry.h"

#include "Reflection/BuiltinTypes.h"
#include "HashUtils.h"

#include <algorithm>

namespace we::runtime::reflection {
namespace {

std::string ShortNameFromQualified(std::string_view qualified) {
    const std::size_t pos = qualified.find_last_of(':');
    if (pos == std::string_view::npos) {
        return std::string(qualified);
    }
    return std::string(qualified.substr(pos + 1));
}

} // namespace

TypeRegistry::TypeRegistry(TypeRegistryDependencies deps) {
    if (deps.registerBuiltins) {
        RegisterBuiltinTypes(*this);
        m_BuiltinsRegistered = true;
        std::unique_lock lock(m_Mutex);
        for (const auto& [id, info] : m_ById) {
            if (info.kind == TypeKind::Primitive) {
                m_BuiltinIds.insert(id);
            }
        }
    }
}

void TypeRegistry::FinalizeType(TypeInfo& info) const {
    if (info.qualifiedName.empty() && !info.name.empty()) {
        info.qualifiedName = info.name;
    }
    if (info.name.empty() && !info.qualifiedName.empty()) {
        info.name = ShortNameFromQualified(info.qualifiedName);
    }
    if (info.typeId == kInvalidTypeId && !info.qualifiedName.empty()) {
        info.typeId = MakeTypeId(info.qualifiedName);
    }
    if (info.schemaVersion == 0) {
        info.schemaVersion = 1;
    }
    if (info.alignment == 0) {
        info.alignment = 1;
    }

    for (std::uint32_t i = 0; i < info.properties.size(); ++i) {
        PropertyInfo& property = info.properties[i];
        property.ownerTypeId = info.typeId;
        property.index = i;
        if (property.alignment == 0) {
            property.alignment = 1;
        }
    }
    for (std::uint32_t i = 0; i < info.functions.size(); ++i) {
        FunctionInfo& function = info.functions[i];
        function.ownerTypeId = info.typeId;
        function.index = i;
        for (std::uint32_t p = 0; p < function.parameters.size(); ++p) {
            function.parameters[p].index = p;
        }
    }
    if (info.kind == TypeKind::Enum) {
        info.enumInfo.typeId = info.typeId;
    }
    if (info.ops.CanDefaultConstruct()) {
        info.flags = info.flags | TypeFlags::Instantiable;
    }
}

bool TypeRegistry::RegisterType(TypeInfo info) {
    FinalizeType(info);
    if (info.typeId == kInvalidTypeId || info.qualifiedName.empty()) {
        return false;
    }

    std::unique_lock lock(m_Mutex);

    // Remove previous short/qualified mappings if replacing.
    if (const auto existing = m_ById.find(info.typeId); existing != m_ById.end()) {
        m_ByQualifiedName.erase(existing->second.qualifiedName);
        m_ByShortName.erase(existing->second.name);
    }

    m_ByQualifiedName[info.qualifiedName] = info.typeId;
    m_ByShortName[info.name] = info.typeId;
    m_ById[info.typeId] = std::move(info);
    return true;
}

bool TypeRegistry::UnregisterType(TypeId typeId) {
    std::unique_lock lock(m_Mutex);
    const auto it = m_ById.find(typeId);
    if (it == m_ById.end()) {
        return false;
    }
    if (m_BuiltinIds.count(typeId) != 0) {
        return false; // builtins are permanent while preserve policy applies
    }
    m_ByQualifiedName.erase(it->second.qualifiedName);
    m_ByShortName.erase(it->second.name);
    m_ById.erase(it);
    return true;
}

void TypeRegistry::Clear(bool preserveBuiltins) {
    std::unique_lock lock(m_Mutex);
    if (!preserveBuiltins) {
        m_ById.clear();
        m_ByQualifiedName.clear();
        m_ByShortName.clear();
        m_BuiltinIds.clear();
        m_BuiltinsRegistered = false;
        return;
    }

    std::unordered_map<TypeId, TypeInfo, TypeIdHash> kept;
    std::unordered_map<std::string, TypeId> byQualified;
    std::unordered_map<std::string, TypeId> byShort;
    for (TypeId id : m_BuiltinIds) {
        const auto it = m_ById.find(id);
        if (it != m_ById.end()) {
            byQualified[it->second.qualifiedName] = id;
            byShort[it->second.name] = id;
            kept.emplace(id, std::move(it->second));
        }
    }
    m_ById = std::move(kept);
    m_ByQualifiedName = std::move(byQualified);
    m_ByShortName = std::move(byShort);
}

const TypeInfo* TypeRegistry::Find(TypeId typeId) const {
    std::shared_lock lock(m_Mutex);
    const auto it = m_ById.find(typeId);
    return it == m_ById.end() ? nullptr : &it->second;
}

const TypeInfo* TypeRegistry::FindByName(std::string_view qualifiedName) const {
    std::shared_lock lock(m_Mutex);
    const auto it = m_ByQualifiedName.find(std::string(qualifiedName));
    if (it == m_ByQualifiedName.end()) {
        return nullptr;
    }
    const auto typeIt = m_ById.find(it->second);
    return typeIt == m_ById.end() ? nullptr : &typeIt->second;
}

const TypeInfo* TypeRegistry::FindByShortName(std::string_view shortName) const {
    std::shared_lock lock(m_Mutex);
    const auto it = m_ByShortName.find(std::string(shortName));
    if (it == m_ByShortName.end()) {
        return nullptr;
    }
    const auto typeIt = m_ById.find(it->second);
    return typeIt == m_ById.end() ? nullptr : &typeIt->second;
}

bool TypeRegistry::Contains(TypeId typeId) const {
    std::shared_lock lock(m_Mutex);
    return m_ById.find(typeId) != m_ById.end();
}

std::size_t TypeRegistry::GetTypeCount() const {
    std::shared_lock lock(m_Mutex);
    return m_ById.size();
}

std::vector<TypeId> TypeRegistry::GetAllTypeIds() const {
    std::shared_lock lock(m_Mutex);
    std::vector<TypeId> ids;
    ids.reserve(m_ById.size());
    for (const auto& [id, _] : m_ById) {
        ids.push_back(id);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

std::vector<const TypeInfo*> TypeRegistry::Query(const TypeQuery& query) const {
    std::shared_lock lock(m_Mutex);
    std::vector<const TypeInfo*> results;
    results.reserve(m_ById.size());
    for (const auto& [id, info] : m_ById) {
        if (query.kind != TypeKind::Unknown && info.kind != query.kind) {
            continue;
        }
        if (query.instantiableOnly && !info.IsInstantiable()) {
            continue;
        }
        if (query.baseTypeId != kInvalidTypeId && !IsAUnlocked(id, query.baseTypeId)) {
            continue;
        }
        if (query.interfaceTypeId != kInvalidTypeId) {
            bool implements = false;
            for (TypeId iface : info.interfaces) {
                if (iface == query.interfaceTypeId || IsAUnlocked(iface, query.interfaceTypeId)) {
                    implements = true;
                    break;
                }
            }
            if (!implements && !IsAUnlocked(id, query.interfaceTypeId)) {
                continue;
            }
        }
        results.push_back(&info);
    }
    std::sort(results.begin(), results.end(), [](const TypeInfo* a, const TypeInfo* b) {
        return a->typeId < b->typeId;
    });
    return results;
}

const TypeInfo* TypeRegistry::ResolveUnlocked(TypeId typeId) const {
    const TypeInfo* current = nullptr;
    {
        const auto it = m_ById.find(typeId);
        if (it == m_ById.end()) {
            return nullptr;
        }
        current = &it->second;
    }
    // Follow alias chain with cycle guard.
    for (int depth = 0; depth < 32 && current && current->kind == TypeKind::Alias; ++depth) {
        if (current->aliasOf == kInvalidTypeId) {
            break;
        }
        const auto it = m_ById.find(current->aliasOf);
        if (it == m_ById.end()) {
            return current;
        }
        current = &it->second;
    }
    return current;
}

const TypeInfo* TypeRegistry::Resolve(TypeId typeId) const {
    std::shared_lock lock(m_Mutex);
    return ResolveUnlocked(typeId);
}

bool TypeRegistry::IsAUnlocked(TypeId typeId, TypeId candidateBase) const {
    if (typeId == kInvalidTypeId || candidateBase == kInvalidTypeId) {
        return false;
    }
    if (typeId == candidateBase) {
        return true;
    }

    // BFS over bases + interfaces.
    std::vector<TypeId> stack;
    stack.push_back(typeId);
    std::unordered_set<TypeId, TypeIdHash> visited;
    while (!stack.empty()) {
        const TypeId currentId = stack.back();
        stack.pop_back();
        if (!visited.insert(currentId).second) {
            continue;
        }
        if (currentId == candidateBase) {
            return true;
        }
        const auto it = m_ById.find(currentId);
        if (it == m_ById.end()) {
            continue;
        }
        const TypeInfo& info = it->second;
        if (info.kind == TypeKind::Alias && info.aliasOf != kInvalidTypeId) {
            stack.push_back(info.aliasOf);
        }
        for (TypeId base : info.baseTypes) {
            stack.push_back(base);
        }
        for (TypeId iface : info.interfaces) {
            stack.push_back(iface);
        }
    }
    return false;
}

bool TypeRegistry::IsA(TypeId typeId, TypeId candidateBase) const {
    std::shared_lock lock(m_Mutex);
    return IsAUnlocked(typeId, candidateBase);
}

std::uint64_t TypeRegistry::ComputeFingerprint() const {
    std::shared_lock lock(m_Mutex);
    std::vector<TypeId> ids;
    ids.reserve(m_ById.size());
    for (const auto& [id, _] : m_ById) {
        ids.push_back(id);
    }
    std::sort(ids.begin(), ids.end());

    std::uint64_t hash = detail::Fnv1a64("we.reflection.registry");
    hash ^= kReflectionSchemaVersion;
    hash *= 1099511628211ull;
    for (TypeId id : ids) {
        const TypeInfo& info = m_ById.at(id);
        hash ^= id;
        hash *= 1099511628211ull;
        hash ^= info.schemaVersion;
        hash *= 1099511628211ull;
        hash ^= static_cast<std::uint64_t>(info.kind);
        hash *= 1099511628211ull;
        hash ^= info.properties.size();
        hash *= 1099511628211ull;
    }
    return hash == 0 ? 1ull : hash;
}

std::uint32_t TypeRegistry::GetSchemaVersion() const {
    return kReflectionSchemaVersion;
}

std::unique_ptr<ITypeRegistry> CreateTypeRegistry(TypeRegistryDependencies deps) {
    return std::make_unique<TypeRegistry>(std::move(deps));
}

ITypeRegistry& GetTypeRegistry() {
    static std::unique_ptr<ITypeRegistry> instance = CreateTypeRegistry({});
    return *instance;
}

} // namespace we::runtime::reflection
