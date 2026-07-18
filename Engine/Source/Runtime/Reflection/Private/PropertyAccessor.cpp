#include "Reflection/IPropertyAccessor.h"
#include "Reflection/ITypeRegistry.h"

#include <cstring>

namespace we::runtime::reflection {
namespace {

class PropertyAccessor final : public IPropertyAccessor {
public:
    explicit PropertyAccessor(ITypeRegistry* registry) : m_Registry(registry) {}

    [[nodiscard]] bool GetRaw(
        TypeId ownerTypeId,
        const void* instance,
        std::string_view propertyName,
        void* outValue,
        std::size_t outSize) const override
    {
        const PropertyInfo* property = FindProperty(ownerTypeId, propertyName);
        if (!property) {
            return false;
        }
        return GetRaw(*property, instance, outValue, outSize);
    }

    [[nodiscard]] bool SetRaw(
        TypeId ownerTypeId,
        void* instance,
        std::string_view propertyName,
        const void* value,
        std::size_t valueSize) const override
    {
        const PropertyInfo* property = FindProperty(ownerTypeId, propertyName);
        if (!property) {
            return false;
        }
        return SetRaw(*property, instance, value, valueSize);
    }

    [[nodiscard]] bool GetRaw(
        const PropertyInfo& property,
        const void* instance,
        void* outValue,
        std::size_t outSize) const override
    {
        if (!instance || !outValue || outSize == 0) {
            return false;
        }
        if (property.getter) {
            return property.getter(instance, outValue, outSize);
        }
        if (property.size == 0 || outSize < property.size) {
            return false;
        }
        const void* src = property.ConstPtr(instance);
        if (!src) {
            return false;
        }
        std::memcpy(outValue, src, property.size);
        return true;
    }

    [[nodiscard]] bool SetRaw(
        const PropertyInfo& property,
        void* instance,
        const void* value,
        std::size_t valueSize) const override
    {
        if (!instance || !value || valueSize == 0) {
            return false;
        }
        if (HasFlag(property.flags, PropertyFlags::ReadOnly) && !property.setter) {
            return false;
        }
        if (property.setter) {
            return property.setter(instance, value, valueSize);
        }
        if (property.size == 0 || valueSize < property.size) {
            return false;
        }
        void* dst = property.MutablePtr(instance);
        if (!dst) {
            return false;
        }
        std::memcpy(dst, value, property.size);
        return true;
    }

    [[nodiscard]] bool GetBool(TypeId ownerTypeId, const void* instance, std::string_view name, bool& out) const override {
        return GetRaw(ownerTypeId, instance, name, &out, sizeof(out));
    }
    [[nodiscard]] bool SetBool(TypeId ownerTypeId, void* instance, std::string_view name, bool value) const override {
        return SetRaw(ownerTypeId, instance, name, &value, sizeof(value));
    }
    [[nodiscard]] bool GetInt32(TypeId ownerTypeId, const void* instance, std::string_view name, std::int32_t& out) const override {
        return GetRaw(ownerTypeId, instance, name, &out, sizeof(out));
    }
    [[nodiscard]] bool SetInt32(TypeId ownerTypeId, void* instance, std::string_view name, std::int32_t value) const override {
        return SetRaw(ownerTypeId, instance, name, &value, sizeof(value));
    }
    [[nodiscard]] bool GetUInt32(TypeId ownerTypeId, const void* instance, std::string_view name, std::uint32_t& out) const override {
        return GetRaw(ownerTypeId, instance, name, &out, sizeof(out));
    }
    [[nodiscard]] bool SetUInt32(TypeId ownerTypeId, void* instance, std::string_view name, std::uint32_t value) const override {
        return SetRaw(ownerTypeId, instance, name, &value, sizeof(value));
    }
    [[nodiscard]] bool GetInt64(TypeId ownerTypeId, const void* instance, std::string_view name, std::int64_t& out) const override {
        return GetRaw(ownerTypeId, instance, name, &out, sizeof(out));
    }
    [[nodiscard]] bool SetInt64(TypeId ownerTypeId, void* instance, std::string_view name, std::int64_t value) const override {
        return SetRaw(ownerTypeId, instance, name, &value, sizeof(value));
    }
    [[nodiscard]] bool GetFloat(TypeId ownerTypeId, const void* instance, std::string_view name, float& out) const override {
        return GetRaw(ownerTypeId, instance, name, &out, sizeof(out));
    }
    [[nodiscard]] bool SetFloat(TypeId ownerTypeId, void* instance, std::string_view name, float value) const override {
        return SetRaw(ownerTypeId, instance, name, &value, sizeof(value));
    }
    [[nodiscard]] bool GetDouble(TypeId ownerTypeId, const void* instance, std::string_view name, double& out) const override {
        return GetRaw(ownerTypeId, instance, name, &out, sizeof(out));
    }
    [[nodiscard]] bool SetDouble(TypeId ownerTypeId, void* instance, std::string_view name, double value) const override {
        return SetRaw(ownerTypeId, instance, name, &value, sizeof(value));
    }

    [[nodiscard]] bool GetString(
        TypeId ownerTypeId,
        const void* instance,
        std::string_view name,
        std::string& out) const override
    {
        const PropertyInfo* property = FindProperty(ownerTypeId, name);
        if (!property || !instance) {
            return false;
        }
        if (property->primitive == PrimitiveKind::String || property->size == sizeof(std::string)) {
            if (property->getter) {
                return property->getter(instance, &out, sizeof(std::string));
            }
            const auto* src = static_cast<const std::string*>(property->ConstPtr(instance));
            if (!src) {
                return false;
            }
            out = *src;
            return true;
        }
        return false;
    }

    [[nodiscard]] bool SetString(
        TypeId ownerTypeId,
        void* instance,
        std::string_view name,
        std::string_view value) const override
    {
        const PropertyInfo* property = FindProperty(ownerTypeId, name);
        if (!property || !instance) {
            return false;
        }
        if (HasFlag(property->flags, PropertyFlags::ReadOnly) && !property->setter) {
            return false;
        }
        std::string temp(value);
        if (property->setter) {
            return property->setter(instance, &temp, sizeof(std::string));
        }
        if (property->primitive == PrimitiveKind::String || property->size == sizeof(std::string)) {
            auto* dst = static_cast<std::string*>(property->MutablePtr(instance));
            if (!dst) {
                return false;
            }
            *dst = std::move(temp);
            return true;
        }
        return false;
    }

private:
    [[nodiscard]] const PropertyInfo* FindProperty(TypeId ownerTypeId, std::string_view name) const {
        if (!m_Registry) {
            return nullptr;
        }
        const TypeInfo* info = m_Registry->Resolve(ownerTypeId);
        if (!info) {
            return nullptr;
        }
        if (const PropertyInfo* direct = info->FindProperty(name)) {
            return direct;
        }
        // Search bases.
        for (TypeId baseId : info->baseTypes) {
            if (const PropertyInfo* found = FindProperty(baseId, name)) {
                return found;
            }
        }
        return nullptr;
    }

    ITypeRegistry* m_Registry = nullptr;
};

} // namespace

std::unique_ptr<IPropertyAccessor> CreatePropertyAccessor(PropertyAccessorDependencies deps) {
    return std::make_unique<PropertyAccessor>(deps.registry);
}

} // namespace we::runtime::reflection
