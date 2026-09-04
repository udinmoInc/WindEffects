#include "KindUI/Theming/PaletteRuntime.h"
#include "KindUI/Theming/Palette.h"
#include "KindUI/Theming/ThemeManager.h"
#include "Core/Logger.h"
#include "Core/Paths.h"

#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#if WE_HAS_NLOHMANN_JSON
#include <nlohmann/json.h>
#endif

namespace we::runtime::kindui::palette {
namespace {

std::mutex g_Mutex;
GraphiteDarkColors g_Colors{};
GraphiteDarkMetrics g_Metrics{};
bool g_Initialized = false;
std::filesystem::file_time_type g_LastWriteTime{};
std::chrono::steady_clock::time_point g_LastPoll{};

Color ParseHexString(std::string_view text) {
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) {
        text.remove_prefix(1);
    }
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t')) {
        text.remove_suffix(1);
    }
    if (text.empty()) {
        return Color::Transparent();
    }
    size_t start = 0;
    if (text.front() == '#') {
        start = 1;
    }
    const size_t digits = text.size() - start;
    if (digits < 6) {
        return Color::Transparent();
    }
    const char* p = text.data() + start;
    const uint8_t r = detail::HexByte(p[0], p[1]);
    const uint8_t g = detail::HexByte(p[2], p[3]);
    const uint8_t b = detail::HexByte(p[4], p[5]);
    float a = 1.0f;
    if (digits >= 8) {
        a = detail::HexChannel(detail::HexByte(p[6], p[7]));
    }
    return {
        detail::HexChannel(r),
        detail::HexChannel(g),
        detail::HexChannel(b),
        a
    };
}

void ResetToCompileDefaults(GraphiteDarkColors& c) {
    using D = GraphiteDark;
    c.Black = D::Black;
    c.Title = D::Title;
    c.Background = D::Background;
    c.WindowBorder = D::WindowBorder;
    c.Foldout = D::Foldout;
    c.Input = D::Input;
    c.InputOutline = D::InputOutline;
    c.InputInsetInner = D::InputInsetInner;
    c.InputInsetOuter = D::InputInsetOuter;
    c.Recessed = D::Recessed;
    c.Panel = D::Panel;
    c.Header = D::Header;
    c.Dropdown = D::Dropdown;
    c.DropdownOutline = D::DropdownOutline;
    c.Hover = D::Hover;
    c.Hover2 = D::Hover2;
    c.Highlight = D::Highlight;
    c.Primary = D::Primary;
    c.PrimaryHover = D::PrimaryHover;
    c.PrimaryPress = D::PrimaryPress;
    c.Secondary = D::Secondary;
    c.Select = D::Select;
    c.SelectInactive = D::SelectInactive;
    c.SelectParent = D::SelectParent;
    c.SelectHover = D::SelectHover;
    c.White = D::White;
    c.White25 = D::White25;
    c.Foreground = D::Foreground;
    c.ForegroundHover = D::ForegroundHover;
    c.ForegroundInverted = D::ForegroundInverted;
    c.ForegroundHeader = D::ForegroundHeader;
    c.Notifications = D::Notifications;
    c.IconNormal = D::IconNormal;
    c.IconHoverTint = D::IconHoverTint;
    c.IconActiveTint = D::IconActiveTint;
    c.IconSubdued = D::IconSubdued;
    c.IconContactShadow = D::IconContactShadow;
    c.Warning = D::Warning;
    c.Error = D::Error;
    c.Success = D::Success;
    c.AccentBlue = D::AccentBlue;
    c.AccentPurple = D::AccentPurple;
    c.AccentPink = D::AccentPink;
    c.AccentRed = D::AccentRed;
    c.AccentOrange = D::AccentOrange;
    c.AccentYellow = D::AccentYellow;
    c.AccentGreen = D::AccentGreen;
    c.AccentBrown = D::AccentBrown;
    c.AccentBlack = D::AccentBlack;
    c.AccentGray = D::AccentGray;
    c.AccentWhite = D::AccentWhite;
    c.AccentFolder = D::AccentFolder;
    c.TooltipBg = D::TooltipBg;
    c.DragGhost = D::DragGhost;
    c.ActiveTabLine = D::ActiveTabLine;
    c.SelectionHighlight = D::SelectionHighlight;
    c.HighlightSubtle = D::HighlightSubtle;
    c.ModalScrim = D::ModalScrim;
    c.ShadowSubtle = D::ShadowSubtle;
    c.ShadowOverlay = D::ShadowOverlay;
    c.ShadowPopup = D::ShadowPopup;
    c.ShadowColor = D::ShadowColor;
    c.FolderShadow = D::FolderShadow;
    c.ButtonBevelTop = D::ButtonBevelTop;
    c.ButtonBevelBottom = D::ButtonBevelBottom;
    c.DebugGlyphBounds = D::DebugGlyphBounds;
}

void ResetMetricsToCompileDefaults(GraphiteDarkMetrics& m) {
    m.TabTopRadius = 6.0f;
}

float* MetricByName(GraphiteDarkMetrics& m, std::string_view name) {
    if (name == "TabTopRadius") return &m.TabTopRadius;
    return nullptr;
}

Color* ColorByName(GraphiteDarkColors& c, std::string_view name) {
    // Keep in sync with GraphiteDark.json keys.
    if (name == "Black") return &c.Black;
    if (name == "Title") return &c.Title;
    if (name == "Background") return &c.Background;
    if (name == "WindowBorder") return &c.WindowBorder;
    if (name == "Foldout") return &c.Foldout;
    if (name == "Input") return &c.Input;
    if (name == "InputOutline") return &c.InputOutline;
    if (name == "InputInsetInner") return &c.InputInsetInner;
    if (name == "InputInsetOuter") return &c.InputInsetOuter;
    if (name == "Recessed") return &c.Recessed;
    if (name == "Panel") return &c.Panel;
    if (name == "Header") return &c.Header;
    if (name == "Dropdown") return &c.Dropdown;
    if (name == "DropdownOutline") return &c.DropdownOutline;
    if (name == "Hover") return &c.Hover;
    if (name == "Hover2") return &c.Hover2;
    if (name == "Highlight") return &c.Highlight;
    if (name == "Primary") return &c.Primary;
    if (name == "PrimaryHover") return &c.PrimaryHover;
    if (name == "PrimaryPress") return &c.PrimaryPress;
    if (name == "Secondary") return &c.Secondary;
    if (name == "Select") return &c.Select;
    if (name == "SelectInactive") return &c.SelectInactive;
    if (name == "SelectParent") return &c.SelectParent;
    if (name == "SelectHover") return &c.SelectHover;
    if (name == "White") return &c.White;
    if (name == "White25") return &c.White25;
    if (name == "Foreground") return &c.Foreground;
    if (name == "ForegroundHover") return &c.ForegroundHover;
    if (name == "ForegroundInverted") return &c.ForegroundInverted;
    if (name == "ForegroundHeader") return &c.ForegroundHeader;
    if (name == "Notifications") return &c.Notifications;
    if (name == "IconNormal") return &c.IconNormal;
    if (name == "IconHoverTint") return &c.IconHoverTint;
    if (name == "IconActiveTint") return &c.IconActiveTint;
    if (name == "IconSubdued") return &c.IconSubdued;
    if (name == "IconContactShadow") return &c.IconContactShadow;
    if (name == "Warning") return &c.Warning;
    if (name == "Error") return &c.Error;
    if (name == "Success") return &c.Success;
    if (name == "AccentBlue") return &c.AccentBlue;
    if (name == "AccentPurple") return &c.AccentPurple;
    if (name == "AccentPink") return &c.AccentPink;
    if (name == "AccentRed") return &c.AccentRed;
    if (name == "AccentOrange") return &c.AccentOrange;
    if (name == "AccentYellow") return &c.AccentYellow;
    if (name == "AccentGreen") return &c.AccentGreen;
    if (name == "AccentBrown") return &c.AccentBrown;
    if (name == "AccentBlack") return &c.AccentBlack;
    if (name == "AccentGray") return &c.AccentGray;
    if (name == "AccentWhite") return &c.AccentWhite;
    if (name == "AccentFolder") return &c.AccentFolder;
    if (name == "TooltipBg") return &c.TooltipBg;
    if (name == "DragGhost") return &c.DragGhost;
    if (name == "ActiveTabLine") return &c.ActiveTabLine;
    if (name == "SelectionHighlight") return &c.SelectionHighlight;
    if (name == "HighlightSubtle") return &c.HighlightSubtle;
    if (name == "ModalScrim") return &c.ModalScrim;
    if (name == "ShadowSubtle") return &c.ShadowSubtle;
    if (name == "ShadowOverlay") return &c.ShadowOverlay;
    if (name == "ShadowPopup") return &c.ShadowPopup;
    if (name == "ShadowColor") return &c.ShadowColor;
    if (name == "FolderShadow") return &c.FolderShadow;
    if (name == "ButtonBevelTop") return &c.ButtonBevelTop;
    if (name == "ButtonBevelBottom") return &c.ButtonBevelBottom;
    if (name == "DebugGlyphBounds") return &c.DebugGlyphBounds;
    return nullptr;
}

void ApplyAliasFallbacks(GraphiteDarkColors& c) {
    // Match Palette.h aliases when JSON omits them.
    if (c.Secondary.a <= 0.0f) {
        c.Secondary = c.Dropdown;
    }
    if (c.Select.a <= 0.0f) {
        c.Select = c.Primary;
    }
    if (c.SelectHover.a <= 0.0f) {
        c.SelectHover = c.Panel;
    }
    if (c.ForegroundHover.a <= 0.0f) {
        c.ForegroundHover = c.White;
    }
    if (c.ForegroundInverted.a <= 0.0f) {
        c.ForegroundInverted = c.Input;
    }
    if (c.IconActiveTint.a <= 0.0f) {
        c.IconActiveTint = c.White;
    }
    if (c.AccentWhite.a <= 0.0f) {
        c.AccentWhite = c.White;
    }
    if (c.HighlightSubtle.a <= 0.0f) {
        c.HighlightSubtle = c.White25;
    }
    if (c.InputInsetInner.a <= 0.0f) {
        c.InputInsetInner = GraphiteDark::InputInsetInner;
    }
    if (c.InputInsetOuter.a <= 0.0f) {
        c.InputInsetOuter = GraphiteDark::InputInsetOuter;
    }
    if (c.IconContactShadow.a <= 0.0f) {
        c.IconContactShadow = GraphiteDark::IconContactShadow;
    }
}

std::filesystem::path ResolveConfigPath() {
    auto& paths = we::core::PathService::Get();
    std::vector<std::filesystem::path> candidates;
    candidates.push_back(paths.EngineConfigRoot() / "Themes" / "GraphiteDark.json");
    candidates.push_back(paths.ConfigRoot() / "Themes" / "GraphiteDark.json");
    if (const auto repo = we::core::PathService::FindRepositoryRoot(paths.ExecutableDirectory())) {
        candidates.push_back(*repo / "Engine" / "Config" / "Themes" / "GraphiteDark.json");
    }

    // Prefer the newest existing copy so editing the repo source file hot-reloads
    // without waiting for a rebuild/restage.
    std::error_code ec;
    std::filesystem::path best;
    std::filesystem::file_time_type bestTime{};
    for (const auto& candidate : candidates) {
        if (!std::filesystem::exists(candidate, ec)) {
            continue;
        }
        const auto writeTime = std::filesystem::last_write_time(candidate, ec);
        if (ec) {
            continue;
        }
        if (best.empty() || writeTime > bestTime) {
            best = candidate;
            bestTime = writeTime;
        }
    }
    return best.empty() ? candidates.front() : best;
}

bool LoadFromFile(const std::filesystem::path& path, GraphiteDarkColors& outColors, GraphiteDarkMetrics& outMetrics) {
    std::ifstream input(path);
    if (!input) {
        return false;
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    const std::string text = buffer.str();
    if (text.empty()) {
        return false;
    }

    GraphiteDarkColors nextColors = outColors;
    GraphiteDarkMetrics nextMetrics = outMetrics;
    int applied = 0;

#if WE_HAS_NLOHMANN_JSON
    try {
        const nlohmann::json root = nlohmann::json::parse(text, nullptr, true, true);
        if (!root.is_object()) {
            return false;
        }
        for (auto it = root.begin(); it != root.end(); ++it) {
            if (it.key() == "Metrics" && it.value().is_object()) {
                for (auto mit = it.value().begin(); mit != it.value().end(); ++mit) {
                    float* slot = MetricByName(nextMetrics, mit.key());
                    if (!slot || !mit.value().is_number()) {
                        continue;
                    }
                    *slot = mit.value().get<float>();
                    ++applied;
                }
                continue;
            }
            if (!it.value().is_string()) {
                continue;
            }
            Color* slot = ColorByName(nextColors, it.key());
            if (!slot) {
                continue;
            }
            const Color parsed = ParseHexString(it.value().get<std::string>());
            if (parsed.a <= 0.0f && parsed.r == 0.0f && parsed.g == 0.0f && parsed.b == 0.0f) {
                continue;
            }
            *slot = parsed;
            ++applied;
        }
    } catch (const std::exception& ex) {
        HE_WARN(std::string("[Palette] Failed to parse GraphiteDark.json: ") + ex.what());
        return false;
    }
#else
    // Minimal fallback: scan "Key": "#RRGGBB" pairs (colors only).
    size_t pos = 0;
    while (pos < text.size()) {
        const size_t keyStart = text.find('"', pos);
        if (keyStart == std::string::npos) {
            break;
        }
        const size_t keyEnd = text.find('"', keyStart + 1);
        if (keyEnd == std::string::npos) {
            break;
        }
        const std::string key = text.substr(keyStart + 1, keyEnd - keyStart - 1);
        const size_t colon = text.find(':', keyEnd + 1);
        if (colon == std::string::npos) {
            break;
        }
        const size_t valStart = text.find('"', colon + 1);
        if (valStart == std::string::npos) {
            break;
        }
        const size_t valEnd = text.find('"', valStart + 1);
        if (valEnd == std::string::npos) {
            break;
        }
        const std::string value = text.substr(valStart + 1, valEnd - valStart - 1);
        if (Color* slot = ColorByName(nextColors, key)) {
            *slot = ParseHexString(value);
            ++applied;
        }
        pos = valEnd + 1;
    }
#endif

    if (applied == 0) {
        return false;
    }
    ApplyAliasFallbacks(nextColors);
    outColors = nextColors;
    outMetrics = nextMetrics;
    HE_INFO("[Palette] Loaded " + std::to_string(applied) + " theme values from " + we::core::PathService::ToUtf8(path));
    return true;
}

bool EnsureLoadedLocked() {
    if (g_Initialized) {
        return false;
    }
    ResetToCompileDefaults(g_Colors);
    ResetMetricsToCompileDefaults(g_Metrics);
    const auto path = ResolveConfigPath();
    std::error_code ec;
    if (std::filesystem::exists(path, ec)) {
        LoadFromFile(path, g_Colors, g_Metrics);
        g_LastWriteTime = std::filesystem::last_write_time(path, ec);
    } else {
        HE_WARN("[Palette] GraphiteDark.json not found — using compile defaults: " +
                we::core::PathService::ToUtf8(path));
    }
    g_Initialized = true;
    g_LastPoll = std::chrono::steady_clock::now();
    return true;
}

} // namespace

GraphiteDarkColors& GraphiteDarkLive() {
    std::lock_guard lock(g_Mutex);
    EnsureLoadedLocked();
    return g_Colors;
}

GraphiteDarkMetrics& GraphiteDarkLiveMetrics() {
    std::lock_guard lock(g_Mutex);
    EnsureLoadedLocked();
    return g_Metrics;
}

bool ReloadGraphiteDarkPaletteIfChanged() {
    bool reloaded = false;
    {
        std::lock_guard lock(g_Mutex);
        EnsureLoadedLocked();

        const auto now = std::chrono::steady_clock::now();
        // Fast poll so Metrics/TabTopRadius edits show up immediately while editing JSON.
        if (now - g_LastPoll < std::chrono::milliseconds(50)) {
            return false;
        }
        g_LastPoll = now;

        const auto path = ResolveConfigPath();
        std::error_code ec;
        if (!std::filesystem::exists(path, ec)) {
            return false;
        }
        const auto writeTime = std::filesystem::last_write_time(path, ec);
        if (ec || writeTime == g_LastWriteTime) {
            return false;
        }

        GraphiteDarkColors nextColors{};
        GraphiteDarkMetrics nextMetrics{};
        ResetToCompileDefaults(nextColors);
        ResetMetricsToCompileDefaults(nextMetrics);
        if (!LoadFromFile(path, nextColors, nextMetrics)) {
            return false;
        }
        g_Colors = nextColors;
        g_Metrics = nextMetrics;
        g_LastWriteTime = writeTime;
        reloaded = true;
    }
    if (reloaded) {
        ThemeManager::Get().NotifyChanged();
    }
    return reloaded;
}

} // namespace we::runtime::kindui::palette
