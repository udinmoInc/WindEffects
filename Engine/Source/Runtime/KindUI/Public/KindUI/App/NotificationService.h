#pragma once

#include "KindUI/Core/Observable.h"
#include "KindUI/Export.h"

#include <chrono>
#include <string>
#include <vector>

namespace we::runtime::kindui {

enum class NotificationKind {
    Info,
    Success,
    Warning,
    Error,
    Progress,
};

struct Notification {
    std::string id;
    std::string title;
    std::string message;
    NotificationKind kind = NotificationKind::Info;
    float durationSeconds = 4.0f;
    float progress = -1.0f;
};

/// Centralized toast/notification queue.
class KINDUI_API NotificationService {
public:
    void Push(Notification notification);
    void Dismiss(const std::string& id);
    void Clear();

    void Info(std::string message, std::string title = {});
    void Success(std::string message, std::string title = {});
    void Warning(std::string message, std::string title = {});
    void Error(std::string message, std::string title = {});
    void Progress(std::string message, float progress, std::string title = {});

    [[nodiscard]] const ObservableList<Notification>& Active() const { return m_Active; }
    ObservableList<Notification>& Active() { return m_Active; }

    void Tick(float deltaTime);

private:
    ObservableList<Notification> m_Active;
    std::vector<float> m_Remaining;
};

} // namespace we::runtime::kindui
