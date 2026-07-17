#pragma once

#include "WindEffects/Runtime/UI/Export.h"
#include "WindEffects/Runtime/UI/Core/IServiceProvider.h"

#include <mutex>
#include <unordered_map>

namespace WindEffects::Editor::UI {

class UI_API ServiceContainer : public IServiceProvider {
public:
    ServiceContainer() = default;

    std::shared_ptr<void> GetService(std::type_index type) const override;
    void RegisterService(std::type_index type, std::shared_ptr<void> service) override;

private:
    mutable std::mutex m_Mutex;
    std::unordered_map<std::type_index, std::shared_ptr<void>> m_Services;
};

} // namespace WindEffects::Editor::UI
