// ==============================================================================
// WindEffects — KindUI — EventBus
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "KindUI/Events/IEventBus.h"

#include <mutex>
#include <unordered_map>
#include <vector>

namespace we::runtime::kindui {

class KINDUI_API EventBus final : public IEventBus {
public:
    void Subscribe(std::string_view eventType, EventHandler handler) override;
    void UnsubscribeAll(std::string_view eventType) override;
    void Publish(const UIEvent& event) override;

private:
    mutable std::mutex m_Mutex;
    std::unordered_map<std::string, std::vector<EventHandler>> m_Handlers;
};

} // namespace we::runtime::kindui
