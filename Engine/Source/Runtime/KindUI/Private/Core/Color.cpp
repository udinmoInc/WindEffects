#include "KindUI/Core/Types.h"
#include "KindUI/Theming/PaletteRuntime.h"

namespace we::runtime::kindui {

Color Color::White() {
    return palette::GraphiteDarkLive().White;
}

Color Color::Black() {
    return palette::GraphiteDarkLive().Black;
}

} // namespace we::runtime::kindui
