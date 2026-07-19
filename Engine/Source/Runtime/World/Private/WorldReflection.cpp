#include "World/WorldReflection.h"

#include "Environment/EnvironmentDirectionalLight.h"
#include "Environment/EnvironmentExposureController.h"
#include "Environment/EnvironmentHeightFog.h"
#include "Environment/EnvironmentSkyAtmosphere.h"
#include "Environment/EnvironmentSkyLight.h"
#include "Environment/EnvironmentVolumetricClouds.h"
#include "Reflection/AttributeInfo.h"
#include "Reflection/BuiltinTypes.h"
#include "Reflection/Registration.h"
#include "Scene/Entity.h"

#include <cstddef>
#include <utility>

#ifdef Float
#undef Float
#endif
#ifdef abs
#undef abs
#endif

namespace we::runtime::world {
namespace {

using reflection::AttributeInfo;
namespace BuiltinTypeId = ::we::runtime::reflection::BuiltinTypeId;
using reflection::MakeOffsetProperty;
using reflection::MakeTypeOpsFor;
using reflection::MakeTypeId;
using reflection::PropertyFlags;
using reflection::PropertyInfo;
using reflection::PrimitiveKind;
using reflection::RegisterMode;
using reflection::TypeBuilder;
using reflection::TypeKind;
using reflection::TypeId;

using scene::Entity;
using scene::EntityType;

using environment::CloudPreset;
using environment::CloudQualityPreset;
using environment::EnvironmentDirectionalLight;
using environment::EnvironmentExposureController;
using environment::EnvironmentHeightFog;
using environment::EnvironmentSkyAtmosphere;
using environment::EnvironmentSkyLight;
using environment::EnvironmentVolumetricClouds;

PropertyInfo WithCategory(PropertyInfo property, std::string_view category) {
    property.attributes.Add(AttributeInfo::MakeString("Category", category));
    return property;
}

TypeId RegisterEntityType(reflection::ITypeRegistry& registry) {
    const TypeId entityTypeId = MakeTypeId("we::runtime::scene::EntityType");
    TypeBuilder("we::runtime::scene::EntityType")
        .Kind(TypeKind::Enum)
        .Size(static_cast<std::uint32_t>(sizeof(EntityType)))
        .Alignment(static_cast<std::uint32_t>(alignof(EntityType)))
        .EnumValue("EmptyActor", static_cast<std::int64_t>(EntityType::EmptyActor))
        .EnumValue("Character", static_cast<std::int64_t>(EntityType::Character))
        .EnumValue("Blueprint", static_cast<std::int64_t>(EntityType::Blueprint))
        .EnumValue("Cube", static_cast<std::int64_t>(EntityType::Cube))
        .EnumValue("Sphere", static_cast<std::int64_t>(EntityType::Sphere))
        .EnumValue("Cylinder", static_cast<std::int64_t>(EntityType::Cylinder))
        .EnumValue("Plane", static_cast<std::int64_t>(EntityType::Plane))
        .EnumValue("DirectionalLight", static_cast<std::int64_t>(EntityType::DirectionalLight))
        .EnumValue("PointLight", static_cast<std::int64_t>(EntityType::PointLight))
        .EnumValue("SpotLight", static_cast<std::int64_t>(EntityType::SpotLight))
        .EnumValue("SkyLight", static_cast<std::int64_t>(EntityType::SkyLight))
        .EnumValue("SkyAtmosphere", static_cast<std::int64_t>(EntityType::SkyAtmosphere))
        .EnumValue("HeightFog", static_cast<std::int64_t>(EntityType::HeightFog))
        .EnumValue("VolumetricClouds", static_cast<std::int64_t>(EntityType::VolumetricClouds))
        .EnumValue("Landscape", static_cast<std::int64_t>(EntityType::Landscape))
        .EnumValue("GroundPlane", static_cast<std::int64_t>(EntityType::GroundPlane))
        .EnumValue("CameraIcon", static_cast<std::int64_t>(EntityType::CameraIcon))
        .EnumValue("AudioSource", static_cast<std::int64_t>(EntityType::AudioSource))
        .EnumValue("Volume", static_cast<std::int64_t>(EntityType::Volume))
        .Register(registry, RegisterMode::Replace);
    return entityTypeId;
}

void RegisterCloudQualityPreset(reflection::ITypeRegistry& registry) {
    TypeBuilder("we::runtime::world::environment::CloudQualityPreset")
        .Kind(TypeKind::Enum)
        .Size(static_cast<std::uint32_t>(sizeof(CloudQualityPreset)))
        .Alignment(static_cast<std::uint32_t>(alignof(CloudQualityPreset)))
        .EnumValue("Low", static_cast<std::int64_t>(CloudQualityPreset::Low))
        .EnumValue("Medium", static_cast<std::int64_t>(CloudQualityPreset::Medium))
        .EnumValue("High", static_cast<std::int64_t>(CloudQualityPreset::High))
        .EnumValue("Epic", static_cast<std::int64_t>(CloudQualityPreset::Epic))
        .Register(registry, RegisterMode::Replace);
}

void RegisterCloudPreset(reflection::ITypeRegistry& registry) {
    TypeBuilder("we::runtime::world::environment::CloudPreset")
        .Kind(TypeKind::Enum)
        .Size(static_cast<std::uint32_t>(sizeof(CloudPreset)))
        .Alignment(static_cast<std::uint32_t>(alignof(CloudPreset)))
        .EnumValue("ClearSky", static_cast<std::int64_t>(CloudPreset::ClearSky))
        .EnumValue("FewClouds", static_cast<std::int64_t>(CloudPreset::FewClouds))
        .EnumValue("ScatteredClouds", static_cast<std::int64_t>(CloudPreset::ScatteredClouds))
        .EnumValue("BrokenClouds", static_cast<std::int64_t>(CloudPreset::BrokenClouds))
        .EnumValue("Overcast", static_cast<std::int64_t>(CloudPreset::Overcast))
        .EnumValue("Storm", static_cast<std::int64_t>(CloudPreset::Storm))
        .EnumValue("HeavyStorm", static_cast<std::int64_t>(CloudPreset::HeavyStorm))
        .EnumValue("SunsetClouds", static_cast<std::int64_t>(CloudPreset::SunsetClouds))
        .EnumValue("SunriseClouds", static_cast<std::int64_t>(CloudPreset::SunriseClouds))
        .EnumValue("HighCirrus", static_cast<std::int64_t>(CloudPreset::HighCirrus))
        .EnumValue("Cumulus", static_cast<std::int64_t>(CloudPreset::Cumulus))
        .EnumValue("Stratocumulus", static_cast<std::int64_t>(CloudPreset::Stratocumulus))
        .Register(registry, RegisterMode::Replace);
}

void RegisterEntity(reflection::ITypeRegistry& registry) {
    const TypeId entityTypeId = RegisterEntityType(registry);

    TypeBuilder builder("we::runtime::scene::Entity");
    builder.Kind(TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(Entity)))
        .Alignment(static_cast<std::uint32_t>(alignof(Entity)))
        .Ops(MakeTypeOpsFor<Entity>())
        .SchemaVersion(1)
        .Property(WithCategory(
            MakeOffsetProperty(
                "Name",
                BuiltinTypeId::String(),
                static_cast<std::uint32_t>(offsetof(Entity, Name)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<Entity>().Name))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<Entity>().Name))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::String),
            "Actor"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "Type",
                entityTypeId,
                static_cast<std::uint32_t>(offsetof(Entity, Type)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<Entity>().Type))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<Entity>().Type))),
                PropertyFlags::Serialize | PropertyFlags::ReadOnly),
            "Actor"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "Position",
                BuiltinTypeId::Vec3(),
                static_cast<std::uint32_t>(offsetof(Entity, Position)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<Entity>().Position))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<Entity>().Position))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Vec3),
            "Transform"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "Rotation",
                BuiltinTypeId::Vec3(),
                static_cast<std::uint32_t>(offsetof(Entity, Rotation)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<Entity>().Rotation))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<Entity>().Rotation))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Vec3),
            "Transform"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "Scale",
                BuiltinTypeId::Vec3(),
                static_cast<std::uint32_t>(offsetof(Entity, Scale)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<Entity>().Scale))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<Entity>().Scale))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Vec3),
            "Transform"));
    builder.Register(registry, RegisterMode::Replace);
}

void RegisterEnvironmentDirectionalLight(reflection::ITypeRegistry& registry) {
    TypeBuilder builder("we::runtime::world::environment::EnvironmentDirectionalLight");
    builder.Kind(TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(EnvironmentDirectionalLight)))
        .Alignment(static_cast<std::uint32_t>(alignof(EnvironmentDirectionalLight)))
        .Ops(MakeTypeOpsFor<EnvironmentDirectionalLight>())
        .SchemaVersion(1)
        .Property(WithCategory(
            MakeOffsetProperty(
                "Intensity",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentDirectionalLight, Intensity)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentDirectionalLight>().Intensity))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentDirectionalLight>().Intensity))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Light"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "TemperatureKelvin",
                BuiltinTypeId::Int32(),
                static_cast<std::uint32_t>(offsetof(EnvironmentDirectionalLight, TemperatureKelvin)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentDirectionalLight>().TemperatureKelvin))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentDirectionalLight>().TemperatureKelvin))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Int32),
            "Light"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "CastDynamicShadows",
                BuiltinTypeId::Bool(),
                static_cast<std::uint32_t>(offsetof(EnvironmentDirectionalLight, CastDynamicShadows)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentDirectionalLight>().CastDynamicShadows))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentDirectionalLight>().CastDynamicShadows))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Bool),
            "Light"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "AtmosphereSun",
                BuiltinTypeId::Bool(),
                static_cast<std::uint32_t>(offsetof(EnvironmentDirectionalLight, AtmosphereSun)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentDirectionalLight>().AtmosphereSun))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentDirectionalLight>().AtmosphereSun))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Bool),
            "Light"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "Rotation",
                BuiltinTypeId::Vec3(),
                static_cast<std::uint32_t>(offsetof(EnvironmentDirectionalLight, Rotation)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentDirectionalLight>().Rotation))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentDirectionalLight>().Rotation))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Vec3),
            "Transform"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "Color",
                BuiltinTypeId::Vec3(),
                static_cast<std::uint32_t>(offsetof(EnvironmentDirectionalLight, Color)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentDirectionalLight>().Color))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentDirectionalLight>().Color))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Vec3),
            "Light"));
    builder.Register(registry, RegisterMode::Replace);
}

void RegisterEnvironmentSkyLight(reflection::ITypeRegistry& registry) {
    TypeBuilder builder("we::runtime::world::environment::EnvironmentSkyLight");
    builder.Kind(TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(EnvironmentSkyLight)))
        .Alignment(static_cast<std::uint32_t>(alignof(EnvironmentSkyLight)))
        .Ops(MakeTypeOpsFor<EnvironmentSkyLight>())
        .SchemaVersion(1)
        .Property(WithCategory(
            MakeOffsetProperty(
                "Intensity",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentSkyLight, Intensity)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentSkyLight>().Intensity))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentSkyLight>().Intensity))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Sky Light"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "RealTimeCapture",
                BuiltinTypeId::Bool(),
                static_cast<std::uint32_t>(offsetof(EnvironmentSkyLight, RealTimeCapture)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentSkyLight>().RealTimeCapture))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentSkyLight>().RealTimeCapture))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Bool),
            "Sky Light"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "LowerHemisphereColor",
                BuiltinTypeId::Vec3(),
                static_cast<std::uint32_t>(offsetof(EnvironmentSkyLight, LowerHemisphereColor)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentSkyLight>().LowerHemisphereColor))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentSkyLight>().LowerHemisphereColor))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Vec3),
            "Sky Light"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "UpperHemisphereColor",
                BuiltinTypeId::Vec3(),
                static_cast<std::uint32_t>(offsetof(EnvironmentSkyLight, UpperHemisphereColor)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentSkyLight>().UpperHemisphereColor))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentSkyLight>().UpperHemisphereColor))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Vec3),
            "Sky Light"));
    builder.Register(registry, RegisterMode::Replace);
}

void RegisterEnvironmentSkyAtmosphere(reflection::ITypeRegistry& registry) {
    TypeBuilder builder("we::runtime::world::environment::EnvironmentSkyAtmosphere");
    builder.Kind(TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(EnvironmentSkyAtmosphere)))
        .Alignment(static_cast<std::uint32_t>(alignof(EnvironmentSkyAtmosphere)))
        .Ops(MakeTypeOpsFor<EnvironmentSkyAtmosphere>())
        .SchemaVersion(1)
        .Property(WithCategory(
            MakeOffsetProperty(
                "RayleighScattering",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentSkyAtmosphere, RayleighScattering)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentSkyAtmosphere>().RayleighScattering))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentSkyAtmosphere>().RayleighScattering))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Atmosphere"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "MieScattering",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentSkyAtmosphere, MieScattering)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentSkyAtmosphere>().MieScattering))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentSkyAtmosphere>().MieScattering))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Atmosphere"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "MieAnisotropy",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentSkyAtmosphere, MieAnisotropy)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentSkyAtmosphere>().MieAnisotropy))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentSkyAtmosphere>().MieAnisotropy))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Atmosphere"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "MultiScatterStrength",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentSkyAtmosphere, MultiScatterStrength)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentSkyAtmosphere>().MultiScatterStrength))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentSkyAtmosphere>().MultiScatterStrength))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Atmosphere"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "EyeAltitude",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentSkyAtmosphere, EyeAltitude)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentSkyAtmosphere>().EyeAltitude))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentSkyAtmosphere>().EyeAltitude))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Atmosphere"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "OzoneAbsorption",
                BuiltinTypeId::Vec3(),
                static_cast<std::uint32_t>(offsetof(EnvironmentSkyAtmosphere, OzoneAbsorption)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentSkyAtmosphere>().OzoneAbsorption))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentSkyAtmosphere>().OzoneAbsorption))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Vec3),
            "Atmosphere"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "AerialPerspectiveStartDepth",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentSkyAtmosphere, AerialPerspectiveStartDepth)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentSkyAtmosphere>().AerialPerspectiveStartDepth))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentSkyAtmosphere>().AerialPerspectiveStartDepth))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Atmosphere"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "GroundAlbedo",
                BuiltinTypeId::Vec3(),
                static_cast<std::uint32_t>(offsetof(EnvironmentSkyAtmosphere, GroundAlbedo)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentSkyAtmosphere>().GroundAlbedo))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentSkyAtmosphere>().GroundAlbedo))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Vec3),
            "Atmosphere"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "AtmosphereDebugMode",
                BuiltinTypeId::Int32(),
                static_cast<std::uint32_t>(offsetof(EnvironmentSkyAtmosphere, AtmosphereDebugMode)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentSkyAtmosphere>().AtmosphereDebugMode))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentSkyAtmosphere>().AtmosphereDebugMode))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Int32),
            "Atmosphere"));
    builder.Register(registry, RegisterMode::Replace);
}

void RegisterEnvironmentHeightFog(reflection::ITypeRegistry& registry) {
    TypeBuilder builder("we::runtime::world::environment::EnvironmentHeightFog");
    builder.Kind(TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(EnvironmentHeightFog)))
        .Alignment(static_cast<std::uint32_t>(alignof(EnvironmentHeightFog)))
        .Ops(MakeTypeOpsFor<EnvironmentHeightFog>())
        .SchemaVersion(1)
        .Property(WithCategory(
            MakeOffsetProperty(
                "Density",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentHeightFog, Density)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentHeightFog>().Density))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentHeightFog>().Density))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Fog"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "HeightFalloff",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentHeightFog, HeightFalloff)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentHeightFog>().HeightFalloff))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentHeightFog>().HeightFalloff))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Fog"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "StartDistance",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentHeightFog, StartDistance)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentHeightFog>().StartDistance))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentHeightFog>().StartDistance))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Fog"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "VolumetricFog",
                BuiltinTypeId::Bool(),
                static_cast<std::uint32_t>(offsetof(EnvironmentHeightFog, VolumetricFog)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentHeightFog>().VolumetricFog))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentHeightFog>().VolumetricFog))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Bool),
            "Fog"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "FogColor",
                BuiltinTypeId::Vec3(),
                static_cast<std::uint32_t>(offsetof(EnvironmentHeightFog, FogColor)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentHeightFog>().FogColor))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentHeightFog>().FogColor))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Vec3),
            "Fog"));
    builder.Register(registry, RegisterMode::Replace);
}

void RegisterEnvironmentExposureController(reflection::ITypeRegistry& registry) {
    TypeBuilder builder("we::runtime::world::environment::EnvironmentExposureController");
    builder.Kind(TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(EnvironmentExposureController)))
        .Alignment(static_cast<std::uint32_t>(alignof(EnvironmentExposureController)))
        .Ops(MakeTypeOpsFor<EnvironmentExposureController>())
        .SchemaVersion(1)
        .Property(WithCategory(
            MakeOffsetProperty(
                "ExposureEV",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentExposureController, ExposureEV)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentExposureController>().ExposureEV))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentExposureController>().ExposureEV))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Exposure"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "ExposureCompensation",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentExposureController, ExposureCompensation)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentExposureController>().ExposureCompensation))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentExposureController>().ExposureCompensation))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Exposure"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "MinEV",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentExposureController, MinEV)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentExposureController>().MinEV))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentExposureController>().MinEV))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Exposure"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "MaxEV",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentExposureController, MaxEV)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentExposureController>().MaxEV))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentExposureController>().MaxEV))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Exposure"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "AutoExposure",
                BuiltinTypeId::Bool(),
                static_cast<std::uint32_t>(offsetof(EnvironmentExposureController, AutoExposure)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentExposureController>().AutoExposure))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentExposureController>().AutoExposure))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Bool),
            "Exposure"));
    builder.Register(registry, RegisterMode::Replace);
}

void RegisterEnvironmentVolumetricClouds(reflection::ITypeRegistry& registry) {
    TypeBuilder builder("we::runtime::world::environment::EnvironmentVolumetricClouds");
    builder.Kind(TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(EnvironmentVolumetricClouds)))
        .Alignment(static_cast<std::uint32_t>(alignof(EnvironmentVolumetricClouds)))
        .Ops(MakeTypeOpsFor<EnvironmentVolumetricClouds>())
        .SchemaVersion(1)
        .Property(WithCategory(
            MakeOffsetProperty(
                "Enabled",
                BuiltinTypeId::Bool(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, Enabled)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().Enabled))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().Enabled))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Bool),
            "Clouds"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "Coverage",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, Coverage)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().Coverage))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().Coverage))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Clouds"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "Density",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, Density)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().Density))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().Density))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Clouds"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "DensityMultiplier",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, DensityMultiplier)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().DensityMultiplier))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().DensityMultiplier))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Clouds"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "CloudHeight",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, CloudHeight)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().CloudHeight))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().CloudHeight))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Clouds"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "CloudThickness",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, CloudThickness)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().CloudThickness))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().CloudThickness))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Clouds"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "BottomAltitude",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, BottomAltitude)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().BottomAltitude))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().BottomAltitude))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Clouds"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "TopAltitude",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, TopAltitude)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().TopAltitude))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().TopAltitude))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Clouds"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "WindDirection",
                BuiltinTypeId::Vec3(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, WindDirection)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().WindDirection))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().WindDirection))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Vec3),
            "Clouds"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "WindSpeed",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, WindSpeed)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().WindSpeed))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().WindSpeed))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Clouds"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "AnimationSpeed",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, AnimationSpeed)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().AnimationSpeed))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().AnimationSpeed))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Clouds"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "NoiseScale",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, NoiseScale)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().NoiseScale))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().NoiseScale))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Clouds"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "DetailNoiseScale",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, DetailNoiseScale)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().DetailNoiseScale))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().DetailNoiseScale))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Clouds"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "ShapeNoise",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, ShapeNoise)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().ShapeNoise))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().ShapeNoise))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Clouds"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "ErosionNoise",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, ErosionNoise)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().ErosionNoise))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().ErosionNoise))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Clouds"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "Seed",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, Seed)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().Seed))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().Seed))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Clouds"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "LightingIntensity",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, LightingIntensity)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().LightingIntensity))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().LightingIntensity))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Lighting"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "SilverLiningIntensity",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, SilverLiningIntensity)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().SilverLiningIntensity))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().SilverLiningIntensity))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Lighting"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "AmbientContribution",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, AmbientContribution)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().AmbientContribution))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().AmbientContribution))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Lighting"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "MultiScatteringStrength",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, MultiScatteringStrength)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().MultiScatteringStrength))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().MultiScatteringStrength))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Lighting"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "PhaseG",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, PhaseG)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().PhaseG))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().PhaseG))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Lighting"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "PowderEffect",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, PowderEffect)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().PowderEffect))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().PowderEffect))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Lighting"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "WeatherMapInfluence",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, WeatherMapInfluence)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().WeatherMapInfluence))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().WeatherMapInfluence))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Weather"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "ShadowStrength",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, ShadowStrength)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().ShadowStrength))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().ShadowStrength))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Shadows"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "ShadowDistance",
                BuiltinTypeId::Float(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, ShadowDistance)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().ShadowDistance))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().ShadowDistance))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Float),
            "Shadows"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "CloudColor",
                BuiltinTypeId::Vec3(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, CloudColor)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().CloudColor))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().CloudColor))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Vec3),
            "Appearance"))
        .Property(WithCategory(
            MakeOffsetProperty(
                "CloudColorTint",
                BuiltinTypeId::Vec3(),
                static_cast<std::uint32_t>(offsetof(EnvironmentVolumetricClouds, CloudColorTint)),
                static_cast<std::uint32_t>(sizeof(decltype(std::declval<EnvironmentVolumetricClouds>().CloudColorTint))),
                static_cast<std::uint16_t>(alignof(decltype(std::declval<EnvironmentVolumetricClouds>().CloudColorTint))),
                PropertyFlags::Serialize | PropertyFlags::Editable,
                PrimitiveKind::Vec3),
            "Appearance"));
    builder.Register(registry, RegisterMode::Replace);
}

void RegisterWorldGuid(reflection::ITypeRegistry& registry) {
    TypeBuilder("we::runtime::world::WorldGuid")
        .Kind(TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(WorldGuid)))
        .Alignment(static_cast<std::uint32_t>(alignof(WorldGuid)))
        .Ops(MakeTypeOpsFor<WorldGuid>())
        .SchemaVersion(1)
        .Property(MakeOffsetProperty(
            "bytes",
            BuiltinTypeId::UInt8(),
            static_cast<std::uint32_t>(offsetof(WorldGuid, bytes)),
            static_cast<std::uint32_t>(sizeof(decltype(std::declval<WorldGuid>().bytes))),
            static_cast<std::uint16_t>(alignof(decltype(std::declval<WorldGuid>().bytes)))))
        .Register(registry, RegisterMode::Replace);
}

void RegisterWorldDescriptor(reflection::ITypeRegistry& registry) {
    TypeBuilder("we::runtime::world::WorldDescriptor")
        .Kind(TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(WorldDescriptor)))
        .Alignment(static_cast<std::uint32_t>(alignof(WorldDescriptor)))
        .Ops(MakeTypeOpsFor<WorldDescriptor>())
        .SchemaVersion(1)
        .Property(MakeOffsetProperty(
            "schemaVersion",
            BuiltinTypeId::UInt32(),
            static_cast<std::uint32_t>(offsetof(WorldDescriptor, schemaVersion)),
            static_cast<std::uint32_t>(sizeof(std::uint32_t)),
            static_cast<std::uint16_t>(alignof(std::uint32_t))))
        .Property(MakeOffsetProperty(
            "persistent",
            BuiltinTypeId::Bool(),
            static_cast<std::uint32_t>(offsetof(WorldDescriptor, persistent)),
            static_cast<std::uint32_t>(sizeof(bool)),
            static_cast<std::uint16_t>(alignof(bool))))
        .Register(registry, RegisterMode::Replace);
}

void RegisterLevelDescriptor(reflection::ITypeRegistry& registry) {
    TypeBuilder("we::runtime::world::LevelDescriptor")
        .Kind(TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(LevelDescriptor)))
        .Alignment(static_cast<std::uint32_t>(alignof(LevelDescriptor)))
        .Ops(MakeTypeOpsFor<LevelDescriptor>())
        .SchemaVersion(1)
        .Property(MakeOffsetProperty(
            "schemaVersion",
            BuiltinTypeId::UInt32(),
            static_cast<std::uint32_t>(offsetof(LevelDescriptor, schemaVersion)),
            static_cast<std::uint32_t>(sizeof(std::uint32_t)),
            static_cast<std::uint16_t>(alignof(std::uint32_t))))
        .Register(registry, RegisterMode::Replace);
}

void RegisterActorSpawnParams(reflection::ITypeRegistry& registry) {
    TypeBuilder("we::runtime::world::ActorSpawnParams")
        .Kind(TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(ActorSpawnParams)))
        .Alignment(static_cast<std::uint32_t>(alignof(ActorSpawnParams)))
        .Ops(MakeTypeOpsFor<ActorSpawnParams>())
        .SchemaVersion(1)
        .Property(MakeOffsetProperty(
            "beginPlayImmediately",
            BuiltinTypeId::Bool(),
            static_cast<std::uint32_t>(offsetof(ActorSpawnParams, beginPlayImmediately)),
            static_cast<std::uint32_t>(sizeof(bool)),
            static_cast<std::uint16_t>(alignof(bool))))
        .Register(registry, RegisterMode::Replace);
}

} // namespace

void WorldTypeRegistrar::RegisterTypes(reflection::ITypeRegistry& registry) {
    const bool wasSealed = registry.IsSealed();
    if (wasSealed) {
        registry.Unseal();
    }
    RegisterWorldGuid(registry);
    RegisterWorldDescriptor(registry);
    RegisterLevelDescriptor(registry);
    RegisterActorSpawnParams(registry);
    RegisterEntity(registry);
    RegisterCloudQualityPreset(registry);
    RegisterCloudPreset(registry);
    RegisterEnvironmentDirectionalLight(registry);
    RegisterEnvironmentSkyLight(registry);
    RegisterEnvironmentSkyAtmosphere(registry);
    RegisterEnvironmentHeightFog(registry);
    RegisterEnvironmentExposureController(registry);
    RegisterEnvironmentVolumetricClouds(registry);
    if (wasSealed) {
        registry.Seal();
    }
}

void WorldTypeRegistrar::UnregisterTypes(reflection::ITypeRegistry& registry) {
    const bool wasSealed = registry.IsSealed();
    if (wasSealed) {
        registry.Unseal();
    }
    (void)registry;
    if (wasSealed) {
        registry.Seal();
    }
}

void RegisterWorldReflectionTypes(reflection::ITypeRegistry& registry) {
    WorldTypeRegistrar registrar;
    registrar.RegisterTypes(registry);
}

} // namespace we::runtime::world
