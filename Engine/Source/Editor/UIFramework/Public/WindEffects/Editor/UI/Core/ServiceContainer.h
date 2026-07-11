#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Core/IServiceProvider.h"

#include <mutex>
#include <unordered_map>

namespace WindEffects::Editor::UI {

class ServiceContainer : public IServiceProvider {
public:
    ServiceContainer() = default;

    std::shared_ptr<void> GetService(std::type_index type) const override;
    void RegisterService(std::type_index type, std::shared_ptr<void> service) override;

private:
    mutable std::mutex m_Mutex;
    std::unordered_map<std::type_index, std::shared_ptr<void>> m_Services;
};

} // namespace WindEffects::Editor::UI
