#pragma once

#include "Reflection/Export.h"
#include "Reflection/PropertyInfo.h"
#include "Reflection/TypeId.h"
#include "Reflection/TypeKinds.h"

#include <cstdint>
#include <vector>

namespace we::runtime::reflection {

/// One precomputed serialization step — no string lookup, no discovery at runtime.
struct REFLECTION_API SerializeStep {
    const PropertyInfo* property = nullptr;
    TypeId propertyTypeId = kInvalidTypeId;
    PrimitiveKind primitive = PrimitiveKind::None;
    std::uint32_t offset = 0;
    std::uint32_t size = 0;
    std::uint32_t schemaTag = 0;
    bool isNestedObject = false;
    bool isEnum = false;
    bool isString = false;
};

/// Cached, immutable traversal order for binary serialization of a type.
struct REFLECTION_API SerializePlan {
    TypeId typeId = kInvalidTypeId;
    std::uint32_t schemaVersion = 1;
    std::uint32_t binaryCompatibilityVersion = 1;
    std::vector<SerializeStep> steps;
    bool includeBases = true;

    [[nodiscard]] bool Empty() const noexcept { return steps.empty(); }
    [[nodiscard]] std::size_t Size() const noexcept { return steps.size(); }
};

} // namespace we::runtime::reflection
