#include "Core/Icon.h"
#include "Core/PaintContext.h"
#include <algorithm>

namespace WindEffects::Editor::UI {

IconRegistry& IconRegistry::Get() {
    static IconRegistry instance;
    return instance;
}

void IconRegistry::InitializeDefaultIcons() {
    // Lucide SVG icons are resolved at runtime from Assets/Icons/icons/.
    
    // Register custom editor icons
    RegisterIcon("save-all", "Assets/Editor/Ic_SaveAll.svg");
    RegisterIcon("search", "Assets/Editor/Ic_Search.svg");
    RegisterIcon("settings", "Assets/Editor/Ic_Settings.svg");
    RegisterIcon("save", "Assets/Editor/Ic_Save.svg");
}

void IconPainter::DrawIcon(PaintContext& context, const std::string& iconName,
                           const Point& position, float size, const Color& color) {
    context.DrawIcon(Icons::ResolveLucideName(iconName), position, color, size);
}

void IconPainter::DrawIcon(PaintContext& context, const std::string& iconName,
                           const Rect& bounds, const Color& color) {
    const float size = std::min(bounds.width, bounds.height);
    context.DrawIcon(Icons::ResolveLucideName(iconName), Point{ bounds.x, bounds.y }, color, size);
}

void IconPainter::DrawVerticalMoreMenu(PaintContext& context, const Rect& bounds, const Color& color) {
    DrawIcon(context, Icons::MoreName, bounds, color);
}

} // namespace WindEffects::Editor::UI
