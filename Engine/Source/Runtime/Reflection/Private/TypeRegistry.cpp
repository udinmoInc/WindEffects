#include "TypeRegistry.h"

#include "HashUtils.h"
#include "Reflection/BuiltinTypes.h"
#include "Reflection/NameId.h"
#include "Reflection/PropertyPath.h"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <utility>

namespace we::runtime::reflection {
namespace {

std::string ShortNameFromQualified(std::string_view qualified) {
    const std::size_t pos = qualified.find_last_of(':');
    if (pos == std::string_view::npos) {
        return std::string(qualified);
    }
    return std::string(qualified.substr(pos + 1));
}

std::mutex& StaticInitMutex() {
    static std::mutex mutex;
    return mutex;
}

std::vector<StaticTypeInitFn>& StaticInitFns() {
    static std::vector<StaticTypeInitFn> fns;
    return fns;
}

std::atomic<std::uint64_t>& PluginTokenCounter() {
    static std::atomic<std::uint64_t> counter{1};
    return counter;
}

} // namespace

void RegisterStaticTypeInitializer(StaticTypeInitFn fn) {
    if (!fn) {
        return;
    }
    std::lock_guard lock(StaticInitMutex());
    StaticInitFns().push_back(fn);
}

void ApplyStaticTypeInitializers(ITypeRegistry& registry) {
    std::vector<StaticTypeInitFn> copy;
    {
        std::lock_guard lock(StaticInitMutex());
        copy = StaticInitFns();
    }
    for (StaticTypeInitFn fn : copy) {
        fn(registry);
    }
}

TypeRegistry::TypeRegistry(TypeRegistryDependencies deps) {
    m_Working = std::make_shared<RegistrySnapshot>();
    PublishFrozenSnapshot();

    if (deps.registerBuiltins) {
        RegisterBuiltinTypes(*this);
        std::unique_lock lock(m_Mutex);
        for (const auto& [id, record] : m_Working->byId) {
            if (record && record->info.kind == TypeKind::Primitive) {
                m_BuiltinIds.insert(id);
            }
        }
    }
    if (deps.applyStaticInitializers) {
        ApplyStaticTypeInitializers(*this);
    }
    if (deps.sealAfterBuiltins) {
        Seal();
    }
}

void TypeRegistry::PublishFrozenSnapshot() {
    // Share the working graph as a const view. After Seal(), working must not mutate records
    // in place without Unseal. Fingerprint is computed at seal/publish time.
    if (m_Working) {
        m_Working->fingerprint = ComputeFingerprintUnlocked(*m_Working);
    }
    std::atomic_store(&m_Frozen, std::shared_ptr<const RegistrySnapshot>(m_Working));
}

std::shared_ptr<const RegistrySnapshot> TypeRegistry::LoadFrozenSnapshot() const {
    return std::atomic_load(&m_Frozen);
}

std::shared_ptr<const RegistrySnapshot> TypeRegistry::GetSnapshot() const {
    if (m_Sealed.load(std::memory_order_acquire)) {
        return LoadFrozenSnapshot();
    }
    std::shared_lock lock(m_Mutex);
    return m_Working;
}

const TypeRecord* TypeRegistry::FindRecord(const RegistrySnapshot& snap, TypeId typeId) const {
    const auto it = snap.byId.find(typeId);
    return it == snap.byId.end() ? nullptr : it->second.get();
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
    if (info.versions.schemaVersion == 1 && info.schemaVersion != 1) {
        info.versions.schemaVersion = info.schemaVersion;
    } else {
        info.schemaVersion = info.versions.schemaVersion == 0 ? 1 : info.versions.schemaVersion;
        info.versions.schemaVersion = info.schemaVersion;
    }
    if (info.versions.typeVersion == 0) {
        info.versions.typeVersion = 1;
    }
    if (info.versions.migrationVersion == 0) {
        info.versions.migrationVersion = 1;
    }
    if (info.versions.binaryCompatibilityVersion == 0) {
        info.versions.binaryCompatibilityVersion = 1;
    }
    if (info.alignment == 0) {
        info.alignment = 1;
    }

    info.qualifiedNameId = InternName(info.qualifiedName);
    info.nameId = InternName(info.name);

    for (std::uint32_t i = 0; i < info.properties.size(); ++i) {
        PropertyInfo& property = info.properties[i];
        property.ownerTypeId = info.typeId;
        property.index = i;
        if (property.alignment == 0) {
            property.alignment = 1;
        }
        if (property.nameId == kInvalidNameId && !property.name.empty()) {
            property.nameId = InternName(property.name);
        }
        for (AttributeInfo& attr : property.attributes.entries) {
            if (attr.nameId == kInvalidNameId && !attr.name.empty()) {
                attr.nameId = InternName(attr.name);
            }
            if (attr.kind == AttributeValueKind::String && attr.stringValueId == kInvalidNameId
                && !attr.stringValue.empty()) {
                attr.stringValueId = InternName(attr.stringValue);
            }
        }
    }
    for (std::uint32_t i = 0; i < info.functions.size(); ++i) {
        FunctionInfo& function = info.functions[i];
        function.ownerTypeId = info.typeId;
        function.index = i;
        if (function.nameId == kInvalidNameId && !function.name.empty()) {
            function.nameId = InternName(function.name);
        }
        for (ParameterInfo& param : function.parameters) {
            if (param.nameId == kInvalidNameId && !param.name.empty()) {
                param.nameId = InternName(param.name);
            }
        }
        for (std::uint32_t p = 0; p < function.parameters.size(); ++p) {
            function.parameters[p].index = p;
        }
    }
    for (AttributeInfo& attr : info.attributes.entries) {
        if (attr.nameId == kInvalidNameId && !attr.name.empty()) {
            attr.nameId = InternName(attr.name);
        }
        if (attr.kind == AttributeValueKind::String && attr.stringValueId == kInvalidNameId
            && !attr.stringValue.empty()) {
            attr.stringValueId = InternName(attr.stringValue);
        }
    }
    if (info.kind == TypeKind::Enum) {
        info.enumInfo.typeId = info.typeId;
    }
    if (info.ops.CanDefaultConstruct()) {
        info.flags = info.flags | TypeFlags::Instantiable;
    }
}

void TypeRegistry::BuildRecordLocalCaches(TypeRecord& record) const {
    record.propertyByNameId.clear();
    record.functionByNameId.clear();
    record.propertyByNameId.reserve(record.info.properties.size());
    record.functionByNameId.reserve(record.info.functions.size());
    for (const PropertyInfo& property : record.info.properties) {
        if (property.nameId != kInvalidNameId) {
            record.propertyByNameId.emplace(property.nameId, property.index);
        }
    }
    for (const FunctionInfo& function : record.info.functions) {
        if (function.nameId != kInvalidNameId) {
            record.functionByNameId.emplace(function.nameId, function.index);
        }
    }
    record.resolvedId = record.info.typeId;
    record.cachesDirty = true;
}

void TypeRegistry::RebuildAncestorCache(TypeRecord& record, const RegistrySnapshot& snap) const {
    record.ancestors.clear();
    record.ancestors.push_back(record.info.typeId);

    std::vector<TypeId> stack;
    std::unordered_set<TypeId, TypeIdHash> visited;
    visited.insert(record.info.typeId);

    auto push = [&](TypeId id) {
        if (id != kInvalidTypeId && visited.insert(id).second) {
            stack.push_back(id);
            record.ancestors.push_back(id);
        }
    };

    for (TypeId base : record.info.baseTypes) {
        push(base);
    }
    for (TypeId iface : record.info.interfaces) {
        push(iface);
    }
    if (record.info.kind == TypeKind::Alias && record.info.aliasOf != kInvalidTypeId) {
        push(record.info.aliasOf);
    }

    while (!stack.empty()) {
        const TypeId current = stack.back();
        stack.pop_back();
        const TypeRecord* other = FindRecord(snap, current);
        if (!other) {
            continue;
        }
        for (TypeId base : other->info.baseTypes) {
            push(base);
        }
        for (TypeId iface : other->info.interfaces) {
            push(iface);
        }
        if (other->info.kind == TypeKind::Alias && other->info.aliasOf != kInvalidTypeId) {
            push(other->info.aliasOf);
        }
    }
}

void TypeRegistry::BuildSerializePlan(TypeRecord& record, const RegistrySnapshot& snap) const {
    SerializePlan plan;
    plan.typeId = record.info.typeId;
    plan.schemaVersion = record.info.versions.schemaVersion;
    plan.binaryCompatibilityVersion = record.info.versions.binaryCompatibilityVersion;
    plan.includeBases = true;

    std::vector<TypeId> order;
    order.reserve(record.ancestors.size());
    for (std::size_t i = record.ancestors.size(); i > 0; --i) {
        order.push_back(record.ancestors[i - 1]);
    }

    std::vector<SerializeStep> steps;
    std::unordered_map<NameId, std::size_t> stepIndexByName;

    for (TypeId ownerId : order) {
        const TypeRecord* owner = FindRecord(snap, ownerId);
        if (!owner) {
            continue;
        }
        for (const PropertyInfo& property : owner->info.properties) {
            if (!property.IsSerialized()) {
                continue;
            }
            SerializeStep step;
            step.property = &property;
            step.propertyTypeId = property.typeId;
            step.primitive = property.primitive;
            step.offset = property.offset;
            step.size = property.size;
            step.schemaTag = property.schemaTag;
            step.isString = property.primitive == PrimitiveKind::String;

            const TypeRecord* propType = FindRecord(snap, property.typeId);
            if (propType) {
                if (step.primitive == PrimitiveKind::None && propType->info.IsPrimitive()) {
                    step.primitive = propType->info.primitive;
                }
                step.isEnum = propType->info.IsEnum();
                step.isNestedObject = propType->info.IsClassOrStruct();
                step.isString = step.isString || propType->info.primitive == PrimitiveKind::String;
            }

            if (property.nameId != kInvalidNameId) {
                const auto it = stepIndexByName.find(property.nameId);
                if (it != stepIndexByName.end()) {
                    steps[it->second] = step;
                    continue;
                }
                stepIndexByName.emplace(property.nameId, steps.size());
            }
            steps.push_back(step);
        }
    }

    plan.steps = std::move(steps);
    record.serializePlan = std::move(plan);
    record.cachesDirty = false;
}

void TypeRegistry::RebuildGlobalCaches(RegistrySnapshot& snap) const {
    for (auto& [_, record] : snap.byId) {
        if (record) {
            record->derived.clear();
        }
    }
    for (auto& [id, record] : snap.byId) {
        if (!record) {
            continue;
        }
        for (TypeId base : record->info.baseTypes) {
            if (auto it = snap.byId.find(base); it != snap.byId.end() && it->second) {
                it->second->derived.push_back(id);
            }
        }
        for (TypeId iface : record->info.interfaces) {
            if (auto it = snap.byId.find(iface); it != snap.byId.end() && it->second) {
                it->second->derived.push_back(id);
            }
        }
    }
    for (auto& [_, record] : snap.byId) {
        if (!record) {
            continue;
        }
        auto& d = record->derived;
        std::sort(d.begin(), d.end());
        d.erase(std::unique(d.begin(), d.end()), d.end());
        RebuildAncestorCache(*record, snap);

        TypeId resolved = record->info.typeId;
        if (record->info.kind == TypeKind::Alias) {
            TypeId current = record->info.aliasOf;
            for (int depth = 0; depth < 32; ++depth) {
                const TypeRecord* next = FindRecord(snap, current);
                if (!next) {
                    break;
                }
                resolved = next->info.typeId;
                if (next->info.kind != TypeKind::Alias) {
                    break;
                }
                current = next->info.aliasOf;
            }
        }
        record->resolvedId = resolved;
        BuildSerializePlan(*record, snap);
    }
}

void TypeRegistry::EnsureAncestorCache(TypeRecord& record, const RegistrySnapshot& snap) const {
    if (!record.cachesDirty && !record.ancestors.empty()) {
        return;
    }
    RebuildAncestorCache(record, snap);
    BuildSerializePlan(record, snap);
}

bool TypeRegistry::RegisterType(TypeInfo info) {
    return RegisterType(std::move(info), RegisterMode::Replace);
}

bool TypeRegistry::RegisterType(TypeInfo info, RegisterMode mode) {
    if (m_Sealed.load(std::memory_order_acquire)) {
        return false;
    }

    const auto start = std::chrono::steady_clock::now();
    FinalizeType(info);
    if (info.typeId == kInvalidTypeId || info.qualifiedName.empty()) {
        return false;
    }

    {
        std::unique_lock lock(m_Mutex);
        if (m_Sealed.load(std::memory_order_relaxed)) {
            return false;
        }

        const auto existing = m_Working->byId.find(info.typeId);
        if (existing != m_Working->byId.end()) {
            if (mode == RegisterMode::FailIfExists) {
                return false;
            }
            if (mode == RegisterMode::KeepExisting) {
                return true;
            }
            m_Working->byQualifiedNameId.erase(existing->second->info.qualifiedNameId);
            m_Working->byShortNameId.erase(existing->second->info.nameId);
        } else {
            info.registrationIndex = m_NextRegistrationIndex++;
            m_Working->allIdsSorted.push_back(info.typeId);
            m_GlobalCachesDirty = true;
        }

        auto record = std::make_shared<TypeRecord>();
        record->info = std::move(info);
        record->pluginToken = m_ActivePluginToken;
        BuildRecordLocalCaches(*record);

        m_Working->byQualifiedNameId[record->info.qualifiedNameId] = record->info.typeId;
        m_Working->byShortNameId[record->info.nameId] = record->info.typeId;
        m_Working->byId[record->info.typeId] = std::move(record);
        m_GlobalCachesDirty = true;
    }

    const auto end = std::chrono::steady_clock::now();
    m_LastRegistrationMs = std::chrono::duration<double, std::milli>(end - start).count();
    m_TotalRegistrationMs += m_LastRegistrationMs;
    m_RegistrationCount.fetch_add(1, std::memory_order_relaxed);
    return true;
}

bool TypeRegistry::UnregisterType(TypeId typeId) {
    if (m_Sealed.load(std::memory_order_acquire)) {
        return false;
    }
    std::unique_lock lock(m_Mutex);
    if (m_Sealed.load(std::memory_order_relaxed) || m_BuiltinIds.count(typeId) != 0) {
        return false;
    }
    const auto it = m_Working->byId.find(typeId);
    if (it == m_Working->byId.end() || !it->second) {
        return false;
    }
    m_Working->byQualifiedNameId.erase(it->second->info.qualifiedNameId);
    m_Working->byShortNameId.erase(it->second->info.nameId);
    m_Working->byId.erase(it);
    m_Working->allIdsSorted.erase(
        std::remove(m_Working->allIdsSorted.begin(), m_Working->allIdsSorted.end(), typeId),
        m_Working->allIdsSorted.end());
    {
        std::lock_guard usedLock(m_UsedMutex);
        m_UsedTypes.erase(typeId);
    }
    m_GlobalCachesDirty = true;
    ClearPropertyPathCache();
    return true;
}

void TypeRegistry::Clear(bool preserveBuiltins) {
    if (m_Sealed.load(std::memory_order_acquire)) {
        return;
    }
    std::unique_lock lock(m_Mutex);
    auto next = std::make_shared<RegistrySnapshot>();
    if (preserveBuiltins) {
        for (TypeId id : m_BuiltinIds) {
            const auto it = m_Working->byId.find(id);
            if (it == m_Working->byId.end() || !it->second) {
                continue;
            }
            next->byId.emplace(id, it->second);
            next->byQualifiedNameId[it->second->info.qualifiedNameId] = id;
            next->byShortNameId[it->second->info.nameId] = id;
            next->allIdsSorted.push_back(id);
        }
        std::sort(next->allIdsSorted.begin(), next->allIdsSorted.end());
    } else {
        m_BuiltinIds.clear();
    }
    m_Working = std::move(next);
    m_GlobalCachesDirty = true;
    PublishFrozenSnapshot();
}

void TypeRegistry::Seal() {
    const auto start = std::chrono::steady_clock::now();
    std::unique_lock lock(m_Mutex);
    std::sort(m_Working->allIdsSorted.begin(), m_Working->allIdsSorted.end());
    m_Working->allIdsSorted.erase(
        std::unique(m_Working->allIdsSorted.begin(), m_Working->allIdsSorted.end()),
        m_Working->allIdsSorted.end());
    if (m_GlobalCachesDirty) {
        RebuildGlobalCaches(*m_Working);
        m_GlobalCachesDirty = false;
    }
    PublishFrozenSnapshot();
    m_Sealed.store(true, std::memory_order_release);
    const auto end = std::chrono::steady_clock::now();
    m_LastSealMs = std::chrono::duration<double, std::milli>(end - start).count();
}

void TypeRegistry::Unseal() {
    std::unique_lock lock(m_Mutex);
    m_Sealed.store(false, std::memory_order_release);
    ClearPropertyPathCache();
}

bool TypeRegistry::IsSealed() const {
    return m_Sealed.load(std::memory_order_acquire);
}

const TypeInfo* TypeRegistry::Find(TypeId typeId) const {
    m_LookupCount.fetch_add(1, std::memory_order_relaxed);
    if (m_Sealed.load(std::memory_order_acquire)) {
        auto snap = LoadFrozenSnapshot();
        if (!snap) {
            return nullptr;
        }
        const TypeRecord* record = FindRecord(*snap, typeId);
        return record ? &record->info : nullptr;
    }
    std::shared_lock lock(m_Mutex);
    const TypeRecord* record = FindRecord(*m_Working, typeId);
    return record ? &record->info : nullptr;
}

const TypeInfo* TypeRegistry::FindByNameId(NameId qualifiedNameId) const {
    m_LookupCount.fetch_add(1, std::memory_order_relaxed);
    if (qualifiedNameId == kInvalidNameId) {
        return nullptr;
    }
    auto lookup = [&](const RegistrySnapshot& snap) -> const TypeInfo* {
        const auto it = snap.byQualifiedNameId.find(qualifiedNameId);
        if (it == snap.byQualifiedNameId.end()) {
            return nullptr;
        }
        const TypeRecord* record = FindRecord(snap, it->second);
        return record ? &record->info : nullptr;
    };
    if (m_Sealed.load(std::memory_order_acquire)) {
        auto snap = LoadFrozenSnapshot();
        return snap ? lookup(*snap) : nullptr;
    }
    std::shared_lock lock(m_Mutex);
    return lookup(*m_Working);
}

const TypeInfo* TypeRegistry::FindByName(std::string_view qualifiedName) const {
    return FindByNameId(InternName(qualifiedName));
}

const TypeInfo* TypeRegistry::FindByShortName(std::string_view shortName) const {
    m_LookupCount.fetch_add(1, std::memory_order_relaxed);
    const NameId id = InternName(shortName);
    auto lookup = [&](const RegistrySnapshot& snap) -> const TypeInfo* {
        const auto it = snap.byShortNameId.find(id);
        if (it == snap.byShortNameId.end()) {
            return nullptr;
        }
        const TypeRecord* record = FindRecord(snap, it->second);
        return record ? &record->info : nullptr;
    };
    if (m_Sealed.load(std::memory_order_acquire)) {
        auto snap = LoadFrozenSnapshot();
        return snap ? lookup(*snap) : nullptr;
    }
    std::shared_lock lock(m_Mutex);
    return lookup(*m_Working);
}

const PropertyInfo* TypeRegistry::FindProperty(TypeId typeId, NameId nameId) const {
    m_PropertyLookupCount.fetch_add(1, std::memory_order_relaxed);
    if (nameId == kInvalidNameId) {
        return nullptr;
    }

    auto lookup = [&](const RegistrySnapshot& snap) -> const PropertyInfo* {
        const TypeRecord* record = FindRecord(snap, typeId);
        if (!record) {
            return nullptr;
        }
        const auto it = record->propertyByNameId.find(nameId);
        if (it != record->propertyByNameId.end() && it->second < record->info.properties.size()) {
            return &record->info.properties[it->second];
        }
        if (record->cachesDirty || record->ancestors.empty()) {
            // Lazy ancestor build for unsealed registries.
            EnsureAncestorCache(const_cast<TypeRecord&>(*record), snap);
        }
        for (TypeId ancestor : record->ancestors) {
            if (ancestor == typeId) {
                continue;
            }
            const TypeRecord* base = FindRecord(snap, ancestor);
            if (!base) {
                continue;
            }
            const auto bit = base->propertyByNameId.find(nameId);
            if (bit != base->propertyByNameId.end() && bit->second < base->info.properties.size()) {
                return &base->info.properties[bit->second];
            }
        }
        return nullptr;
    };

    if (m_Sealed.load(std::memory_order_acquire)) {
        auto snap = LoadFrozenSnapshot();
        return snap ? lookup(*snap) : nullptr;
    }
    std::shared_lock lock(m_Mutex);
    return lookup(*m_Working);
}

const PropertyInfo* TypeRegistry::FindProperty(TypeId typeId, std::string_view name) const {
    return FindProperty(typeId, InternName(name));
}

const FunctionInfo* TypeRegistry::FindFunction(TypeId typeId, NameId nameId) const {
    m_FunctionLookupCount.fetch_add(1, std::memory_order_relaxed);
    if (nameId == kInvalidNameId) {
        return nullptr;
    }
    auto lookup = [&](const RegistrySnapshot& snap) -> const FunctionInfo* {
        const TypeRecord* record = FindRecord(snap, typeId);
        if (!record) {
            return nullptr;
        }
        const auto it = record->functionByNameId.find(nameId);
        if (it != record->functionByNameId.end() && it->second < record->info.functions.size()) {
            return &record->info.functions[it->second];
        }
        if (record->cachesDirty || record->ancestors.empty()) {
            EnsureAncestorCache(const_cast<TypeRecord&>(*record), snap);
        }
        for (TypeId ancestor : record->ancestors) {
            if (ancestor == typeId) {
                continue;
            }
            const TypeRecord* base = FindRecord(snap, ancestor);
            if (!base) {
                continue;
            }
            const auto bit = base->functionByNameId.find(nameId);
            if (bit != base->functionByNameId.end() && bit->second < base->info.functions.size()) {
                return &base->info.functions[bit->second];
            }
        }
        return nullptr;
    };
    if (m_Sealed.load(std::memory_order_acquire)) {
        auto snap = LoadFrozenSnapshot();
        return snap ? lookup(*snap) : nullptr;
    }
    std::shared_lock lock(m_Mutex);
    return lookup(*m_Working);
}

const FunctionInfo* TypeRegistry::FindFunction(TypeId typeId, std::string_view name) const {
    return FindFunction(typeId, InternName(name));
}

const SerializePlan* TypeRegistry::GetSerializePlan(TypeId typeId) const {
    auto lookup = [&](const RegistrySnapshot& snap) -> const SerializePlan* {
        const TypeRecord* record = FindRecord(snap, typeId);
        if (!record) {
            return nullptr;
        }
        if (record->cachesDirty) {
            EnsureAncestorCache(const_cast<TypeRecord&>(*record), snap);
        }
        return &record->serializePlan;
    };
    if (m_Sealed.load(std::memory_order_acquire)) {
        auto snap = LoadFrozenSnapshot();
        return snap ? lookup(*snap) : nullptr;
    }
    std::shared_lock lock(m_Mutex);
    return lookup(*m_Working);
}

const TypeId* TypeRegistry::GetAncestorChain(TypeId typeId, std::size_t& outCount) const {
    outCount = 0;
    auto lookup = [&](const RegistrySnapshot& snap) -> const TypeId* {
        const TypeRecord* record = FindRecord(snap, typeId);
        if (!record) {
            return nullptr;
        }
        if (record->cachesDirty || record->ancestors.empty()) {
            EnsureAncestorCache(const_cast<TypeRecord&>(*record), snap);
        }
        if (record->ancestors.empty()) {
            return nullptr;
        }
        outCount = record->ancestors.size();
        return record->ancestors.data();
    };
    if (m_Sealed.load(std::memory_order_acquire)) {
        auto snap = LoadFrozenSnapshot();
        return snap ? lookup(*snap) : nullptr;
    }
    std::shared_lock lock(m_Mutex);
    return lookup(*m_Working);
}

const TypeId* TypeRegistry::GetDerivedTypes(TypeId typeId, std::size_t& outCount) const {
    outCount = 0;
    auto lookup = [&](const RegistrySnapshot& snap) -> const TypeId* {
        const TypeRecord* record = FindRecord(snap, typeId);
        if (!record || record->derived.empty()) {
            return nullptr;
        }
        outCount = record->derived.size();
        return record->derived.data();
    };
    if (m_Sealed.load(std::memory_order_acquire)) {
        auto snap = LoadFrozenSnapshot();
        return snap ? lookup(*snap) : nullptr;
    }
    std::shared_lock lock(m_Mutex);
    if (m_GlobalCachesDirty) {
        // Derived cache requires global rebuild; do it lazily under upgrade... use unique briefly.
        lock.unlock();
        std::unique_lock writeLock(m_Mutex);
        if (m_GlobalCachesDirty) {
            RebuildGlobalCaches(*m_Working);
            m_GlobalCachesDirty = false;
        }
        return lookup(*m_Working);
    }
    return lookup(*m_Working);
}

bool TypeRegistry::Contains(TypeId typeId) const {
    return Find(typeId) != nullptr;
}

std::size_t TypeRegistry::GetTypeCount() const {
    if (m_Sealed.load(std::memory_order_acquire)) {
        auto snap = LoadFrozenSnapshot();
        return snap ? snap->byId.size() : 0;
    }
    std::shared_lock lock(m_Mutex);
    return m_Working->byId.size();
}

std::vector<TypeId> TypeRegistry::GetAllTypeIds() const {
    if (m_Sealed.load(std::memory_order_acquire)) {
        auto snap = LoadFrozenSnapshot();
        return snap ? snap->allIdsSorted : std::vector<TypeId>{};
    }
    std::shared_lock lock(m_Mutex);
    return m_Working->allIdsSorted;
}

std::vector<const TypeInfo*> TypeRegistry::Query(const TypeQuery& query) const {
    std::vector<const TypeInfo*> results;
    auto run = [&](const RegistrySnapshot& snap) {
        results.reserve(snap.byId.size());
        for (TypeId id : snap.allIdsSorted) {
            const TypeRecord* record = FindRecord(snap, id);
            if (!record) {
                continue;
            }
            const TypeInfo& info = record->info;
            if (query.kind != TypeKind::Unknown && info.kind != query.kind) {
                continue;
            }
            if (query.instantiableOnly && !info.IsInstantiable()) {
                continue;
            }
            if (query.baseTypeId != kInvalidTypeId && !IsAInSnapshot(snap, id, query.baseTypeId)) {
                continue;
            }
            if (query.interfaceTypeId != kInvalidTypeId
                && !IsAInSnapshot(snap, id, query.interfaceTypeId)) {
                continue;
            }
            results.push_back(&info);
        }
    };
    if (m_Sealed.load(std::memory_order_acquire)) {
        auto snap = LoadFrozenSnapshot();
        if (snap) {
            run(*snap);
        }
        return results;
    }
    std::shared_lock lock(m_Mutex);
    run(*m_Working);
    return results;
}

const TypeInfo* TypeRegistry::Resolve(TypeId typeId) const {
    auto lookup = [&](const RegistrySnapshot& snap) -> const TypeInfo* {
        const TypeRecord* record = FindRecord(snap, typeId);
        if (!record) {
            return nullptr;
        }
        if (record->resolvedId == typeId || record->resolvedId == kInvalidTypeId) {
            return &record->info;
        }
        const TypeRecord* resolved = FindRecord(snap, record->resolvedId);
        return resolved ? &resolved->info : &record->info;
    };
    if (m_Sealed.load(std::memory_order_acquire)) {
        auto snap = LoadFrozenSnapshot();
        return snap ? lookup(*snap) : nullptr;
    }
    std::shared_lock lock(m_Mutex);
    return lookup(*m_Working);
}

bool TypeRegistry::IsAInSnapshot(const RegistrySnapshot& snap, TypeId typeId, TypeId candidateBase) const {
    if (typeId == kInvalidTypeId || candidateBase == kInvalidTypeId) {
        return false;
    }
    if (typeId == candidateBase) {
        return true;
    }
    const TypeRecord* record = FindRecord(snap, typeId);
    if (!record) {
        return false;
    }
    if (record->cachesDirty || record->ancestors.empty()) {
        EnsureAncestorCache(const_cast<TypeRecord&>(*record), snap);
    }
    return std::find(record->ancestors.begin(), record->ancestors.end(), candidateBase)
        != record->ancestors.end();
}

bool TypeRegistry::IsA(TypeId typeId, TypeId candidateBase) const {
    if (m_Sealed.load(std::memory_order_acquire)) {
        auto snap = LoadFrozenSnapshot();
        return snap ? IsAInSnapshot(*snap, typeId, candidateBase) : false;
    }
    std::shared_lock lock(m_Mutex);
    return IsAInSnapshot(*m_Working, typeId, candidateBase);
}

std::uint64_t TypeRegistry::ComputeFingerprintUnlocked(const RegistrySnapshot& snap) const {
    std::uint64_t hash = detail::Fnv1a64("we.reflection.registry.v2");
    hash ^= kReflectionSchemaVersion;
    hash *= 1099511628211ull;
    for (TypeId id : snap.allIdsSorted) {
        const TypeRecord* record = FindRecord(snap, id);
        if (!record) {
            continue;
        }
        const TypeInfo& info = record->info;
        hash ^= id;
        hash *= 1099511628211ull;
        hash ^= info.versions.schemaVersion;
        hash *= 1099511628211ull;
        hash ^= info.versions.binaryCompatibilityVersion;
        hash *= 1099511628211ull;
        hash ^= static_cast<std::uint64_t>(info.kind);
        hash *= 1099511628211ull;
        hash ^= info.properties.size();
        hash *= 1099511628211ull;
    }
    return hash == 0 ? 1ull : hash;
}

std::uint64_t TypeRegistry::ComputeFingerprint() const {
    if (m_Sealed.load(std::memory_order_acquire)) {
        auto snap = LoadFrozenSnapshot();
        return snap ? snap->fingerprint : 0;
    }
    std::shared_lock lock(m_Mutex);
    return ComputeFingerprintUnlocked(*m_Working);
}

std::uint32_t TypeRegistry::GetSchemaVersion() const {
    return kReflectionSchemaVersion;
}

void TypeRegistry::MarkTypeUsed(TypeId typeId) {
    if (typeId == kInvalidTypeId) {
        return;
    }
    std::lock_guard lock(m_UsedMutex);
    m_UsedTypes.insert(typeId);
}

bool TypeRegistry::IsTypeMarkedUsed(TypeId typeId) const {
    std::lock_guard lock(m_UsedMutex);
    return m_UsedTypes.count(typeId) != 0;
}

std::uint64_t TypeRegistry::BeginPluginRegistration(std::string_view pluginId) {
    const std::uint64_t token = PluginTokenCounter().fetch_add(1, std::memory_order_relaxed);
    {
        std::unique_lock lock(m_Mutex);
        m_ActivePluginToken = token;
        m_PluginIds[token] = std::string(pluginId);
    }
    return token;
}

void TypeRegistry::EndPluginRegistration() {
    m_ActivePluginToken = 0;
}

bool TypeRegistry::UnregisterByPluginToken(std::uint64_t token) {
    if (token == 0 || m_Sealed.load(std::memory_order_acquire)) {
        return false;
    }
    std::unique_lock lock(m_Mutex);
    std::vector<TypeId> toRemove;
    for (const auto& [id, record] : m_Working->byId) {
        if (record && record->pluginToken == token && m_BuiltinIds.count(id) == 0) {
            toRemove.push_back(id);
        }
    }
    if (toRemove.empty()) {
        return false;
    }
    for (TypeId id : toRemove) {
        const auto it = m_Working->byId.find(id);
        if (it == m_Working->byId.end() || !it->second) {
            continue;
        }
        m_Working->byQualifiedNameId.erase(it->second->info.qualifiedNameId);
        m_Working->byShortNameId.erase(it->second->info.nameId);
        m_Working->byId.erase(it);
        {
            std::lock_guard usedLock(m_UsedMutex);
            m_UsedTypes.erase(id);
        }
    }
    m_Working->allIdsSorted.erase(
        std::remove_if(
            m_Working->allIdsSorted.begin(),
            m_Working->allIdsSorted.end(),
            [&](TypeId id) { return m_Working->byId.find(id) == m_Working->byId.end(); }),
        m_Working->allIdsSorted.end());
    m_PluginIds.erase(token);
    m_GlobalCachesDirty = true;
    ClearPropertyPathCache();
    return true;
}

std::string TypeRegistry::GetPluginId(std::uint64_t token) const {
    std::shared_lock lock(m_Mutex);
    const auto it = m_PluginIds.find(token);
    return it == m_PluginIds.end() ? std::string{} : it->second;
}

std::vector<TypeId> TypeRegistry::GetTypesByPluginToken(std::uint64_t token) const {
    std::vector<TypeId> result;
    auto collect = [&](const RegistrySnapshot& snap) {
        for (TypeId id : snap.allIdsSorted) {
            const TypeRecord* record = FindRecord(snap, id);
            if (record && record->pluginToken == token) {
                result.push_back(id);
            }
        }
    };
    if (m_Sealed.load(std::memory_order_acquire)) {
        auto snap = LoadFrozenSnapshot();
        if (snap) {
            collect(*snap);
        }
        return result;
    }
    std::shared_lock lock(m_Mutex);
    collect(*m_Working);
    return result;
}

std::uint64_t TypeRegistry::GetOwningPluginToken(TypeId typeId) const {
    auto lookup = [&](const RegistrySnapshot& snap) -> std::uint64_t {
        const TypeRecord* record = FindRecord(snap, typeId);
        return record ? record->pluginToken : 0;
    };
    if (m_Sealed.load(std::memory_order_acquire)) {
        auto snap = LoadFrozenSnapshot();
        return snap ? lookup(*snap) : 0;
    }
    std::shared_lock lock(m_Mutex);
    return lookup(*m_Working);
}

std::unique_ptr<ITypeRegistry> CreateTypeRegistry(TypeRegistryDependencies deps) {
    return std::make_unique<TypeRegistry>(std::move(deps));
}

ITypeRegistry& GetTypeRegistry() {
    static std::unique_ptr<ITypeRegistry> instance = CreateTypeRegistry({});
    return *instance;
}

} // namespace we::runtime::reflection
