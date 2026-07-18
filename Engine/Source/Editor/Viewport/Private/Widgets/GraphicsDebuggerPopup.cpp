#include "Widgets/GraphicsDebuggerPopup.h"
#include "EditorCamera.h"
#include "Scene/Scene.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Input/InputEvents.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"
#include "Core/Logger.h"
#include "Core/Math/Types.h"
#include <iomanip>
#include <sstream>

using ::we::runtime::kindui::ColorToken;
using ::we::runtime::kindui::MetricToken;
using ::we::runtime::kindui::PaddingToken;

namespace we::editor::viewport {
using ::we::runtime::kindui::Color;
using ::we::runtime::kindui::MouseButton;

namespace {
constexpr float kHeaderHeight = 22.0f;
constexpr float kPadding = 8.0f;
constexpr float kLineHeight = 13.0f;
constexpr float kPopupWidth = 360.0f;
constexpr const char* kTitle = "Graphics Debugger";
} // namespace

GraphicsDebuggerPopup::GraphicsDebuggerPopup(
    ::we::runtime::renderer::Renderer* renderer,
    const std::shared_ptr<::we::runtime::engine::EditorCamera>& camera,
    const std::shared_ptr<::we::runtime::scene::Scene>& scene)
    : m_Renderer(renderer), m_Camera(camera), m_Scene(scene) {}

void GraphicsDebuggerPopup::SetFrameStats(float fps, float frameTimeMs) {
    m_FPS = fps;
    m_FrameTimeMs = frameTimeMs;
}

Size GraphicsDebuggerPopup::Measure(const Size& /*availableSize*/) {
    std::vector<std::string> lines;
    BuildLines(lines);
    const float height = kHeaderHeight + kPadding * 2.0f + static_cast<float>(lines.size()) * kLineHeight;
    m_DesiredSize = Size{ kPopupWidth, height };
    return m_DesiredSize;
}

void GraphicsDebuggerPopup::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    m_HeaderRect = Rect{ allottedRect.x, allottedRect.y, allottedRect.width, kHeaderHeight };
}

void GraphicsDebuggerPopup::ArrangeInViewport(const Rect& viewportRect) {
    const Size size = Measure(Size{ kPopupWidth, 10000.0f });
    Arrange(Rect{
        viewportRect.x + m_Offset.x,
        viewportRect.y + m_Offset.y,
        size.width,
        size.height
    });
}

void GraphicsDebuggerPopup::BuildLines(std::vector<std::string>& outLines) const {
    uint32_t triangleCount = 0;
    uint32_t drawCallCount = 0;
    if (m_Scene) {
        for (const auto& entity : m_Scene->GetEntities()) {
            drawCallCount++;
            if (entity.Type == ::we::runtime::scene::EntityType::Plane
                || entity.Type == ::we::runtime::scene::EntityType::GroundPlane) {
                triangleCount += 2;
            } else {
                triangleCount += 12;
            }
        }
    }

    outLines.push_back("FPS: " + std::to_string(static_cast<int>(m_FPS))
        + " (" + std::to_string(m_FrameTimeMs).substr(0, 4) + " ms)");
    outLines.push_back("Triangles: " + std::to_string(triangleCount));
    outLines.push_back("Draw Calls: " + std::to_string(drawCallCount));
    outLines.push_back("Entities: " + std::to_string(m_Scene ? m_Scene->GetEntities().size() : 0));

    if (!m_Camera) {
        return;
    }

    auto formatVec3 = [](const we::math::Vec3& v) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(3) << v.x << ", " << v.y << ", " << v.z;
        return ss.str();
    };

    const we::math::Vec3 camPos = m_Camera->GetPosition();
    outLines.push_back("Cam Pos: " + formatVec3(camPos));
    outLines.push_back("Cam Rot: pitch " + std::to_string(m_Camera->GetPitch()).substr(0, 6)
        + " yaw " + std::to_string(m_Camera->GetYaw()).substr(0, 6));
    outLines.push_back("Camera Speed: " + std::to_string(static_cast<int>(std::lround(m_Camera->GetCameraSpeed()))));
}

void GraphicsDebuggerPopup::Paint(PaintContext& context) {
    if (!m_Visible) {
        return;
    }
    context.DrawShadow(m_Geometry, ThemeColor(ColorToken::ContentBrowserFolderShadow), 6.0f, 12.0f);
    context.DrawRoundedRect(m_Geometry, ThemeColor(ColorToken::PopupBackground), ThemeMetric(MetricToken::CornerRadiusSmall));
    context.DrawRoundedRectOutline(m_Geometry, ThemeColor(ColorToken::BorderDefault), 1.0f, ThemeMetric(MetricToken::CornerRadiusSmall));
    context.DrawRect(m_HeaderRect, ThemeColor(ColorToken::HeaderBackground));
    context.DrawText(kTitle, Point{ m_Geometry.x + kPadding, m_Geometry.y + 5.0f },
        ThemeColor(ColorToken::TextPrimary), 11.0f, true);

    std::vector<std::string> lines;
    BuildLines(lines);
    float lineY = m_Geometry.y + kHeaderHeight + kPadding;
    const Color textColor = ThemeColor(ColorToken::TextSecondary);
    const Color accentColor = ThemeColor(ColorToken::TextPrimary);
    for (size_t i = 0; i < lines.size(); ++i) {
        const Color color = (i >= lines.size() - 2) ? accentColor : textColor;
        context.DrawText(lines[i], Point{ m_Geometry.x + kPadding, lineY }, color, 11.0f);
        lineY += kLineHeight;
    }
}

void GraphicsDebuggerPopup::LogMoveIfChanged() {
    if (m_Offset.x == m_LastLoggedOffset.x && m_Offset.y == m_LastLoggedOffset.y) {
        return;
    }
    m_LastLoggedOffset = m_Offset;
    HE_INFO("[GraphicsDebugger] Popup moved to offset (" + std::to_string(static_cast<int>(m_Offset.x))
        + ", " + std::to_string(static_cast<int>(m_Offset.y)) + ")");
}

void GraphicsDebuggerPopup::OnMouseDown(const MouseEvent& event) {
    if (event.button != MouseButton::Left || !m_HeaderRect.Contains(event.position)) {
        return;
    }
    m_Dragging = true;
    m_DragOffset = Point{
        event.position.x - m_Geometry.x,
        event.position.y - m_Geometry.y
    };
}

void GraphicsDebuggerPopup::OnMouseMove(const MouseEvent& event) {
    if (!m_Dragging) {
        return;
    }

    const float newX = event.position.x - m_DragOffset.x;
    const float newY = event.position.y - m_DragOffset.y;
    if (auto parent = GetParent()) {
        const Rect& parentGeom = parent->GetGeometry();
        m_Offset.x = newX - parentGeom.x;
        m_Offset.y = newY - parentGeom.y;
        Arrange(Rect{ newX, newY, m_Geometry.width, m_Geometry.height });
        LogMoveIfChanged();
    }
}

void GraphicsDebuggerPopup::OnMouseUp(const MouseEvent& /*event*/) {
    if (m_Dragging) {
        LogMoveIfChanged();
    }
    m_Dragging = false;
}

bool GraphicsDebuggerPopup::ShowsPointerCursor(const Point& position) const {
    return m_HeaderRect.Contains(position);
}

} // namespace we::editor::viewport