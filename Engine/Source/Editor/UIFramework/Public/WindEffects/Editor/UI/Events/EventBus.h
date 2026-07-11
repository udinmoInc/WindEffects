#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "WindEffects/Editor/UI/Events/IEventBus.h"

#include <mutex>
#include <unordered_map>
#include <vector>

namespace WindEffects::Editor::UI {

class EventBus final : public IEventBus {
public:
    void Subscribe(std::string_view eventType, EventHandler handler) override;
    void UnsubscribeAll(std::string_view eventType) override;
    void Publish(const EditorEvent& event) override;

private:
    mutable std::mutex m_Mutex;
    std::unordered_map<std::string, std::vector<EventHandler>> m_Handlers;
};

} // namespace WindEffects::Editor::UI
