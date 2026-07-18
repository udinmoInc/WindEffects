#pragma once

#include "Reflection/Export.h"
#include "Reflection/TypeId.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::reflection {

/// Attribute value kinds stored as native binary-friendly payloads.
enum class AttributeValueKind : std::uint8_t {
    None = 0,
    Bool,
    Int64,
    Float,
    Double,
    String,
    TypeId,
    Bytes, // opaque blob (e.g. color, custom binary tag)
};

/// Single named attribute attached to a type, property, or function.
/// Attributes are the engine-native replacement for annotation metadata —
/// no text formats involved.
struct REFLECTION_API AttributeInfo {
    std::string name;
    AttributeValueKind kind = AttributeValueKind::None;

    bool boolValue = false;
    std::int64_t intValue = 0;
    double floatValue = 0.0;
    std::string stringValue;
    TypeId typeIdValue = kInvalidTypeId;
    std::vector<std::uint8_t> bytesValue;

    [[nodiscard]] static AttributeInfo MakeBool(std::string_view name, bool value);
    [[nodiscard]] static AttributeInfo MakeInt(std::string_view name, std::int64_t value);
    [[nodiscard]] static AttributeInfo MakeFloat(std::string_view name, double value);
    [[nodiscard]] static AttributeInfo MakeString(std::string_view name, std::string_view value);
    [[nodiscard]] static AttributeInfo MakeTypeId(std::string_view name, TypeId value);
    [[nodiscard]] static AttributeInfo MakeBytes(std::string_view name, const std::uint8_t* data, std::size_t size);
};

/// Contiguous attribute bag with O(n) lookup by name (n is typically tiny).
struct REFLECTION_API AttributeBag {
    std::vector<AttributeInfo> entries;

    void Add(AttributeInfo attr);
    [[nodiscard]] const AttributeInfo* Find(std::string_view name) const noexcept;
    [[nodiscard]] bool Has(std::string_view name) const noexcept { return Find(name) != nullptr; }
    [[nodiscard]] bool Empty() const noexcept { return entries.empty(); }
    [[nodiscard]] std::size_t Size() const noexcept { return entries.size(); }
};

} // namespace we::runtime::reflection
