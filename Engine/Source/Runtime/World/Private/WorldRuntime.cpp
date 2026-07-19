#include "World/IWorldRuntime.h"
#include "World/WorldReflection.h"
#include "WorldInternal.h"

#include "Reflection/ITypeRegistry.h"
#include "Reflection/Reflection.h"

namespace we::runtime::world {
namespace detail {

WorldRuntimeImpl::WorldRuntimeImpl(WorldRuntimeDependencies deps) : m_Deps(std::move(deps)) {
    if (m_Deps.registerReflectionTypes && m_Deps.typeRegistry) {
        RegisterWorldReflectionTypes(*m_Deps.typeRegistry);
    } else if (m_Deps.registerReflectionTypes) {
        RegisterWorldReflectionTypes(reflection::GetTypeRegistry());
    }
    m_Manager = std::make_unique<WorldManagerImpl>(m_Deps);
}

WorldRuntimeImpl::~WorldRuntimeImpl() {
    Shutdown();
}

void WorldRuntimeImpl::Tick(const WorldTickParams& params) {
    if (m_Manager) {
        m_Manager->TickAll(params);
    }
}

void WorldRuntimeImpl::Shutdown() {
    if (m_Manager) {
        m_Manager->Shutdown();
        m_Manager.reset();
    }
}

} // namespace detail

std::unique_ptr<IWorldRuntime> CreateWorldRuntime(WorldRuntimeDependencies deps) {
    return std::make_unique<detail::WorldRuntimeImpl>(std::move(deps));
}

} // namespace we::runtime::world
