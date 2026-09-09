// ==============================================================================
// WindEffects — KindUI — IEventBus
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::kindui {

struct UIEvent {
    std::string type;
    std::string sourceId;
    std::string payload;
};

using EventHandler = std::function<void(const UIEvent&)>;

class KINDUI_API IEventBus {
public:
    virtual ~IEventBus() = default;

    virtual void Subscribe(std::string_view eventType, EventHandler handler) = 0;
    virtual void UnsubscribeAll(std::string_view eventType) = 0;
    virtual void Publish(const UIEvent& event) = 0;
};

} // namespace we::runtime::kindui
