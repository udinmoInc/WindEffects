// ==============================================================================
// WindEffects — KindUI — ServiceContainer
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
