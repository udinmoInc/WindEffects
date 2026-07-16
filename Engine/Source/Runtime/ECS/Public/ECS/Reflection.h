#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "ECS/ComponentType.h"
#include "ECS/Export.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::ecs {

enum class ReflectionFieldType : std::uint8_t {
    Bool,
    Int32,
    UInt32,
    Float,
    Vec3,
    Vec4,
    String,
    Entity,
    AssetId,
    Unknown
};

struct ReflectionField {
    std::string name;
    ReflectionFieldType type = ReflectionFieldType::Unknown;
    std::uint32_t offset = 0;
    std::uint32_t size = 0;
};

struct ComponentReflection {
    ComponentTypeId typeId = kInvalidComponentType;
    std::string typeName;
    std::vector<ReflectionField> fields;
};

class ECS_API ReflectionRegistry {
public:
    static ReflectionRegistry& Get();

    void RegisterComponent(const ComponentReflection& reflection);
    [[nodiscard]] const ComponentReflection* Find(ComponentTypeId typeId) const;
    [[nodiscard]] const std::vector<ComponentReflection>& All() const { return m_Components; }

    // Serialization hooks consume reflection metadata.
    [[nodiscard]] std::vector<std::uint8_t> SerializeComponent(
        ComponentTypeId typeId,
        const void* componentData) const;
    bool DeserializeComponent(
        ComponentTypeId typeId,
        void* componentData,
        const std::uint8_t* bytes,
        std::size_t byteCount) const;

private:
    ReflectionRegistry() = default;
    std::vector<ComponentReflection> m_Components;
};

#define WE_ECS_REFLECT_FIELD(Type, Field, FieldType) \
    { #Field, ReflectionFieldType::FieldType, \
      static_cast<std::uint32_t>(offsetof(Type, Field)), \
      static_cast<std::uint32_t>(sizeof(((Type*)nullptr)->Field)) }

} // namespace we::runtime::ecs

#pragma warning(pop)
