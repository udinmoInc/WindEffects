#include "ECS/ComponentOps.h"
#include "ECS/Types.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace we::runtime::ecs {
namespace {

std::vector<ComponentOps> g_Ops(kMaxComponentTypes);

void PodConstruct(void* dst, std::size_t size) { std::memset(dst, 0, size); }
void PodDestruct(void*, std::size_t) {}
void PodMove(void* dst, void* src, std::size_t size) { std::memmove(dst, src, size); }
void PodCopy(void* dst, const void* src, std::size_t size) { std::memcpy(dst, src, size); }

} // namespace

ComponentOps MakeTrivialOps() {
    ComponentOps ops{};
    ops.trivial = true;
    ops.construct = PodConstruct;
    ops.destruct = PodDestruct;
    ops.move = PodMove;
    ops.copy = PodCopy;
    return ops;
}

ComponentOps MakePODOps() {
    return MakeTrivialOps();
}

void RegisterComponentOps(std::uint32_t typeId, ComponentOps ops) {
    if (typeId >= g_Ops.size()) {
        g_Ops.resize(typeId + 1);
    }
    g_Ops[typeId] = ops;
}

const ComponentOps* FindComponentOps(std::uint32_t typeId) {
    if (typeId >= g_Ops.size()) {
        return nullptr;
    }
    return &g_Ops[typeId];
}

} // namespace we::runtime::ecs
