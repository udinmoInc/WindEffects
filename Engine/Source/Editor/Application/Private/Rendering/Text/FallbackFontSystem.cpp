#include "Rendering/Text/FallbackFontSystem.h"
#include "Core/Logger.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <filesystem>

namespace we::UI::Text {

// ============================================================================
// FallbackFontSystem Implementation
// ============================================================================

FallbackFontSystem::FallbackFontSystem() = default;

FallbackFontSystem::~FallbackFontSystem() {
    Shutdown();
}

bool FallbackFontSystem::Initialize() {
    if (m_Initialized) {
        HE_WARN("FallbackFontSystem: Already initialized");
        return true;
    }

    m_FallbackFonts.clear();
    m_SortedFallbacks.clear();
    m_ReplacementCharacter = 0xFFFD;

    // Load default system fallback fonts
    LoadDefaultFallbacks();

    m_Initialized = true;
    HE_INFO("FallbackFontSystem: Initialized with " + std::to_string(m_FallbackFonts.size()) + " fallback fonts");
    return true;
}

void FallbackFontSystem::Shutdown() {
    if (!m_Initialized) {
        return;
    }

    // Unload all fallback faces
    for (auto& [name, data] : m_FallbackFonts) {
        UnloadFallbackFace(name);
    }

    m_FallbackFonts.clear();
    m_SortedFallbacks.clear();
    m_FontLoader = nullptr;
    m_GlyphManager = nullptr;
    m_Initialized = false;

    HE_INFO("FallbackFontSystem: Shutdown");
}

bool FallbackFontSystem::AddFallbackFont(const FallbackFontDescriptor& descriptor) {
    if (!m_Initialized) {
        return false;
    }

    // Check if already exists
    if (m_FallbackFonts.find(descriptor.fontName) != m_FallbackFonts.end()) {
        HE_WARN("FallbackFontSystem: Fallback font already exists: " + descriptor.fontName);
        return false;
    }

    FallbackFontData data;
    data.descriptor = descriptor;
    data.face = LoadFallbackFace(descriptor);

    if (!data.face) {
        HE_WARN("FallbackFontSystem: Failed to load fallback font: " + descriptor.fontName);
        return false;
    }

    m_FallbackFonts[descriptor.fontName] = data;
    SortFallbacks();

    HE_INFO("FallbackFontSystem: Added fallback font '" + descriptor.fontName + "' with priority " + 
            std::to_string(static_cast<int>(descriptor.priority)));
    return true;
}

bool FallbackFontSystem::RemoveFallbackFont(const std::string& fontName) {
    auto it = m_FallbackFonts.find(fontName);
    if (it == m_FallbackFonts.end()) {
        return false;
    }

    UnloadFallbackFace(fontName);
    m_FallbackFonts.erase(it);
    SortFallbacks();

    HE_INFO("FallbackFontSystem: Removed fallback font '" + fontName + "'");
    return true;
}

void FallbackFontSystem::SetFontLoader(FontLoader* fontLoader) {
    m_FontLoader = fontLoader;
}

void FallbackFontSystem::SetGlyphManager(GlyphManager* glyphManager) {
    m_GlyphManager = glyphManager;
}

FallbackResult FallbackFontSystem::FindFallback(uint32_t codepoint, FT_FaceRec_* primaryFace) {
    FallbackResult result;
    result.success = false;

    if (!m_Initialized || m_SortedFallbacks.empty()) {
        // Use replacement character as last resort
        result.usedReplacement = true;
        result.fallbackCodepoint = m_ReplacementCharacter;
        return result;
    }

    // Check if primary font supports the codepoint
    if (primaryFace && FontSupportsCodepoint(primaryFace, codepoint)) {
        return result;  // Not a fallback case
    }

    // Try fallback fonts in priority order
    ScriptType script = DetectScriptForCodepoint(codepoint);

    for (const auto& fontName : m_SortedFallbacks) {
        auto it = m_FallbackFonts.find(fontName);
        if (it == m_FallbackFonts.end()) {
            continue;
        }

        FallbackFontData& data = it->second;

        // Check if font supports the script
        if (!data.descriptor.supportedScripts.empty()) {
            bool scriptSupported = false;
            for (uint32_t supportedScript : data.descriptor.supportedScripts) {
                if (static_cast<ScriptType>(supportedScript) == script) {
                    scriptSupported = true;
                    break;
                }
            }
            if (!scriptSupported) {
                continue;
            }
        }

        // Check if font has the glyph
        if (data.face && FontSupportsCodepoint(data.face, codepoint)) {
            result.success = true;
            result.fallbackCodepoint = codepoint;
            result.fallbackFontName = fontName;
            result.fallbackFace = data.face;
            data.usageCount++;
            return result;
        }
    }

    // No fallback found, use replacement character
    result.usedReplacement = true;
    result.fallbackCodepoint = m_ReplacementCharacter;

    HE_DEBUG("FallbackFontSystem: No fallback found for U+" + std::to_string(codepoint) + 
             ", using replacement character");

    return result;
}

FT_FaceRec_* FallbackFontSystem::LoadFallbackFace(const FallbackFontDescriptor& descriptor) {
    if (!m_FontLoader) {
        return nullptr;
    }

    FontLoaderConfig loaderConfig;
    FontLoadResult result = m_FontLoader->LoadFont(descriptor.fontPath, descriptor.faceIndex, loaderConfig);

    if (!result.success) {
        return nullptr;
    }

    return result.face;
}

void FallbackFontSystem::UnloadFallbackFace(const std::string& fontName) {
    auto it = m_FallbackFonts.find(fontName);
    if (it == m_FallbackFonts.end()) {
        return;
    }

    if (it->second.face && m_FontLoader) {
        m_FontLoader->UnloadFont(it->second.face);
    }

    it->second.face = nullptr;
}

bool FallbackFontSystem::FontSupportsCodepoint(FT_FaceRec_* face, uint32_t codepoint) const {
    if (!face) {
        return false;
    }

    FT_UInt glyphIndex = FT_Get_Char_Index(face, codepoint);
    return glyphIndex != 0;
}

ScriptType FallbackFontSystem::DetectScriptForCodepoint(uint32_t codepoint) const {
    return TextShaperUtils::GetScriptForCodepoint(codepoint);
}

void FallbackFontSystem::SortFallbacks() {
    m_SortedFallbacks.clear();
    m_SortedFallbacks.reserve(m_FallbackFonts.size());

    // Sort by priority (Critical first, Low last)
    for (const auto& [name, data] : m_FallbackFonts) {
        m_SortedFallbacks.push_back(name);
    }

    std::sort(m_SortedFallbacks.begin(), m_SortedFallbacks.end(),
              [this](const std::string& a, const std::string& b) {
                  auto itA = m_FallbackFonts.find(a);
                  auto itB = m_FallbackFonts.find(b);
                  if (itA == m_FallbackFonts.end() || itB == m_FallbackFonts.end()) {
                      return false;
                  }
                  return static_cast<int>(itA->second.descriptor.priority) < 
                         static_cast<int>(itB->second.descriptor.priority);
              });
}

void FallbackFontSystem::LoadDefaultFallbacks() {
    // Try to load common system fonts as fallbacks
    std::vector<std::string> commonFallbacks = {
        "Arial.ttf",
        "Arial Unicode MS.ttf",
        "Segoe UI.ttf",
        "Roboto-Regular.ttf",
        "NotoSans-Regular.ttf",
        "DejaVuSans.ttf",
        "LiberationSans-Regular.ttf",
        "Symbola.ttf"  // Good for Unicode symbols
    };

    for (const auto& fontName : commonFallbacks) {
        std::string fontPath = FontLoaderUtils::FindSystemFont(fontName);
        if (!fontPath.empty()) {
            FallbackFontDescriptor descriptor;
            descriptor.fontPath = fontPath;
            descriptor.fontName = fontName;
            descriptor.priority = FallbackPriority::Medium;
            
            // Set higher priority for Unicode fonts
            if (fontName.find("Unicode") != std::string::npos || 
                fontName.find("Symbola") != std::string::npos) {
                descriptor.priority = FallbackPriority::High;
            }

            AddFallbackFont(descriptor);
        }
    }
}

std::vector<FallbackFontDescriptor> FallbackFontSystem::GetFallbackFonts() const {
    std::vector<FallbackFontDescriptor> descriptors;
    descriptors.reserve(m_FallbackFonts.size());

    for (const auto& [name, data] : m_FallbackFonts) {
        descriptors.push_back(data.descriptor);
    }

    return descriptors;
}

FallbackFontDescriptor FallbackFontSystem::GetFallbackFont(const std::string& fontName) const {
    auto it = m_FallbackFonts.find(fontName);
    if (it != m_FallbackFonts.end()) {
        return it->second.descriptor;
    }
    return {};
}

bool FallbackFontSystem::IsSupportedByFallback(uint32_t codepoint) const {
    for (const auto& [name, data] : m_FallbackFonts) {
        if (data.face && FontSupportsCodepoint(data.face, codepoint)) {
            return true;
        }
    }
    return false;
}

std::unordered_map<std::string, size_t> FallbackFontSystem::GetStatistics() const {
    std::unordered_map<std::string, size_t> stats;

    for (const auto& [name, data] : m_FallbackFonts) {
        stats[name] = data.usageCount;
    }

    return stats;
}

void FallbackFontSystem::ResetStatistics() {
    for (auto& [name, data] : m_FallbackFonts) {
        data.usageCount = 0;
    }
}

// ============================================================================
// FallbackFontSystemUtils Implementation
// ============================================================================

namespace FallbackFontSystemUtils {

std::vector<FallbackFontDescriptor> GetDefaultFallbacks() {
    std::vector<FallbackFontDescriptor> fallbacks;

    // Common fallback fonts for different scripts
    std::vector<std::pair<std::string, FallbackPriority>> fonts = {
        {"Arial Unicode MS.ttf", FallbackPriority::High},
        {"Segoe UI.ttf", FallbackPriority::Medium},
        {"Roboto-Regular.ttf", FallbackPriority::Medium},
        {"NotoSans-Regular.ttf", FallbackPriority::Medium},
        {"NotoSansCJK-Regular.ttc", FallbackPriority::High},
        {"NotoSansArabic-Regular.ttf", FallbackPriority::High},
        {"DejaVuSans.ttf", FallbackPriority::Medium},
        {"Symbola.ttf", FallbackPriority::High}
    };

    for (const auto& [fontName, priority] : fonts) {
        std::string fontPath = FontLoaderUtils::FindSystemFont(fontName);
        if (!fontPath.empty()) {
            FallbackFontDescriptor descriptor;
            descriptor.fontPath = fontPath;
            descriptor.fontName = fontName;
            descriptor.priority = priority;
            fallbacks.push_back(descriptor);
        }
    }

    return fallbacks;
}

FallbackFontDescriptor CreateDescriptor(const std::string& fontPath, 
                                         FallbackPriority priority) {
    FallbackFontDescriptor descriptor;
    descriptor.fontPath = fontPath;
    descriptor.fontName = std::filesystem::path(fontPath).stem().string();
    descriptor.priority = priority;
    return descriptor;
}

bool IsCodepointInScript(uint32_t codepoint, ScriptType script) {
    ScriptType detected = TextShaperUtils::GetScriptForCodepoint(codepoint);
    return detected == script;
}

std::unordered_map<ScriptType, std::vector<std::pair<uint32_t, uint32_t>>> GetScriptRanges() {
    std::unordered_map<ScriptType, std::vector<std::pair<uint32_t, uint32_t>>> ranges;

    ranges[ScriptType::Latin] = {
        {0x0041, 0x00FF},  // Basic Latin + Latin-1
        {0x0100, 0x017F},  // Latin Extended-A
        {0x0180, 0x024F}   // Latin Extended-B
    };

    ranges[ScriptType::Cyrillic] = {
        {0x0400, 0x04FF}
    };

    ranges[ScriptType::Greek] = {
        {0x0370, 0x03FF}
    };

    ranges[ScriptType::Arabic] = {
        {0x0600, 0x06FF},
        {0x0750, 0x077F},
        {0xFB50, 0xFDFF}
    };

    ranges[ScriptType::Hebrew] = {
        {0x0590, 0x05FF}
    };

    ranges[ScriptType::Devanagari] = {
        {0x0900, 0x097F}
    };

    ranges[ScriptType::CJK] = {
        {0x4E00, 0x9FFF},
        {0x3400, 0x4DBF},
        {0x20000, 0x2A6DF}
    };

    return ranges;
}

std::string FindBestFallbackForScript(ScriptType script,
                                      const std::vector<FallbackFontDescriptor>& fallbacks) {
    for (const auto& descriptor : fallbacks) {
        if (!descriptor.supportedScripts.empty()) {
            for (uint32_t supportedScript : descriptor.supportedScripts) {
                if (static_cast<ScriptType>(supportedScript) == script) {
                    return descriptor.fontName;
                }
            }
        }
    }

    // Return highest priority fallback if no script match
    if (!fallbacks.empty()) {
        return fallbacks[0].fontName;
    }

    return "";
}

} // namespace FallbackFontSystemUtils

} // namespace we::UI::Text
