#include "Reflection/PluginOwnership.h"
#include "Reflection/ITypeRegistry.h"
#include "TypeRegistry.h"

#include <atomic>
#include <mutex>
#include <unordered_map>

namespace we::runtime::reflection {
namespace {

std::mutex& RetainMutex() {
    static std::mutex mutex;
    return mutex;
}

std::unordered_map<TypeId, std::atomic<std::uint32_t>, TypeIdHash>& RetainCounts() {
    static std::unordered_map<TypeId, std::atomic<std::uint32_t>, TypeIdHash> map;
    return map;
}

const TypeRegistry* AsRegistry(const ITypeRegistry& registry) {
    return static_cast<const TypeRegistry*>(&registry);
}

TypeRegistry* AsRegistryMutable(ITypeRegistry& registry) {
    return static_cast<TypeRegistry*>(&registry);
}

} // namespace

void RetainType(TypeId typeId) {
    if (typeId == kInvalidTypeId) {
        return;
    }
    std::lock_guard lock(RetainMutex());
    auto& map = RetainCounts();
    auto it = map.find(typeId);
    if (it == map.end()) {
        map.emplace(typeId, 1u);
        return;
    }
    it->second.fetch_add(1, std::memory_order_relaxed);
}

void ReleaseType(TypeId typeId) {
    if (typeId == kInvalidTypeId) {
        return;
    }
    std::lock_guard lock(RetainMutex());
    auto& map = RetainCounts();
    auto it = map.find(typeId);
    if (it == map.end()) {
        return;
    }
    const std::uint32_t prev = it->second.load(std::memory_order_relaxed);
    if (prev <= 1) {
        map.erase(it);
        return;
    }
    it->second.fetch_sub(1, std::memory_order_relaxed);
}

std::uint32_t GetTypeRetainCount(TypeId typeId) {
    std::lock_guard lock(RetainMutex());
    const auto it = RetainCounts().find(typeId);
    return it == RetainCounts().end() ? 0u : it->second.load(std::memory_order_relaxed);
}

std::string GetPluginIdForToken(const ITypeRegistry& registry, std::uint64_t token) {
    return registry.GetPluginId(token);
}

std::vector<TypeId> GetTypesOwnedByPlugin(const ITypeRegistry& registry, std::uint64_t token) {
    return registry.GetTypesByPluginToken(token);
}

PluginOwnershipInfo GetPluginOwnershipInfo(const ITypeRegistry& registry, std::uint64_t token) {
    PluginOwnershipInfo info;
    info.token = token;
    info.pluginId = registry.GetPluginId(token);
    info.typeIds = registry.GetTypesByPluginToken(token);
    for (TypeId id : info.typeIds) {
        info.totalRetainCount += GetTypeRetainCount(id);
    }
    return info;
}

bool IsPluginUnloadSafe(const ITypeRegistry& registry, std::uint64_t token) {
    if (token == 0 || registry.IsSealed()) {
        return false;
    }
    const auto types = registry.GetTypesByPluginToken(token);
    if (types.empty()) {
        return false;
    }
    for (TypeId id : types) {
        if (GetTypeRetainCount(id) > 0) {
            return false;
        }
    }
    return true;
}

bool UnloadPluginReflection(ITypeRegistry& registry, std::uint64_t token, bool reseal) {
    const bool wasSealed = registry.IsSealed();
    if (wasSealed) {
        registry.Unseal();
    }
    if (!IsPluginUnloadSafe(registry, token)) {
        if (wasSealed && reseal) {
            registry.Seal();
        }
        return false;
    }
    const bool ok = registry.UnregisterByPluginToken(token);
    if (reseal && (wasSealed || ok)) {
        registry.Seal();
    }
    (void)AsRegistry(registry);
    (void)AsRegistryMutable(registry);
    return ok;
}

} // namespace we::runtime::reflection
