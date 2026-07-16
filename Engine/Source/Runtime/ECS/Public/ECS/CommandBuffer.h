#pragma once

#pragma warning(push)
#pragma warning(disable: 4251)

#include "ECS/ComponentType.h"
#include "ECS/Entity.h"
#include "ECS/Export.h"
#include "ECS/Types.h"

#include <cstdint>
#include <mutex>
#include <vector>

namespace we::runtime::ecs {

class World;

struct StructuralCommand {
    StructuralChangeType type = StructuralChangeType::CreateEntity;
    Entity entity{};
    ComponentTypeId componentType = kInvalidComponentType;
    bool boolValue = false;
    Entity parent{};
    std::uint32_t prefabId = 0;
    std::vector<std::uint8_t> payload;
};

class ECS_API CommandBuffer {
public:
    CommandBuffer() = default;

    Entity CreateEntity(World& world);
    void DestroyEntity(Entity entity);
    void SetEntityEnabled(Entity entity, bool enabled);

    template <typename T>
    void AddComponent(Entity entity, const T& value);

    template <typename T>
    void RemoveComponent(Entity entity);

    template <typename T>
    void SetComponentEnabled(Entity entity, bool enabled);

    void Clear();
    [[nodiscard]] bool Empty() const { return m_Commands.empty(); }
    void Flush(World& world);

private:
    std::vector<StructuralCommand> m_Commands;
};

class ECS_API CommandBufferQueue {
public:
    explicit CommandBufferQueue(std::uint32_t threadCount = 32);

    [[nodiscard]] CommandBuffer& GetThreadLocal();
    [[nodiscard]] CommandBuffer& Get(std::uint32_t threadIndex);
    void FlushAll(World& world);
    void ClearAll();

private:
    std::vector<CommandBuffer> m_Buffers;
};

} // namespace we::runtime::ecs

#include "ECS/CommandBuffer.inl"

#pragma warning(pop)
