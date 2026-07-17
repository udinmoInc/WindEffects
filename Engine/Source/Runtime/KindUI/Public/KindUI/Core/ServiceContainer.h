#pragma once

#include "KindUI/Export.h"
#include "KindUI/Core/IServiceProvider.h"

#include <mutex>
#include <unordered_map>

namespace we::runtime::kindui {

class KINDUI_API ServiceContainer : public IServiceProvider {
public:
    ServiceContainer() = default;

    std::shared_ptr<void> GetService(std::type_index type) const override;
    void RegisterService(std::type_index type, std::shared_ptr<void> service) override;

private:
    mutable std::mutex m_Mutex;
    std::unordered_map<std::type_index, std::shared_ptr<void>> m_Services;
};

} // namespace we::runtime::kindui
