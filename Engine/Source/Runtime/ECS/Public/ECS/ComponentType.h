#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "ECS/ComponentMask.h"
#include "ECS/Export.h"
#include "ECS/Types.h"

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::ecs {

using ComponentTypeId = std::uint32_t;
inline constexpr ComponentTypeId kInvalidComponentType = 0xFFFFFFFFu;

// Stable, explicit IDs — never use RTTI/typeid across DLL boundaries.
enum class CoreComponentId : ComponentTypeId {
    Name = 0,
    Tag,
    Uuid,
    Transform,
    Hierarchy,
    Visibility,
    Camera,
    DirectionalLight,
    PointLight,
    SpotLight,
    StaticMesh,
    Material,
    SkyAtmosphere,
    VolumetricCloud,
    Terrain,
    Water,
    Collider,
    RigidBody,
    AudioSource,
    Script,
    Animation,
    LOD,
    LegacyActor,
    Count
};

struct ComponentTypeInfo {
    ComponentTypeId id = kInvalidComponentType;
    std::string name;
    std::size_t size = 0;
    std::size_t alignment = 1;
    ComponentCategory category = ComponentCategory::Data;
    bool enableable = true;
    ComponentMask mask{};
};

template <typename T>
struct ComponentTypeTrait;

#define WE_ECS_DECLARE_COMPONENT(Type, EnumId) \
    template <> struct ComponentTypeTrait<Type> { \
        static constexpr ComponentTypeId Id() { return static_cast<ComponentTypeId>(CoreComponentId::EnumId); } \
        static constexpr const char* Name() { return #Type; } \
        static constexpr ComponentCategory Category() { return ComponentCategory::Data; } \
        static constexpr bool Enableable() { return true; } \
    }

#define WE_ECS_DECLARE_TAG_COMPONENT(Type, EnumId) \
    template <> struct ComponentTypeTrait<Type> { \
        static constexpr ComponentTypeId Id() { return static_cast<ComponentTypeId>(CoreComponentId::EnumId); } \
        static constexpr const char* Name() { return #Type; } \
        static constexpr ComponentCategory Category() { return ComponentCategory::Tag; } \
        static constexpr bool Enableable() { return false; } \
    }

class ECS_API ComponentTypeRegistry {
public:
    static ComponentTypeRegistry& Get();

    // Registers all CoreComponentId types + ops. Safe to call repeatedly.
    void EnsureCoreTypesRegistered();

    template <typename T>
    ComponentTypeId Id() {
        return ComponentTypeTrait<T>::Id();
    }

    template <typename T>
    ComponentTypeId Register(const char* nameOverride = nullptr) {
        return RegisterType(
            ComponentTypeTrait<T>::Id(),
            nameOverride ? nameOverride : ComponentTypeTrait<T>::Name(),
            ComponentTypeTrait<T>::Category() == ComponentCategory::Tag ? 0 : sizeof(T),
            ComponentTypeTrait<T>::Category() == ComponentCategory::Tag ? 1 : alignof(T),
            ComponentTypeTrait<T>::Category(),
            ComponentTypeTrait<T>::Enableable());
    }

    ComponentTypeId RegisterDynamic(
        const char* name,
        std::size_t size,
        std::size_t alignment,
        ComponentCategory category = ComponentCategory::Data,
        bool enableable = true);

    const ComponentTypeInfo* Find(ComponentTypeId id) const;
    [[nodiscard]] std::size_t Count() const { return m_Types.size(); }
    [[nodiscard]] const std::vector<ComponentTypeInfo>& All() const { return m_Types; }

private:
    ComponentTypeRegistry();
    ComponentTypeId RegisterType(
        ComponentTypeId id,
        const char* name,
        std::size_t size,
        std::size_t alignment,
        ComponentCategory category,
        bool enableable);

    std::vector<ComponentTypeInfo> m_Types;
    ComponentTypeId m_NextDynamicId = static_cast<ComponentTypeId>(CoreComponentId::Count);
    bool m_CoreRegistered = false;
};

} // namespace we::runtime::ecs

#pragma warning(pop)
