#pragma once

#include "KindUI/Export.h"

#include <memory>
#include <typeindex>
#include <typeinfo>

namespace we::runtime::kindui {

class KINDUI_API IServiceProvider {
public:
    virtual ~IServiceProvider() = default;

    template<typename T>
    [[nodiscard]] std::shared_ptr<T> GetService() const {
        return std::static_pointer_cast<T>(GetService(typeid(T)));
    }

    template<typename T>
    void RegisterService(std::shared_ptr<T> service) {
        RegisterService(typeid(T), std::move(service));
    }

protected:
    virtual std::shared_ptr<void> GetService(std::type_index type) const = 0;
    virtual void RegisterService(std::type_index type, std::shared_ptr<void> service) = 0;
};

} // namespace we::runtime::kindui
