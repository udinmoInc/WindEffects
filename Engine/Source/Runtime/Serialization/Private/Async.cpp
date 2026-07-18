#include "Serialization/Async.h"

#include <utility>

namespace we::runtime::serialization {

std::future<AsyncSerializeResult> SerializeObjectAsync(
    std::shared_ptr<ISerializer> serializer,
    reflection::TypeId typeId,
    std::vector<std::uint8_t> instanceCopy,
    SerializeOptions options)
{
    return std::async(std::launch::async, [serializer = std::move(serializer), typeId,
                                           instanceCopy = std::move(instanceCopy),
                                           options]() mutable {
        AsyncSerializeResult result;
        if (!serializer || instanceCopy.empty()) {
            result.error = "invalid_args";
            return result;
        }
        result.bytes = serializer->SerializeObject(typeId, instanceCopy.data(), options);
        result.success = !result.bytes.empty();
        if (!result.success) {
            result.error = "serialize_failed";
        }
        return result;
    });
}

std::future<AsyncDeserializeResult> DeserializeObjectAsync(
    std::shared_ptr<ISerializer> serializer,
    reflection::TypeId typeId,
    std::vector<std::uint8_t> documentBytes,
    std::vector<std::uint8_t> instanceBuffer,
    SerializeOptions options)
{
    return std::async(std::launch::async, [serializer = std::move(serializer), typeId,
                                           documentBytes = std::move(documentBytes),
                                           instanceBuffer = std::move(instanceBuffer),
                                           options]() mutable {
        AsyncDeserializeResult result;
        if (!serializer || documentBytes.empty() || instanceBuffer.empty()) {
            result.error = "invalid_args";
            return result;
        }
        result.success = serializer->DeserializeObject(
            typeId,
            instanceBuffer.data(),
            documentBytes.data(),
            documentBytes.size(),
            options);
        if (!result.success) {
            result.error = "deserialize_failed";
        }
        return result;
    });
}

std::future<AsyncSerializeResult> SerializeGraphAsync(
    std::shared_ptr<ISerializer> serializer,
    std::shared_ptr<ObjectGraph> graph,
    SerializeOptions options)
{
    return std::async(std::launch::async, [serializer = std::move(serializer),
                                           graph = std::move(graph), options]() {
        AsyncSerializeResult result;
        if (!serializer || !graph) {
            result.error = "invalid_args";
            return result;
        }
        result.bytes = serializer->SerializeGraph(*graph, options);
        result.success = !result.bytes.empty();
        if (!result.success) {
            result.error = "serialize_graph_failed";
        }
        return result;
    });
}

} // namespace we::runtime::serialization
