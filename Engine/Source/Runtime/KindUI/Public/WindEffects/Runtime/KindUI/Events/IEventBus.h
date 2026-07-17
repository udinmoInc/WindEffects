#pragma once

#include "WindEffects/Runtime/UI/Export.h"

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace WindEffects::Editor::UI {

struct EditorEvent {
    std::string type;
    std::string sourceId;
    std::string payload;
};

using EventHandler = std::function<void(const EditorEvent&)>;

class UI_API IEventBus {
public:
    virtual ~IEventBus() = default;

    virtual void Subscribe(std::string_view eventType, EventHandler handler) = 0;
    virtual void UnsubscribeAll(std::string_view eventType) = 0;
    virtual void Publish(const EditorEvent& event) = 0;
};

} // namespace WindEffects::Editor::UI
