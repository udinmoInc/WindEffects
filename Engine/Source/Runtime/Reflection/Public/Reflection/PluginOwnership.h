#pragma once

#include "Reflection/Export.h"
#include "Reflection/TypeId.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::reflection {

class ITypeRegistry;

struct REFLECTION_API PluginOwnershipInfo {
    std::uint64_t token = 0;
    std::string pluginId;
    std::vector<TypeId> typeIds;
    std::uint32_t totalRetainCount = 0;
};

/// Process-wide retain/release for live consumers of a reflected type (editor, assets, etc.).
/// Plugin unload is refused while any owned type has retainCount > 0.
REFLECTION_API void RetainType(TypeId typeId);
REFLECTION_API void ReleaseType(TypeId typeId);
[[nodiscard]] REFLECTION_API std::uint32_t GetTypeRetainCount(TypeId typeId);

/// Query plugin registration ownership recorded by BeginPluginRegistration.
[[nodiscard]] REFLECTION_API std::string GetPluginIdForToken(
    const ITypeRegistry& registry,
    std::uint64_t token);

[[nodiscard]] REFLECTION_API std::vector<TypeId> GetTypesOwnedByPlugin(
    const ITypeRegistry& registry,
    std::uint64_t token);

[[nodiscard]] REFLECTION_API PluginOwnershipInfo GetPluginOwnershipInfo(
    const ITypeRegistry& registry,
    std::uint64_t token);

/// True when the registry is unsealed and no owned type has outstanding retains.
[[nodiscard]] REFLECTION_API bool IsPluginUnloadSafe(
    const ITypeRegistry& registry,
    std::uint64_t token);

/// Unseal → unregister by token → optional reseal. Returns false if retains remain or unregister fails.
[[nodiscard]] REFLECTION_API bool UnloadPluginReflection(
    ITypeRegistry& registry,
    std::uint64_t token,
    bool reseal = true);

} // namespace we::runtime::reflection
