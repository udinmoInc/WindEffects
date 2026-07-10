#pragma once

#include "Application/Export.h"

#include "Rendering/Text/UnicodeDecoder.h"
#include "Rendering/Text/FontLoader.h"
#include "Rendering/Text/GlyphManager.h"
#include "Rendering/Text/MsdfAtlasGenerator.h"
#include "Rendering/Text/GpuBatcher.h"
#include "Rendering/Text/RenderingBackend.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace we::UI::Text {

/**
 * @brief Text rendering configuration
 */
struct TextRenderConfig {
    // Font settings
    std::string fontPath;
    std::string fallbackFontPath;
    float fontSize = 18.0f;
    
    // Atlas settings
    int atlasWidth = 1024;
    int atlasHeight = 1024;
    float msdfPixelRange = 4.0f;
    
    // Rendering settings
    float edgeSoftness = 1.0f;
    GraphicsApi graphicsApi = GraphicsApi::Vulkan;
    
    // Performance settings
    bool enableBatching = true;
    bool enableCaching = true;
    bool asyncLoading = false;
};

/**
 * @brief Text rendering statistics
 */
struct TextRenderStats {
    // Atlas statistics
    size_t atlasGlyphCount = 0;
    size_t atlasWidth = 0;
    size_t atlasHeight = 0;
    float packingEfficiency = 0.0f;
    
    // Cache statistics
    size_t cacheHits = 0;
    size_t cacheMisses = 0;
    float cacheHitRate = 0.0f;
    
    // Rendering statistics
    size_t drawCalls = 0;
    size_t verticesRendered = 0;
    size_t indicesRendered = 0;
    
    // Memory statistics
    size_t gpuMemoryUsage = 0;
    size_t cpuMemoryUsage = 0;
};

/**
 * @brief Font system interface
 * 
 * Coordinates all font rendering modules and provides a high-level API.
 * Acts as the main entry point for text rendering functionality.
 */
class APPLICATION_API IFontSystem {
public:
    virtual ~IFontSystem() = default;

    /**
     * @brief Initialize the font system
     * @param config Rendering configuration
     * @return true if successful, false otherwise
     */
    virtual bool Initialize(const TextRenderConfig& config) = 0;

    /**
     * @brief Shutdown the font system
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Render text at the specified position
     * @param text UTF-8 text to render
     * @param x Screen X position
     * @param y Screen Y position
     * @param color RGBA color (0-1 range)
     * @return true if successful, false otherwise
     */
    virtual bool RenderText(std::string_view text, float x, float y, const float color[4]) = 0;

    /**
     * @brief Measure text width
     * @param text UTF-8 text to measure
     * @return Text width in pixels
     */
    virtual float MeasureText(std::string_view text) const = 0;

    /**
     * @brief Measure text dimensions
     * @param text UTF-8 text to measure
     * @param outWidth Output width
     * @param outHeight Output height
     */
    virtual void MeasureText(std::string_view text, float& outWidth, float& outHeight) const = 0;

    /**
     * @brief Get font metrics
     * @return FontMetrics structure
     */
    virtual FontMetrics GetFontMetrics() const = 0;

    /**
     * @brief Set font size
     * @param size Font size in pixels
     * @return true if successful, false otherwise
     */
    virtual bool SetFontSize(float size) = 0;

    /**
     * @brief Get current font size
     * @return Font size in pixels
     */
    virtual float GetFontSize() const = 0;

    /**
     * @brief Load a font
     * @param fontPath Path to font file
     * @return true if successful, false otherwise
     */
    virtual bool LoadFont(const std::string& fontPath) = 0;

    /**
     * @brief Set fallback font
     * @param fontPath Path to fallback font file
     * @return true if successful, false otherwise
     */
    virtual bool SetFallbackFont(const std::string& fontPath) = 0;

    /**
     * @brief Reload current font (hot-reloading)
     * @return true if successful, false otherwise
     */
    virtual bool ReloadFont() = 0;

    /**
     * @brief Begin a new frame
     */
    virtual void BeginFrame() = 0;

    /**
     * @brief End the current frame and submit for rendering
     */
    virtual void EndFrame() = 0;

    /**
     * @brief Get rendering statistics
     * @return TextRenderStats structure
     */
    virtual TextRenderStats GetStatistics() const = 0;

    /**
     * @brief Reset statistics
     */
    virtual void ResetStatistics() = 0;

    /**
     * @brief Check if font system is initialized
     * @return true if initialized, false otherwise
     */
    virtual bool IsInitialized() const = 0;

    /**
     * @brief Get the rendering backend
     * @return Pointer to rendering backend
     */
    virtual IRenderingBackend* GetRenderingBackend() const = 0;

    /**
     * @brief Get the glyph manager
     * @return Pointer to glyph manager
     */
    virtual IGlyphManager* GetGlyphManager() const = 0;

    /**
     * @brief Get the atlas generator
     * @return Pointer to atlas generator
     */
    virtual IMsdfAtlasGenerator* GetAtlasGenerator() const = 0;

    /**
     * @brief Get the GPU batcher
     * @return Pointer to GPU batcher
     */
    virtual IGpuBatcher* GetGpuBatcher() const = 0;
};

/**
 * @brief Standard font system implementation
 */
class APPLICATION_API FontSystem : public IFontSystem {
public:
    FontSystem();
    ~FontSystem() override;

    // Disable copying
    FontSystem(const FontSystem&) = delete;
    FontSystem& operator=(const FontSystem&) = delete;

    bool Initialize(const TextRenderConfig& config) override;
    void Shutdown() override;

    bool RenderText(std::string_view text, float x, float y, const float color[4]) override;
    float MeasureText(std::string_view text) const override;
    void MeasureText(std::string_view text, float& outWidth, float& outHeight) const override;

    FontMetrics GetFontMetrics() const override;
    bool SetFontSize(float size) override;
    float GetFontSize() const override;

    bool LoadFont(const std::string& fontPath) override;
    bool SetFallbackFont(const std::string& fontPath) override;
    bool ReloadFont() override;

    void BeginFrame() override;
    void EndFrame() override;

    TextRenderStats GetStatistics() const override;
    void ResetStatistics() override;

    bool IsInitialized() const override { return m_Initialized; }

    IRenderingBackend* GetRenderingBackend() const override { return m_RenderingBackend.get(); }
    IGlyphManager* GetGlyphManager() const override { return m_GlyphManager.get(); }
    IMsdfAtlasGenerator* GetAtlasGenerator() const override { return m_AtlasGenerator.get(); }
    IGpuBatcher* GetGpuBatcher() const override { return m_GpuBatcher.get(); }

private:
    /**
     * @brief Initialize all modules
     * @param config Rendering configuration
     * @return true if successful, false otherwise
     */
    bool InitializeModules(const TextRenderConfig& config);

    /**
     * @brief Shutdown all modules
     */
    void ShutdownModules();

    /**
     * @brief Ensure glyphs are loaded for text
     * @param text UTF-8 text
     * @return true if successful, false otherwise
     */
    bool EnsureGlyphsForText(std::string_view text);

    /**
     * @brief Regenerate atlas if needed
     * @return true if successful, false otherwise
     */
    bool RegenerateAtlasIfNeeded();

    /**
     * @brief Upload atlas to GPU if needed
     * @return true if successful, false otherwise
     */
    bool UploadAtlasIfNeeded();

    /**
     * @brief Update statistics
     */
    void UpdateStatistics();

    // Module instances
    std::unique_ptr<FontLoader> m_FontLoader;
    std::unique_ptr<UnicodeDecoder> m_UnicodeDecoder;
    std::unique_ptr<GlyphManager> m_GlyphManager;
    std::unique_ptr<MsdfAtlasGenerator> m_AtlasGenerator;
    std::unique_ptr<GpuBatcher> m_GpuBatcher;
    std::unique_ptr<IRenderingBackend> m_RenderingBackend;

    // Configuration
    TextRenderConfig m_Config;
    std::string m_CurrentFontPath;
    std::string m_CurrentFallbackPath;

    // Font face handles
    FT_FaceRec_* m_PrimaryFace = nullptr;
    FT_FaceRec_* m_FallbackFace = nullptr;

    // Atlas texture handle
    TextureHandle m_AtlasTexture;

    // Statistics
    TextRenderStats m_Stats;
    uint32_t m_CurrentFrame = 0;

    // State
    bool m_Initialized = false;
    bool m_AtlasDirty = false;
};

/**
 * @brief Font system utility functions
 */
namespace FontSystemUtils {
    /**
     * @brief Get default rendering configuration
     */
    TextRenderConfig GetDefaultConfig();

    /**
     * @brief Get high-quality rendering configuration
     */
    TextRenderConfig GetHighQualityConfig();

    /**
     * @brief Get performance-oriented configuration
     */
    TextRenderConfig GetPerformanceConfig();

    /**
     * @brief Validate rendering configuration
     * @param config Configuration to validate
     * @param outErrors Output error messages
     * @return true if valid, false otherwise
     */
    bool ValidateConfig(const TextRenderConfig& config, std::vector<std::string>& outErrors);

    /**
     * @brief Calculate optimal atlas size for a font
     * @param fontSize Font size in pixels
     * @param glyphCount Estimated glyph count
     * @return Pair of (width, height)
     */
    std::pair<int, int> CalculateOptimalAtlasSize(float fontSize, size_t glyphCount);

    /**
     * @brief Get system default font path
     * @return Path to default font, or empty if not found
     */
    std::string GetSystemDefaultFont();

    /**
     * @brief Check if a font file is valid
     * @param fontPath Path to font file
     * @return true if valid, false otherwise
     */
    bool IsValidFontFile(const std::string& fontPath);
};

} // namespace we::UI::Text
