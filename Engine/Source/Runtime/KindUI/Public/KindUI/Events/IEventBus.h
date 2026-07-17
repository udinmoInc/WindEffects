#pragma once

#include "KindUI/Export.h"

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::kindui {

struct EditorEvent {
    std::string type;
    std::string sourceId;
    std::string payload;
};

using EventHandler = std::function<void(const EditorEvent&)>;

class KINDUI_API IEventBus {
public:
    virtual ~IEventBus() = default;

    virtual void Subscribe(std::string_view eventType, EventHandler handler) = 0;
    virtual void UnsubscribeAll(std::string_view eventType) = 0;
    virtual void Publish(const EditorEvent& event) = 0;
};

} // namespace we::runtime::kindui
