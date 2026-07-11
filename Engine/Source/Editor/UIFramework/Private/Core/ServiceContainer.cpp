#include "WindEffects/Editor/UI/Core/ServiceContainer.h"

namespace WindEffects::Editor::UI {

std::shared_ptr<void> ServiceContainer::GetService(std::type_index type) const {
    std::lock_guard lock(m_Mutex);
    const auto it = m_Services.find(type);
    return it != m_Services.end() ? it->second : nullptr;
}

void ServiceContainer::RegisterService(std::type_index type, std::shared_ptr<void> service) {
    std::lock_guard lock(m_Mutex);
    m_Services[type] = std::move(service);
}

} // namespace WindEffects::Editor::UI
