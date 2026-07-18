#pragma once

#include "Reflection/AttributeInfo.h"
#include "Reflection/Export.h"
#include "Reflection/TypeId.h"
#include "Reflection/TypeKinds.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace we::runtime::reflection {

/// Optional typed accessors for non-POD / computed properties.
/// Signatures are C-callable across DLL boundaries (no exceptions required).
using PropertyGetterFn = bool (*)(const void* instance, void* outValue, std::size_t outSize);
using PropertySetterFn = bool (*)(void* instance, const void* value, std::size_t valueSize);

/// Reflected field / property. Layout-oriented for binary serialization and editors.
struct REFLECTION_API PropertyInfo {
    std::string name;
    TypeId typeId = kInvalidTypeId;       // type of the property value
    TypeId ownerTypeId = kInvalidTypeId;  // declaring type
    std::uint32_t offset = 0;             // byte offset within owner (0 if getter-only)
    std::uint32_t size = 0;               // byte size of the value storage
    std::uint16_t alignment = 1;
    PropertyFlags flags = PropertyFlags::Serialize | PropertyFlags::Editable;
    PrimitiveKind primitive = PrimitiveKind::None; // fast-path when value is primitive
    PropertyGetterFn getter = nullptr;
    PropertySetterFn setter = nullptr;
    AttributeBag attributes;
    std::uint32_t index = 0;              // stable index within declaring type
    std::uint32_t schemaTag = 0;          // optional field tag for versioned binary formats

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
