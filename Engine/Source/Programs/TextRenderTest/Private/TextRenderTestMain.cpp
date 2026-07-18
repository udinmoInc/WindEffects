#include "Text/Assets/FontAsset.h"
#include "Text/TextEngine.h"

#include "Core/BuildPaths.h"
#include "Core/Paths.h"

#include <iostream>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
struct TextRenderTestBootstrap {
    TextRenderTestBootstrap() { we::core::ConfigureModuleSearchPaths(); }
};
#pragma init_seg(lib)
TextRenderTestBootstrap g_Bootstrap;
#endif

int main()
{
    using namespace we::runtime::text;

    auto engine = CreateTextEngine();
    const auto fontPath = we::core::PathService::FindExisting(
        we::core::PathService::Get().FontCandidates("Inter-Regular.wefont"));
    if (!fontPath) {
        std::cerr << "Font not found\n";
        return 1;
    }
    const auto loaded = engine->LoadFont(*fontPath);
    if (!loaded.ok) {
        std::cerr << "LoadFont failed\n";
        return 1;
    }

    layout::TextStyle style;
    style.sizePx = 13.0f;
    layout::LayoutConstraints constraints;
    constraints.dpiScale = 1.0f;

    const auto layout = engine->Layout("Hello", style, constraints, loaded.value);
    std::cout << "glyphs=" << layout.glyphs.size() << " width=" << layout.bounds.width << '\n';
    for (const auto& g : layout.glyphs) {
        if (!g.glyph.metrics.hasDrawableQuad) {
            continue;
        }
        std::cout << " cp=" << static_cast<char>(g.glyph.metrics.codepoint)
                  << " x=" << g.x << " y=" << g.y
                  << " w=" << g.width << " h=" << g.height << '\n';
    }
    return 0;
}
