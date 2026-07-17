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
    void Publish(const EditorEvent& event) override;

private:
    mutable std::mutex m_Mutex;
    std::unordered_map<std::string, std::vector<EventHandler>> m_Handlers;
};

} // namespace we::runtime::kindui
