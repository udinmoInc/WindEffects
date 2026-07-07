#include "Core/Theme.h"
#include "Core/Style.h"
namespace we::UI {

Theme& Theme::Get() {
    static Theme instance;
    return instance;
}

StyleManager& StyleManager::Get() {
    static StyleManager instance;
    return instance;
}

} // namespace we::editor::application::UI
