#include "KindUI/App/NotificationService.h"

#include <algorithm>

namespace we::runtime::kindui {

void NotificationService::Push(Notification notification) {
    if (notification.id.empty()) {
        notification.id = "toast-" + std::to_string(m_Active.Items().size());
    }
    auto items = m_Active.Items();
    items.push_back(std::move(notification));
    m_Remaining.push_back(items.back().durationSeconds);
    m_Active.Set(std::move(items));
}

void NotificationService::Dismiss(const std::string& id) {
    auto items = m_Active.Items();
    std::vector<float> remaining;
    remaining.reserve(items.size());
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (items[i].id != id) {
            remaining.push_back(i < m_Remaining.size() ? m_Remaining[i] : 0.0f);
        }
    }
    items.erase(
        std::remove_if(items.begin(), items.end(), [&](const Notification& n) { return n.id == id; }),
        items.end());
    m_Remaining = std::move(remaining);
    m_Active.Set(std::move(items));
}

void NotificationService::Clear() {
    m_Remaining.clear();
    m_Active.Clear();
}

void NotificationService::Info(std::string message, std::string title) {
    Push({ {}, std::move(title), std::move(message), NotificationKind::Info });
}

void NotificationService::Success(std::string message, std::string title) {
    Push({ {}, std::move(title), std::move(message), NotificationKind::Success });
}

void NotificationService::Warning(std::string message, std::string title) {
    Push({ {}, std::move(title), std::move(message), NotificationKind::Warning });
}

void NotificationService::Error(std::string message, std::string title) {
    Push({ {}, std::move(title), std::move(message), NotificationKind::Error, 6.0f });
}

void NotificationService::Progress(std::string message, float progress, std::string title) {
    Notification n;
    n.title = std::move(title);
    n.message = std::move(message);
    n.kind = NotificationKind::Progress;
    n.progress = progress;
    n.durationSeconds = 0.0f;
    Push(std::move(n));
}

void NotificationService::Tick(float deltaTime) {
    if (m_Active.Items().empty()) {
        return;
    }
    auto items = m_Active.Items();
    bool changed = false;
    for (std::size_t i = 0; i < items.size();) {
        if (items[i].kind == NotificationKind::Progress && items[i].durationSeconds <= 0.0f) {
            ++i;
            continue;
        }
        if (i >= m_Remaining.size()) {
            ++i;
            continue;
        }
        m_Remaining[i] -= deltaTime;
        if (m_Remaining[i] <= 0.0f) {
            items.erase(items.begin() + static_cast<std::ptrdiff_t>(i));
            m_Remaining.erase(m_Remaining.begin() + static_cast<std::ptrdiff_t>(i));
            changed = true;
        } else {
            ++i;
        }
    }
    if (changed) {
        m_Active.Set(std::move(items));
    }
}

} // namespace we::runtime::kindui
