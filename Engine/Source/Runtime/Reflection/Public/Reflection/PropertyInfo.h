#pragma once

#include "Reflection/AttributeInfo.h"
#include "Reflection/Export.h"
#include "Reflection/NameId.h"
#include "Reflection/TypeId.h"
#include "Reflection/TypeKinds.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace we::runtime::reflection {

using PropertyGetterFn = bool (*)(const void* instance, void* outValue, std::size_t outSize);
using PropertySetterFn = bool (*)(void* instance, const void* value, std::size_t valueSize);

/// Reflected field / property. Layout-oriented for binary serialization and editors.
/// Hot-path fields are grouped first for cache locality; nameId enables O(1) registry lookup.
struct REFLECTION_API PropertyInfo {
    // --- Hot path (frequently read during serialize / iterate) ---
    TypeId typeId = kInvalidTypeId;
    TypeId ownerTypeId = kInvalidTypeId;
    std::uint32_t offset = 0;
    std::uint32_t size = 0;
    PropertyFlags flags = PropertyFlags::Serialize | PropertyFlags::Editable;
    PrimitiveKind primitive = PrimitiveKind::None;
    std::uint16_t alignment = 1;
    std::uint16_t reserved = 0;
    NameId nameId = kInvalidNameId;
    std::uint32_t index = 0;
    std::uint32_t schemaTag = 0;
    PropertyGetterFn getter = nullptr;
    PropertySetterFn setter = nullptr;

    // --- Cold path ---
    std::string name;
    AttributeBag attributes;

    [[nodiscard]] bool IsSerialized() const noexcept {
        return HasFlag(flags, PropertyFlags::Serialize) && !HasFlag(flags, PropertyFlags::Transient);
    }

    [[nodiscard]] bool HasDirectStorage() const noexcept {
        return size > 0 && getter == nullptr;
    }

    [[nodiscard]] void* MutablePtr(void* instance) const noexcept {
        if (!instance || size == 0) {
            return nullptr;
        }
        return static_cast<std::uint8_t*>(instance) + offset;
    }

    [[nodiscard]] const void* ConstPtr(const void* instance) const noexcept {
        if (!instance || size == 0) {
            return nullptr;
        }
        return static_cast<const std::uint8_t*>(instance) + offset;
    }
};

} // namespace we::runtime::reflection
