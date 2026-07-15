#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "ECS/Export.h"

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
    std::size_t alignment = 0;
};

template <typename T>
struct ComponentTypeTrait;

#define WE_ECS_DECLARE_COMPONENT(Type, EnumId) \
    template <> struct ComponentTypeTrait<Type> { \
        static constexpr ComponentTypeId Id() { return static_cast<ComponentTypeId>(CoreComponentId::EnumId); } \
        static constexpr const char* Name() { return #Type; } \
    }

class ECS_API ComponentTypeRegistry {
public:
    static ComponentTypeRegistry& Get();

    template <typename T>
    ComponentTypeId Id() {
        return ComponentTypeTrait<T>::Id();
    }

    template <typename T>
    ComponentTypeId Register(const char* /*name*/ = nullptr) {
        EnsureRegistered<T>();
        return ComponentTypeTrait<T>::Id();
    }

    const ComponentTypeInfo* Find(ComponentTypeId id) const;
    std::size_t Count() const { return static_cast<std::size_t>(CoreComponentId::Count); }

private:
    ComponentTypeRegistry();

    template <typename T>
    void EnsureRegistered() {
        const ComponentTypeId id = ComponentTypeTrait<T>::Id();
        if (id >= m_Types.size()) {
            m_Types.resize(static_cast<std::size_t>(CoreComponentId::Count));
        }
        if (m_Types[id].id == kInvalidComponentType) {
            m_Types[id].id = id;
            m_Types[id].name = ComponentTypeTrait<T>::Name();
            m_Types[id].size = sizeof(T);
            m_Types[id].alignment = alignof(T);
        }
    }

    std::vector<ComponentTypeInfo> m_Types;
};

} // namespace we::runtime::ecs

#pragma warning(pop)
