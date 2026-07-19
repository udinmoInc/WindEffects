#include "World/WorldReflection.h"

#include "Reflection/BuiltinTypes.h"
#include "Reflection/Registration.h"

#include <cstddef>

namespace we::runtime::world {
namespace {

void RegisterWorldGuid(reflection::ITypeRegistry& registry) {
    using reflection::MakeOffsetProperty;
    using reflection::MakeTypeOpsFor;
    using reflection::RegisterMode;
    using reflection::TypeBuilder;
    using reflection::TypeKind;

    TypeBuilder("we::runtime::world::WorldGuid")
        .Kind(TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(WorldGuid)))
        .Alignment(static_cast<std::uint32_t>(alignof(WorldGuid)))
        .Ops(MakeTypeOpsFor<WorldGuid>())
        .SchemaVersion(1)
        .Property(MakeOffsetProperty(
            "bytes",
            reflection::BuiltinTypeId::UInt8(),
            static_cast<std::uint32_t>(offsetof(WorldGuid, bytes)),
            static_cast<std::uint32_t>(sizeof(WorldGuid::bytes)),
            static_cast<std::uint16_t>(alignof(std::uint8_t))))
        .Register(registry, RegisterMode::Replace);
}

void RegisterWorldDescriptor(reflection::ITypeRegistry& registry) {
    using reflection::MakeOffsetProperty;
    using reflection::MakeTypeOpsFor;
    using reflection::RegisterMode;
    using reflection::TypeBuilder;
    using reflection::TypeKind;

    TypeBuilder("we::runtime::world::WorldDescriptor")
        .Kind(TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(WorldDescriptor)))
        .Alignment(static_cast<std::uint32_t>(alignof(WorldDescriptor)))
        .Ops(MakeTypeOpsFor<WorldDescriptor>())
        .SchemaVersion(1)
        .Property(MakeOffsetProperty(
            "schemaVersion",
            reflection::BuiltinTypeId::UInt32(),
            static_cast<std::uint32_t>(offsetof(WorldDescriptor, schemaVersion)),
            static_cast<std::uint32_t>(sizeof(std::uint32_t)),
            static_cast<std::uint16_t>(alignof(std::uint32_t))))
        .Property(MakeOffsetProperty(
            "persistent",
            reflection::BuiltinTypeId::Bool(),
            static_cast<std::uint32_t>(offsetof(WorldDescriptor, persistent)),
            static_cast<std::uint32_t>(sizeof(bool)),
            static_cast<std::uint16_t>(alignof(bool))))
        .Register(registry, RegisterMode::Replace);
}

void RegisterLevelDescriptor(reflection::ITypeRegistry& registry) {
    using reflection::MakeOffsetProperty;
    using reflection::MakeTypeOpsFor;
    using reflection::RegisterMode;
    using reflection::TypeBuilder;
    using reflection::TypeKind;

    TypeBuilder("we::runtime::world::LevelDescriptor")
        .Kind(TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(LevelDescriptor)))
        .Alignment(static_cast<std::uint32_t>(alignof(LevelDescriptor)))
        .Ops(MakeTypeOpsFor<LevelDescriptor>())
        .SchemaVersion(1)
        .Property(MakeOffsetProperty(
            "schemaVersion",
            reflection::BuiltinTypeId::UInt32(),
            static_cast<std::uint32_t>(offsetof(LevelDescriptor, schemaVersion)),
            static_cast<std::uint32_t>(sizeof(std::uint32_t)),
            static_cast<std::uint16_t>(alignof(std::uint32_t))))
        .Register(registry, RegisterMode::Replace);
}

void RegisterActorSpawnParams(reflection::ITypeRegistry& registry) {
    using reflection::MakeOffsetProperty;
    using reflection::MakeTypeOpsFor;
    using reflection::RegisterMode;
    using reflection::TypeBuilder;
    using reflection::TypeKind;

    TypeBuilder("we::runtime::world::ActorSpawnParams")
        .Kind(TypeKind::Struct)
        .Size(static_cast<std::uint32_t>(sizeof(ActorSpawnParams)))
        .Alignment(static_cast<std::uint32_t>(alignof(ActorSpawnParams)))
        .Ops(MakeTypeOpsFor<ActorSpawnParams>())
        .SchemaVersion(1)
        .Property(MakeOffsetProperty(
            "beginPlayImmediately",
            reflection::BuiltinTypeId::Bool(),
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
