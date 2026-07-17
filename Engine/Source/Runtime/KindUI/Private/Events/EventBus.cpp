#include "WindEffects/Runtime/UI/Events/EventBus.h"

namespace WindEffects::Editor::UI {

void EventBus::Subscribe(std::string_view eventType, EventHandler handler) {
    std::lock_guard lock(m_Mutex);
    m_Handlers[std::string(eventType)].push_back(std::move(handler));
}

void EventBus::UnsubscribeAll(std::string_view eventType) {
    std::lock_guard lock(m_Mutex);
    m_Handlers.erase(std::string(eventType));
}

void EventBus::Publish(const EditorEvent& event) {
    std::vector<EventHandler> handlers;
    {
        std::lock_guard lock(m_Mutex);
        const auto it = m_Handlers.find(event.type);
        if (it != m_Handlers.end()) {
            handlers = it->second;
        }
    }

    for (const auto& handler : handlers) {
        if (handler) {
            handler(event);
        }
    }
}

} // namespace WindEffects::Editor::UI

// export rebuild
