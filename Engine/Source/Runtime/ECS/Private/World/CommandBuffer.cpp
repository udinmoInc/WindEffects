#include "ECS/CommandBuffer.h"
#include "ECS/World.h"

#include <algorithm>

namespace we::runtime::ecs {

namespace {
thread_local std::uint32_t g_CommandBufferThreadIndex = 0;
} // namespace

CommandBufferQueue::CommandBufferQueue(std::uint32_t threadCount)
{
    m_Buffers.resize(std::max(1u, threadCount));
}

CommandBuffer& CommandBufferQueue::GetThreadLocal() {
    return m_Buffers[g_CommandBufferThreadIndex % static_cast<std::uint32_t>(m_Buffers.size())];
}

CommandBuffer& CommandBufferQueue::Get(std::uint32_t threadIndex) {
    return m_Buffers[threadIndex % static_cast<std::uint32_t>(m_Buffers.size())];
}

void CommandBufferQueue::FlushAll(World& world) {
    for (CommandBuffer& buffer : m_Buffers) {
        buffer.Flush(world);
    }
}

void CommandBufferQueue::ClearAll() {
    for (CommandBuffer& buffer : m_Buffers) {
        buffer.Clear();
    }
}

Entity CommandBuffer::CreateEntity(World& world) {
    StructuralCommand cmd{};
    cmd.type = StructuralChangeType::CreateEntity;
    m_Commands.push_back(cmd);
    return world.CreateEntity();
}

void CommandBuffer::DestroyEntity(Entity entity) {
    StructuralCommand cmd{};
    cmd.type = StructuralChangeType::DestroyEntity;
    cmd.entity = entity;
    m_Commands.push_back(cmd);
}

void CommandBuffer::SetEntityEnabled(Entity entity, bool enabled) {
    StructuralCommand cmd{};
    cmd.type = StructuralChangeType::SetEntityEnabled;
    cmd.entity = entity;
    cmd.boolValue = enabled;
    m_Commands.push_back(cmd);
}

void CommandBuffer::Clear() {
    m_Commands.clear();
}

void CommandBuffer::Flush(World& world) {
    for (const StructuralCommand& cmd : m_Commands) {
        switch (cmd.type) {
        case StructuralChangeType::CreateEntity:
            break; // immediate path records only
        case StructuralChangeType::DestroyEntity:
            world.DestroyEntity(cmd.entity);
            break;
        case StructuralChangeType::AddComponent:
            if (!cmd.payload.empty()) {
                world.MigrateAddComponent(cmd.entity, cmd.componentType, cmd.payload.data());
                world.BumpChangeVersion();
            }
            break;
        case StructuralChangeType::RemoveComponent:
            world.MigrateRemoveComponent(cmd.entity, cmd.componentType);
            world.BumpChangeVersion();
            break;
        case StructuralChangeType::SetEntityEnabled:
            world.SetEntityEnabled(cmd.entity, cmd.boolValue);
            break;
        case StructuralChangeType::SetComponentEnabled:
            if (const ComponentTypeInfo* info = ComponentTypeRegistry::Get().Find(cmd.componentType)) {
                (void)info;
            }
            break;
        default:
            break;
        }
    }
    m_Commands.clear();
}

} // namespace we::runtime::ecs
