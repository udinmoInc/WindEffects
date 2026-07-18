#pragma once

#include "Serialization/Export.h"
#include "Serialization/ISerializer.h"
#include "Serialization/Types.h"
#include "Reflection/TypeId.h"

#include <cstdint>
#include <future>
#include <memory>
#include <vector>

namespace we::runtime::serialization {

struct SERIALIZATION_API AsyncSerializeResult {
    bool success = false;
    std::vector<std::uint8_t> bytes;
    std::string error;
};

struct SERIALIZATION_API AsyncDeserializeResult {
    bool success = false;
    std::string error;
};

/// Fire-and-forget async serialize of a single object (copies instance bytes first if size known).
[[nodiscard]] SERIALIZATION_API std::future<AsyncSerializeResult> SerializeObjectAsync(
    std::shared_ptr<ISerializer> serializer,
    reflection::TypeId typeId,
    std::vector<std::uint8_t> instanceCopy,
    SerializeOptions options = {});

[[nodiscard]] SERIALIZATION_API std::future<AsyncDeserializeResult> DeserializeObjectAsync(
    std::shared_ptr<ISerializer> serializer,
    reflection::TypeId typeId,
    std::vector<std::uint8_t> documentBytes,
    std::vector<std::uint8_t> instanceBuffer,
    SerializeOptions options = {});

[[nodiscard]] SERIALIZATION_API std::future<AsyncSerializeResult> SerializeGraphAsync(
    std::shared_ptr<ISerializer> serializer,
    std::shared_ptr<ObjectGraph> graph,
    SerializeOptions options = {});

} // namespace we::runtime::serialization
