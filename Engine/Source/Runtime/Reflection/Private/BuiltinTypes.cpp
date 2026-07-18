#include "Reflection/BuiltinTypes.h"
#include "Reflection/Registration.h"
#include "Reflection/TypeKinds.h"

#include "Core/Math/Types.h"

#include <array>
#include <cstdint>
#include <string>

namespace we::runtime::reflection {
namespace {

template <typename T>
void RegisterPrimitive(
    ITypeRegistry& registry,
    std::string_view qualifiedName,
    PrimitiveKind kind,
    TypeFlags extraFlags = TypeFlags::None)
{
    TypeBuilder(qualifiedName)
        .Primitive(kind)
        .Size(static_cast<std::uint32_t>(sizeof(T)))
        .Alignment(static_cast<std::uint32_t>(alignof(T)))
        .Flags(TypeFlags::TrivialCopy | TypeFlags::TrivialDestroy | TypeFlags::Pod | extraFlags)
        .Ops(MakeTypeOpsFor<T>())
        .Register(registry);
}

void RegisterVoid(ITypeRegistry& registry) {
    TypeBuilder("void")
        .Primitive(PrimitiveKind::Void)
        .Size(0)
        .Alignment(1)
        .Flags(TypeFlags::None)
        .Register(registry);
}

} // namespace

namespace BuiltinTypeId {
TypeId Void() { return MakeTypeId("void"); }
TypeId Bool() { return MakeTypeId("bool"); }
TypeId Int8() { return MakeTypeId("int8"); }
TypeId UInt8() { return MakeTypeId("uint8"); }
TypeId Int16() { return MakeTypeId("int16"); }
TypeId UInt16() { return MakeTypeId("uint16"); }
TypeId Int32() { return MakeTypeId("int32"); }
TypeId UInt32() { return MakeTypeId("uint32"); }
TypeId Int64() { return MakeTypeId("int64"); }
TypeId UInt64() { return MakeTypeId("uint64"); }
TypeId Float() { return MakeTypeId("float"); }
TypeId Double() { return MakeTypeId("double"); }
TypeId Char() { return MakeTypeId("char"); }
TypeId String() { return MakeTypeId("string"); }
TypeId Vec2() { return MakeTypeId("we::math::Vec2"); }
TypeId Vec3() { return MakeTypeId("we::math::Vec3"); }
TypeId Vec4() { return MakeTypeId("we::math::Vec4"); }
TypeId Quat() { return MakeTypeId("we::math::Quat"); }
TypeId Mat4() { return MakeTypeId("we::math::Mat4"); }
TypeId Guid() { return MakeTypeId("we::Guid"); }
} // namespace BuiltinTypeId

void RegisterBuiltinTypes(ITypeRegistry& registry) {
    RegisterVoid(registry);
    RegisterPrimitive<bool>(registry, "bool", PrimitiveKind::Bool);
    RegisterPrimitive<std::int8_t>(registry, "int8", PrimitiveKind::Int8);
    RegisterPrimitive<std::uint8_t>(registry, "uint8", PrimitiveKind::UInt8);
    RegisterPrimitive<std::int16_t>(registry, "int16", PrimitiveKind::Int16);
    RegisterPrimitive<std::uint16_t>(registry, "uint16", PrimitiveKind::UInt16);
    RegisterPrimitive<std::int32_t>(registry, "int32", PrimitiveKind::Int32);
    RegisterPrimitive<std::uint32_t>(registry, "uint32", PrimitiveKind::UInt32);
    RegisterPrimitive<std::int64_t>(registry, "int64", PrimitiveKind::Int64);
    RegisterPrimitive<std::uint64_t>(registry, "uint64", PrimitiveKind::UInt64);
    RegisterPrimitive<float>(registry, "float", PrimitiveKind::Float);
    RegisterPrimitive<double>(registry, "double", PrimitiveKind::Double);
    RegisterPrimitive<char>(registry, "char", PrimitiveKind::Char);

    TypeBuilder("string")
        .Primitive(PrimitiveKind::String)
        .Size(static_cast<std::uint32_t>(sizeof(std::string)))
        .Alignment(static_cast<std::uint32_t>(alignof(std::string)))
        .Flags(TypeFlags::Instantiable)
        .Ops(MakeTypeOpsFor<std::string>())
        .Register(registry);

    RegisterPrimitive<we::math::Vec2>(registry, "we::math::Vec2", PrimitiveKind::Vec2);
    RegisterPrimitive<we::math::Vec3>(registry, "we::math::Vec3", PrimitiveKind::Vec3);
    RegisterPrimitive<we::math::Vec4>(registry, "we::math::Vec4", PrimitiveKind::Vec4);
    RegisterPrimitive<we::math::Quat>(registry, "we::math::Quat", PrimitiveKind::Quat);
    RegisterPrimitive<we::math::Mat4>(registry, "we::math::Mat4", PrimitiveKind::Mat4);

    using GuidBytes = std::array<std::uint8_t, 16>;
    RegisterPrimitive<GuidBytes>(registry, "we::Guid", PrimitiveKind::Guid);
}

} // namespace we::runtime::reflection
