#pragma once

#include "Reflection/Export.h"
#include "Reflection/PropertyInfo.h"
#include "Reflection/TypeId.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace we::runtime::reflection {

class ITypeRegistry;

/// Generic property get/set against a live object instance using reflection metadata.
/// Supports direct-offset POD fields and optional getter/setter function pointers.
class REFLECTION_API IPropertyAccessor {
public:
    virtual ~IPropertyAccessor() = default;

    [[nodiscard]] virtual bool GetRaw(
        TypeId ownerTypeId,
        const void* instance,
        std::string_view propertyName,
        void* outValue,
        std::size_t outSize) const = 0;

    [[nodiscard]] virtual bool SetRaw(
        TypeId ownerTypeId,
        void* instance,
        std::string_view propertyName,
        const void* value,
        std::size_t valueSize) const = 0;

    [[nodiscard]] virtual bool GetRaw(
        const PropertyInfo& property,
        const void* instance,
        void* outValue,
        std::size_t outSize) const = 0;

    [[nodiscard]] virtual bool SetRaw(
        const PropertyInfo& property,
        void* instance,
        const void* value,
        std::size_t valueSize) const = 0;

    /// Typed helpers for common primitives.
    [[nodiscard]] virtual bool GetBool(TypeId ownerTypeId, const void* instance, std::string_view name, bool& out) const = 0;
    [[nodiscard]] virtual bool SetBool(TypeId ownerTypeId, void* instance, std::string_view name, bool value) const = 0;
    [[nodiscard]] virtual bool GetInt32(TypeId ownerTypeId, const void* instance, std::string_view name, std::int32_t& out) const = 0;
    [[nodiscard]] virtual bool SetInt32(TypeId ownerTypeId, void* instance, std::string_view name, std::int32_t value) const = 0;
    [[nodiscard]] virtual bool GetUInt32(TypeId ownerTypeId, const void* instance, std::string_view name, std::uint32_t& out) const = 0;
    [[nodiscard]] virtual bool SetUInt32(TypeId ownerTypeId, void* instance, std::string_view name, std::uint32_t value) const = 0;
    [[nodiscard]] virtual bool GetInt64(TypeId ownerTypeId, const void* instance, std::string_view name, std::int64_t& out) const = 0;
    [[nodiscard]] virtual bool SetInt64(TypeId ownerTypeId, void* instance, std::string_view name, std::int64_t value) const = 0;
    [[nodiscard]] virtual bool GetFloat(TypeId ownerTypeId, const void* instance, std::string_view name, float& out) const = 0;
    [[nodiscard]] virtual bool SetFloat(TypeId ownerTypeId, void* instance, std::string_view name, float value) const = 0;
    [[nodiscard]] virtual bool GetDouble(TypeId ownerTypeId, const void* instance, std::string_view name, double& out) const = 0;
    [[nodiscard]] virtual bool SetDouble(TypeId ownerTypeId, void* instance, std::string_view name, double value) const = 0;
    [[nodiscard]] virtual bool GetString(TypeId ownerTypeId, const void* instance, std::string_view name, std::string& out) const = 0;
    [[nodiscard]] virtual bool SetString(TypeId ownerTypeId, void* instance, std::string_view name, std::string_view value) const = 0;
};

struct REFLECTION_API PropertyAccessorDependencies {
    ITypeRegistry* registry = nullptr; // required; not owned
};

[[nodiscard]] REFLECTION_API std::unique_ptr<IPropertyAccessor> CreatePropertyAccessor(
    PropertyAccessorDependencies deps);

} // namespace we::runtime::reflection
