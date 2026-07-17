#pragma once

#include "KindUI/Export.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::kindui {

struct ResourceId {
    std::string moduleId;
    std::string resourceName;

    [[nodiscard]] std::string ToString() const {
        return moduleId + ":" + resourceName;
    }

    auto operator<=>(const ResourceId&) const = default;
};

struct ResourceDescriptor {
    ResourceId id;
    std::string path;
    std::string type; // icon, font, texture, svg
};

using ResourceLoader = std::function<bool(const ResourceDescriptor& descriptor, void* outHandle)>;

class KINDUI_API IModuleResourceBundle {
public:
    virtual ~IModuleResourceBundle() = default;

    [[nodiscard]] virtual std::string_view GetModuleId() const = 0;
    virtual void RegisterResources(class IResourceRegistry& registry) = 0;
};

class KINDUI_API IResourceRegistry {
public:
    virtual ~IResourceRegistry() = default;

    virtual void RegisterBundle(std::shared_ptr<IModuleResourceBundle> bundle) = 0;
    virtual void UnregisterBundle(std::string_view moduleId) = 0;

    [[nodiscard]] virtual bool HasResource(const ResourceId& id) const = 0;
    [[nodiscard]] virtual std::string GetResourcePath(const ResourceId& id) const = 0;
    [[nodiscard]] virtual std::vector<ResourceDescriptor> GetModuleResources(std::string_view moduleId) const = 0;
};

} // namespace we::runtime::kindui
