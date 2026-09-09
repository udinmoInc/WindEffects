// ==============================================================================
// WindEffects — KindUI — ModuleResourceRegistry
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Resources/IResourceRegistry.h"

#include <mutex>
#include <unordered_map>

namespace we::runtime::kindui {

class KINDUI_API ModuleResourceRegistry final : public IResourceRegistry {
public:
    void RegisterBundle(std::shared_ptr<IModuleResourceBundle> bundle) override;
    void UnregisterBundle(std::string_view moduleId) override;

    [[nodiscard]] bool HasResource(const ResourceId& id) const override;
    [[nodiscard]] std::string GetResourcePath(const ResourceId& id) const override;
    [[nodiscard]] std::vector<ResourceDescriptor> GetModuleResources(std::string_view moduleId) const override;

    void RegisterDescriptor(const ResourceDescriptor& descriptor);

private:
    mutable std::mutex m_Mutex;
    std::unordered_map<std::string, std::shared_ptr<IModuleResourceBundle>> m_Bundles;
    std::unordered_map<std::string, ResourceDescriptor> m_Resources;
};

} // namespace we::runtime::kindui
