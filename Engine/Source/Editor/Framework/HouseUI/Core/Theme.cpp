#include "Theme.hpp"
#include "Style.hpp"
namespace HouseEngine::UI {

Theme& Theme::Get() {
    static Theme instance;
    return instance;
}

StyleManager& StyleManager::Get() {
    static StyleManager instance;
    return instance;
}

} // namespace HouseEngine::UI
