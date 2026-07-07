#pragma once

#include <typeindex>
#include <unordered_map>
#include <memory>
#include <stdexcept>

namespace we::core {

class ServiceLocator {
public:
    static ServiceLocator& Get() {
        static ServiceLocator instance;
        return instance;
    }

    template<typename T>
    void RegisterService(std::shared_ptr<T> service) {
        m_Services[std::type_index(typeid(T))] = service;
    }

    template<typename T>
    std::shared_ptr<T> GetService() {
        auto it = m_Services.find(std::type_index(typeid(T)));
        if (it != m_Services.end()) {
            return std::static_pointer_cast<T>(it->second);
        }
        return nullptr;
    }

    template<typename T>
    void RemoveService() {
        m_Services.erase(std::type_index(typeid(T)));
    }

private:
    ServiceLocator() = default;
    ~ServiceLocator() = default;

    std::unordered_map<std::type_index, std::shared_ptr<void>> m_Services;
};

} // namespace we::core
