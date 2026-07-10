#pragma once

#include "Application/Export.h"

#include "Rendering/Text/FontLoader.h"
#include "Rendering/Text/GlyphManager.h"
#include "Rendering/Text/TextShaper.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

struct FT_FaceRec_;

namespace we::UI::Text {

/**
 * @brief Fallback priority level
 */
enum class FallbackPriority : uint8_t {
    Critical,  // Always use (e.g., replacement character)
    High,      // Use for common missing glyphs
    Medium,    // Use for script-specific glyphs
    Low        // Use as last resort
};

/**
 * @brief Fallback font descriptor
 */
struct FallbackFontDescriptor {
    std::string fontPath;
    std::string fontName;
    uint32_t faceIndex = 0;
    FallbackPriority priority = FallbackPriority::Medium;
    std::vector<uint32_t> supportedScripts;  // Script ranges this font supports
    bool isDefault = false;
};

/**
 * @brief Fallback result
 */
struct FallbackResult {
    bool success = false;
    uint32_t fallbackCodepoint = 0;
    std::string fallbackFontName;
    FT_FaceRec_* fallbackFace = nullptr;
    bool usedReplacement = false;
};

/**
 * @brief Fallback font system interface
 * 
 * Provides fallback font support for missing glyphs.
 * Automatically selects appropriate fallback fonts based on script and priority.
 */
class APPLICATION_API IFallbackFontSystem {
public:
    virtual ~IFallbackFontSystem() = default;

    /**
     * @brief Initialize the fallback system
     * @return true if successful, false otherwise
     */
    virtual bool Initialize() = 0;

    /**
     * @brief Shutdown the fallback system
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Add a fallback font
     * @param descriptor Fallback font descriptor
     * @return true if successful, false otherwise
     */
    virtual bool AddFallbackFont(const FallbackFontDescriptor& descriptor) = 0;

    /**
     * @brief Remove a fallback font
     * @param fontName Font name to remove
     * @return true if successful, false otherwise
     */
    virtual bool RemoveFallbackFont(const std::string& fontName) = 0;

    /**
     * @brief Set the primary font loader
     * @param fontLoader Font loader instance
     */
    virtual void SetFontLoader(FontLoader* fontLoader) = 0;

    /**
     * @brief Set the primary glyph manager
     * @param glyphManager Glyph manager instance
     */
    virtual void SetGlyphManager(GlyphManager* glyphManager) = 0;

    /**
     * @brief Find fallback for a missing codepoint
     * @param codepoint Missing codepoint
     * @param primaryFace Primary font face
     * @return FallbackResult
     */
    virtual FallbackResult FindFallback(uint32_t codepoint, FT_FaceRec_* primaryFace) = 0;

    /**
     * @brief Get replacement character (U+FFFD)
     * @return Replacement character codepoint
     */
    virtual uint32_t GetReplacementCharacter() const = 0;

    /**
     * @brief Set replacement character
     * @param codepoint Replacement character codepoint
     */
    virtual void SetReplacementCharacter(uint32_t codepoint) = 0;

    /**
     * @brief Get all fallback fonts
     * @return Vector of fallback font descriptors
     */
    virtual std::vector<FallbackFontDescriptor> GetFallbackFonts() const = 0;

    /**
     * @brief Get fallback font by name
     * @param fontName Font name
     * @return Fallback font descriptor (empty if not found)
     */
    virtual FallbackFontDescriptor GetFallbackFont(const std::string& fontName) const = 0;

    /**
     * @brief Check if a codepoint is supported by any fallback font
     * @param codepoint Codepoint to check
     * @return true if supported, false otherwise
     */
    virtual bool IsSupportedByFallback(uint32_t codepoint) const = 0;

    /**
     * @brief Get fallback statistics
     * @return Map of font name to usage count
     */
    virtual std::unordered_map<std::string, size_t> GetStatistics() const = 0;

    /**
     * @brief Reset statistics
     */
    virtual void ResetStatistics() = 0;

    /**
     * @brief Check if fallback system is initialized
     * @return true if initialized, false otherwise
     */
    virtual bool IsInitialized() const = 0;
};

/**
 * @brief Standard fallback font system implementation
 */
class APPLICATION_API FallbackFontSystem : public IFallbackFontSystem {
public:
    FallbackFontSystem();
    ~FallbackFontSystem() override;

    // Disable copying
    FallbackFontSystem(const FallbackFontSystem&) = delete;
    FallbackFontSystem& operator=(const FallbackFontSystem&) = delete;

    bool Initialize() override;
    void Shutdown() override;

    bool AddFallbackFont(const FallbackFontDescriptor& descriptor) override;
    bool RemoveFallbackFont(const std::string& fontName) override;

    void SetFontLoader(FontLoader* fontLoader) override;
    void SetGlyphManager(GlyphManager* glyphManager) override;

    FallbackResult FindFallback(uint32_t codepoint, FT_FaceRec_* primaryFace) override;

    uint32_t GetReplacementCharacter() const override { return m_ReplacementCharacter; }
    void SetReplacementCharacter(uint32_t codepoint) override { m_ReplacementCharacter = codepoint; }

    std::vector<FallbackFontDescriptor> GetFallbackFonts() const override;
    FallbackFontDescriptor GetFallbackFont(const std::string& fontName) const override;

    bool IsSupportedByFallback(uint32_t codepoint) const override;

    std::unordered_map<std::string, size_t> GetStatistics() const override;
    void ResetStatistics() override;

    bool IsInitialized() const override { return m_Initialized; }

private:
    /**
     * @brief Load fallback font face
     * @param descriptor Font descriptor
     * @return Font face, or nullptr if failed
     */
    FT_FaceRec_* LoadFallbackFace(const FallbackFontDescriptor& descriptor);

    /**
     * @brief Unload fallback font face
     * @param fontName Font name
     */
    void UnloadFallbackFace(const std::string& fontName);

    /**
     * @brief Check if a font supports a codepoint
     * @param face Font face
     * @param codepoint Codepoint to check
     * @return true if supported, false otherwise
     */
    bool FontSupportsCodepoint(FT_FaceRec_* face, uint32_t codepoint) const;

    /**
     * @brief Detect script for a codepoint
     * @param codepoint Codepoint
     * @return Script type
     */
    ScriptType DetectScriptForCodepoint(uint32_t codepoint) const;

    /**
     * @brief Sort fallback fonts by priority
     */
    void SortFallbacks();

    /**
     * @brief Load default system fallback fonts
     */
    void LoadDefaultFallbacks();

    struct FallbackFontData {
        FallbackFontDescriptor descriptor;
        FT_FaceRec_* face = nullptr;
        size_t usageCount = 0;
    };

    std::unordered_map<std::string, FallbackFontData> m_FallbackFonts;
    std::vector<std::string> m_SortedFallbacks;  // Sorted by priority
    
    FontLoader* m_FontLoader = nullptr;
    GlyphManager* m_GlyphManager = nullptr;
    
    uint32_t m_ReplacementCharacter = 0xFFFD;  // Unicode replacement character
    bool m_Initialized = false;
};

/**
 * @brief Fallback font system utility functions
 */
namespace FallbackFontSystemUtils {
    /**
     * @brief Get default fallback font descriptors
     * @return Vector of common fallback fonts
     */
    std::vector<FallbackFontDescriptor> GetDefaultFallbacks();

    /**
     * @brief Create fallback descriptor for a font
     * @param fontPath Font file path
     * @param priority Fallback priority
     * @return FallbackFontDescriptor
     */
    FallbackFontDescriptor CreateDescriptor(const std::string& fontPath, 
                                             FallbackPriority priority);

    /**
     * @brief Check if a codepoint is in a script range
     * @param codepoint Codepoint
     * @param script Script type
     * @return true if in range, false otherwise
     */
    bool IsCodepointInScript(uint32_t codepoint, ScriptType script);

    /**
     * @brief Get common script ranges
     * @return Map of script type to codepoint ranges
     */
    std::unordered_map<ScriptType, std::vector<std::pair<uint32_t, uint32_t>>> GetScriptRanges();

    /**
     * @brief Find best fallback font for a script
     * @param script Script type
     * @param fallbacks Available fallback fonts
     * @return Font name, or empty if none found
     */
    std::string FindBestFallbackForScript(ScriptType script,
                                          const std::vector<FallbackFontDescriptor>& fallbacks);
};

} // namespace we::UI::Text
