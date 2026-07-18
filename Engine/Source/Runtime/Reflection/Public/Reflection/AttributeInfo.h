#pragma once

#include "Reflection/Export.h"
#include "Reflection/NameId.h"
#include "Reflection/TypeId.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::reflection {

enum class AttributeValueKind : std::uint8_t {
    None = 0,
    Bool,
    Int64,
    Float,
    Double,
    String,
    TypeId,
    Bytes,
};

struct REFLECTION_API AttributeInfo {
    NameId nameId = kInvalidNameId;
    AttributeValueKind kind = AttributeValueKind::None;
    bool boolValue = false;
    std::int64_t intValue = 0;
    double floatValue = 0.0;
    TypeId typeIdValue = kInvalidTypeId;
    NameId stringValueId = kInvalidNameId; // interned when kind == String
    std::string name;                      // cold / display
    std::string stringValue;               // cold; prefer stringValueId when set
    std::vector<std::uint8_t> bytesValue;

    [[nodiscard]] static AttributeInfo MakeBool(std::string_view name, bool value);
    [[nodiscard]] static AttributeInfo MakeInt(std::string_view name, std::int64_t value);
    [[nodiscard]] static AttributeInfo MakeFloat(std::string_view name, double value);
    [[nodiscard]] static AttributeInfo MakeString(std::string_view name, std::string_view value);
    [[nodiscard]] static AttributeInfo MakeTypeId(std::string_view name, TypeId value);
    [[nodiscard]] static AttributeInfo MakeBytes(std::string_view name, const std::uint8_t* data, std::size_t size);
};

struct AttributeBag;

using AttributeLazyLoadFn = void (*)(AttributeBag& bag);

/// Contiguous attribute bag. Supports optional lazy population via loader callback.
struct REFLECTION_API AttributeBag {
    std::vector<AttributeInfo> entries;
    AttributeLazyLoadFn lazyLoader = nullptr;
    mutable bool loaded = true;

    void Add(AttributeInfo attr);
    void SetLazyLoader(AttributeLazyLoadFn loader);
    void EnsureLoaded() const;

    [[nodiscard]] const AttributeInfo* Find(std::string_view name) const noexcept;
    [[nodiscard]] const AttributeInfo* Find(NameId nameId) const noexcept;
    [[nodiscard]] bool Has(std::string_view name) const noexcept { return Find(name) != nullptr; }
    [[nodiscard]] bool Has(NameId nameId) const noexcept { return Find(nameId) != nullptr; }
    [[nodiscard]] bool Empty() const noexcept {
        EnsureLoaded();
        return entries.empty();
    }
    [[nodiscard]] std::size_t Size() const noexcept {
        EnsureLoaded();
        return entries.size();
    }
};

} // namespace we::runtime::reflection
