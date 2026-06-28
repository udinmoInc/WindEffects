#include "Theme.hpp"
#include "Style.hpp"
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
