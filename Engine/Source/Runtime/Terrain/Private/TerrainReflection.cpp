#include "Terrain/TerrainReflection.h"

#include "Terrain/TerrainTypes.h"
#include "Terrain/TerrainBrushSystem.h"
#include "Terrain/TerrainAsset.h"
#include "Reflection/BuiltinTypes.h"
#include "Reflection/Registration.h"

#include <cstddef>

namespace we::runtime::terrain {
namespace {

using reflection::MakeOffsetProperty;
using reflection::MakeTypeOpsFor;
using reflection::PropertyFlags;
using reflection::PrimitiveKind;
using reflection::RegisterMode;
using reflection::TypeBuilder;
using reflection::TypeKind;
namespace BuiltinTypeId = ::we::runtime::reflection::BuiltinTypeId;

} // namespace

void RegisterTerrainReflection(reflection::ITypeRegistry& registry) {
    TypeBuilder("we::runtime::terrain::TerrainBrushOp")
        .Kind(TypeKind::Enum)
        .Size(static_cast<std::uint32_t>(sizeof(TerrainBrushOp)))
        .Alignment(static_cast<std::uint32_t>(alignof(TerrainBrushOp)))
        .EnumValue("Raise", static_cast<std::int64_t>(TerrainBrushOp::Raise))
        .EnumValue("Lower", static_cast<std::int64_t>(TerrainBrushOp::Lower))
        .EnumValue("Smooth", static_cast<std::int64_t>(TerrainBrushOp::Smooth))
        .EnumValue("Flatten", static_cast<std::int64_t>(TerrainBrushOp::Flatten))
        .EnumValue("Noise", static_cast<std::int64_t>(TerrainBrushOp::Noise))
        .EnumValue("Paint", static_cast<std::int64_t>(TerrainBrushOp::Paint))
        .EnumValue("Ramp", static_cast<std::int64_t>(TerrainBrushOp::Ramp))
        .EnumValue("Terrace", static_cast<std::int64_t>(TerrainBrushOp::Terrace))
        .EnumValue("HydraulicErosion", static_cast<std::int64_t>(TerrainBrushOp::HydraulicErosion))
        .EnumValue("ThermalErosion", static_cast<std::int64_t>(TerrainBrushOp::ThermalErosion))
        .EnumValue("CustomAlpha", static_cast<std::int64_t>(TerrainBrushOp::CustomAlpha))
        .Register(registry, RegisterMode::Replace);

    TypeBuilder("we::runtime::terrain::TerrainCreationMethod")
        .Kind(TypeKind::Enum)
        .Size(static_cast<std::uint32_t>(sizeof(TerrainCreationMethod)))
        .Alignment(static_cast<std::uint32_t>(alignof(TerrainCreationMethod)))
        .EnumValue("Flat", static_cast<std::int64_t>(TerrainCreationMethod::Flat))
        .EnumValue("Empty", static_cast<std::int64_t>(TerrainCreationMethod::Empty))
        .EnumValue("Procedural", static_cast<std::int64_t>(TerrainCreationMethod::Procedural))
        .EnumValue("Noise", static_cast<std::int64_t>(TerrainCreationMethod::Noise))
        .EnumValue("Fractal", static_cast<std::int64_t>(TerrainCreationMethod::Fractal))
        .EnumValue("HeightmapImport", static_cast<std::int64_t>(TerrainCreationMethod::HeightmapImport))
        .EnumValue("PluginGenerator", static_cast<std::int64_t>(TerrainCreationMethod::PluginGenerator))
        .Register(registry, RegisterMode::Replace);

    TypeBuilder("we::runtime::terrain::TerrainGeneratorId")
        .Kind(TypeKind::Enum)
        .Size(static_cast<std::uint32_t>(sizeof(TerrainGeneratorId)))
        .Alignment(static_cast<std::uint32_t>(alignof(TerrainGeneratorId)))
        .EnumValue("Flat", static_cast<std::int64_t>(TerrainGeneratorId::Flat))
        .EnumValue("Empty", static_cast<std::int64_t>(TerrainGeneratorId::Empty))
        .EnumValue("PerlinNoise", static_cast<std::int64_t>(TerrainGeneratorId::PerlinNoise))
        .EnumValue("Fbm", static_cast<std::int64_t>(TerrainGeneratorId::Fbm))
        .EnumValue("RidgedNoise", static_cast<std::int64_t>(TerrainGeneratorId::RidgedNoise))
        .EnumValue("Voronoi", static_cast<std::int64_t>(TerrainGeneratorId::Voronoi))
        .EnumValue("Island", static_cast<std::int64_t>(TerrainGeneratorId::Island))
        .EnumValue("Mountain", static_cast<std::int64_t>(TerrainGeneratorId::Mountain))
        .EnumValue("Plateau", static_cast<std::int64_t>(TerrainGeneratorId::Plateau))
        .EnumValue("RandomSeed", static_cast<std::int64_t>(TerrainGeneratorId::RandomSeed))
        .Register(registry, RegisterMode::Replace);

    TypeBuilder("we::runtime::terrain::TerrainCreateInfo")
        .Kind(TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(TerrainCreateInfo)))
        .Alignment(static_cast<std::uint32_t>(alignof(TerrainCreateInfo)))
        .Ops(MakeTypeOpsFor<TerrainCreateInfo>())
        .Property(MakeOffsetProperty(
            "resolutionX",
            BuiltinTypeId::Int32(),
            static_cast<std::uint32_t>(offsetof(TerrainCreateInfo, resolutionX)),
            static_cast<std::uint32_t>(sizeof(int)),
            alignof(int)))
        .Property(MakeOffsetProperty(
            "resolutionY",
            BuiltinTypeId::Int32(),
            static_cast<std::uint32_t>(offsetof(TerrainCreateInfo, resolutionY)),
            static_cast<std::uint32_t>(sizeof(int)),
            alignof(int)))
        .Property(MakeOffsetProperty(
            "worldSizeX",
            BuiltinTypeId::Float(),
            static_cast<std::uint32_t>(offsetof(TerrainCreateInfo, worldSizeX)),
            static_cast<std::uint32_t>(sizeof(float)),
            alignof(float)))
        .Property(MakeOffsetProperty(
            "worldSizeY",
            BuiltinTypeId::Float(),
            static_cast<std::uint32_t>(offsetof(TerrainCreateInfo, worldSizeY)),
            static_cast<std::uint32_t>(sizeof(float)),
            alignof(float)))
        .Property(MakeOffsetProperty(
            "heightScale",
            BuiltinTypeId::Float(),
            static_cast<std::uint32_t>(offsetof(TerrainCreateInfo, heightScale)),
            static_cast<std::uint32_t>(sizeof(float)),
            alignof(float)))
        .Property(MakeOffsetProperty(
            "heightOffset",
            BuiltinTypeId::Float(),
            static_cast<std::uint32_t>(offsetof(TerrainCreateInfo, heightOffset)),
            static_cast<std::uint32_t>(sizeof(float)),
            alignof(float)))
        .Property(MakeOffsetProperty(
            "initialElevation",
            BuiltinTypeId::Float(),
            static_cast<std::uint32_t>(offsetof(TerrainCreateInfo, initialElevation)),
            static_cast<std::uint32_t>(sizeof(float)),
            alignof(float)))
        .Property(MakeOffsetProperty(
            "chunkQuads",
            BuiltinTypeId::Int32(),
            static_cast<std::uint32_t>(offsetof(TerrainCreateInfo, chunkQuads)),
            static_cast<std::uint32_t>(sizeof(int)),
            alignof(int)))
        .Property(MakeOffsetProperty(
            "tileSize",
            BuiltinTypeId::Int32(),
            static_cast<std::uint32_t>(offsetof(TerrainCreateInfo, tileSize)),
            static_cast<std::uint32_t>(sizeof(int)),
            alignof(int)))
        .Register(registry, RegisterMode::Replace);

    TypeBuilder("we::runtime::terrain::TerrainBrushSettings")
        .Kind(TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(TerrainBrushSettings)))
        .Alignment(static_cast<std::uint32_t>(alignof(TerrainBrushSettings)))
        .Ops(MakeTypeOpsFor<TerrainBrushSettings>())
        .Property(MakeOffsetProperty(
            "radius",
            BuiltinTypeId::Float(),
            static_cast<std::uint32_t>(offsetof(TerrainBrushSettings, radius)),
            static_cast<std::uint32_t>(sizeof(float)),
            alignof(float)))
        .Property(MakeOffsetProperty(
            "strength",
            BuiltinTypeId::Float(),
            static_cast<std::uint32_t>(offsetof(TerrainBrushSettings, strength)),
            static_cast<std::uint32_t>(sizeof(float)),
            alignof(float)))
        .Property(MakeOffsetProperty(
            "falloff",
            BuiltinTypeId::Float(),
            static_cast<std::uint32_t>(offsetof(TerrainBrushSettings, falloff)),
            static_cast<std::uint32_t>(sizeof(float)),
            alignof(float)))
        .Property(MakeOffsetProperty(
            "targetHeight",
            BuiltinTypeId::Float(),
            static_cast<std::uint32_t>(offsetof(TerrainBrushSettings, targetHeight)),
            static_cast<std::uint32_t>(sizeof(float)),
            alignof(float)))
        .Property(MakeOffsetProperty(
            "noiseScale",
            BuiltinTypeId::Float(),
            static_cast<std::uint32_t>(offsetof(TerrainBrushSettings, noiseScale)),
            static_cast<std::uint32_t>(sizeof(float)),
            alignof(float)))
        .Register(registry, RegisterMode::Replace);

    TypeBuilder("we::runtime::terrain::TerrainAssetDesc")
        .Kind(TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(TerrainAssetDesc)))
        .Alignment(static_cast<std::uint32_t>(alignof(TerrainAssetDesc)))
        .Ops(MakeTypeOpsFor<TerrainAssetDesc>())
        .Property(MakeOffsetProperty(
            "streamingEnabled",
            BuiltinTypeId::Bool(),
            static_cast<std::uint32_t>(offsetof(TerrainAssetDesc, streamingEnabled)),
            static_cast<std::uint32_t>(sizeof(bool)),
            alignof(bool)))
        .Property(MakeOffsetProperty(
            "maxResidentChunks",
            BuiltinTypeId::Int32(),
            static_cast<std::uint32_t>(offsetof(TerrainAssetDesc, maxResidentChunks)),
            static_cast<std::uint32_t>(sizeof(int)),
            alignof(int)))
        .Register(registry, RegisterMode::Replace);
}

} // namespace we::runtime::terrain
