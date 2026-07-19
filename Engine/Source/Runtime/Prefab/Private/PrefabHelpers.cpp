#include "PrefabInternal.h"

#include <random>

namespace we::runtime::prefab {
namespace detail {

PrefabGuid GeneratePrefabGuid() {
    thread_local std::mt19937_64 rng{std::random_device{}()};
    PrefabGuid g;
    do {
        g.hi = rng();
        g.lo = rng();
    } while (!g.IsValid());
    return g;
}

std::string EntityTypeToName(scene::EntityType type) {
    using T = scene::EntityType;
    switch (type) {
    case T::EmptyActor: return "EmptyActor";
    case T::Character: return "Character";
    case T::Blueprint: return "Blueprint";
    case T::Cube: return "Cube";
    case T::Sphere: return "Sphere";
    case T::Cylinder: return "Cylinder";
    case T::Plane: return "Plane";
    case T::DirectionalLight: return "DirectionalLight";
    case T::PointLight: return "PointLight";
    case T::SpotLight: return "SpotLight";
    case T::SkyLight: return "SkyLight";
    case T::SkyAtmosphere: return "SkyAtmosphere";
    case T::HeightFog: return "HeightFog";
    case T::VolumetricClouds: return "VolumetricClouds";
    case T::Landscape: return "Landscape";
    case T::GroundPlane: return "GroundPlane";
    case T::CameraIcon: return "CameraIcon";
    case T::AudioSource: return "AudioSource";
    case T::Volume: return "Volume";
    }
    return "EmptyActor";
}

scene::EntityType EntityTypeFromName(std::string_view name) {
    using T = scene::EntityType;
    if (name == "Character") return T::Character;
    if (name == "Blueprint") return T::Blueprint;
    if (name == "Cube") return T::Cube;
    if (name == "Sphere") return T::Sphere;
    if (name == "Cylinder") return T::Cylinder;
    if (name == "Plane") return T::Plane;
    if (name == "DirectionalLight") return T::DirectionalLight;
    if (name == "PointLight") return T::PointLight;
    if (name == "SpotLight") return T::SpotLight;
    if (name == "SkyLight") return T::SkyLight;
    if (name == "SkyAtmosphere") return T::SkyAtmosphere;
    if (name == "HeightFog") return T::HeightFog;
    if (name == "VolumetricClouds") return T::VolumetricClouds;
    if (name == "Landscape") return T::Landscape;
    if (name == "GroundPlane") return T::GroundPlane;
    if (name == "CameraIcon") return T::CameraIcon;
    if (name == "AudioSource") return T::AudioSource;
    if (name == "Volume") return T::Volume;
    return T::EmptyActor;
}

PrefabGuid WorldGuidToPrefabGuid(const world::WorldGuid& guid) {
    PrefabGuid out{};
    for (int i = 0; i < 8; ++i) {
        out.hi |= static_cast<std::uint64_t>(guid.bytes[static_cast<std::size_t>(i)]) << (8 * (7 - i));
        out.lo |= static_cast<std::uint64_t>(guid.bytes[static_cast<std::size_t>(8 + i)]) << (8 * (7 - i));
    }
    return out;
}

world::WorldGuid PrefabGuidToWorldGuid(const PrefabGuid& guid) {
    world::WorldGuid out = world::WorldGuid::Nil();
    for (int i = 0; i < 8; ++i) {
        out.bytes[static_cast<std::size_t>(i)] =
            static_cast<std::uint8_t>((guid.hi >> (8 * (7 - i))) & 0xFFu);
        out.bytes[static_cast<std::size_t>(8 + i)] =
            static_cast<std::uint8_t>((guid.lo >> (8 * (7 - i))) & 0xFFu);
    }
    return out;
}

} // namespace detail
} // namespace we::runtime::prefab
