#include "Core/Theme.h"
namespace we::UI {

Theme& Theme::Get() {
    static Theme instance;
    return instance;
}

} // namespace we::editor::application::UI
