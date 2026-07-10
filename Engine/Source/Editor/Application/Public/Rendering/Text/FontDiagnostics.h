#pragma once

#include "Application/Export.h"

#include "Rendering/Text/GlyphManager.h"
#include "Rendering/Text/MsdfAtlasGenerator.h"
#include "Rendering/Text/GpuBatcher.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace we::UI::Text {

/**
 * @brief Diagnostic information level
 */
enum class DiagnosticLevel : uint8_t {
    Info,
    Warning,
    Error,
    Critical
};

/**
 * @brief Diagnostic message
 */
struct DiagnosticMessage {
    DiagnosticLevel level = DiagnosticLevel::Info;
    std::string message;
    std::string category;
    uint64_t timestamp = 0;
};

/**
 * @brief Atlas validation result
 */
struct AtlasValidationResult {
    bool isValid = true;
    std::vector<DiagnosticMessage> messages;
    
    size_t glyphCount = 0;
    size_t overlappingGlyphs = 0;
    size_t outOfBoundsGlyphs = 0;
    size_t invalidUVs = 0;
    
    float packingEfficiency = 0.0f;
    float usedArea = 0.0f;
    float totalArea = 0.0f;
};

/**
 * @brief System health status
 */
struct SystemHealth {
    bool isHealthy = true;
    DiagnosticLevel worstLevel = DiagnosticLevel::Info;
    
    // Module status
    bool fontLoaderHealthy = true;
    bool glyphManagerHealthy = true;
    bool atlasGeneratorHealthy = true;
    bool gpuBatcherHealthy = true;
    bool renderingBackendHealthy = true;
    
    // Resource status
    size_t gpuMemoryUsed = 0;
    size_t gpuMemoryAvailable = 0;
    float gpuMemoryUsagePercent = 0.0f;
    
    // Performance status
    float averageFrameTime = 0.0f;
    float peakFrameTime = 0.0f;
    uint32_t droppedFrames = 0;
};

/**
 * @brief Font diagnostics interface
 * 
 * Provides diagnostics for the font rendering system including:
 * - Atlas preview generation
 * - Validation of atlas data
 * - Performance statistics
 * - System health monitoring
 */
class APPLICATION_API IFontDiagnostics {
public:
    virtual ~IFontDiagnostics() = default;

    /**
     * @brief Initialize the diagnostics system
     * @return true if successful, false otherwise
     */
    virtual bool Initialize() = 0;

    /**
     * @brief Shutdown the diagnostics system
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Validate the MSDF atlas
     * @param atlasGenerator Atlas generator to validate
     * @return AtlasValidationResult
     */
    virtual AtlasValidationResult ValidateAtlas(IMsdfAtlasGenerator* atlasGenerator) = 0;

    /**
     * @brief Generate atlas preview image
     * @param atlasGenerator Atlas generator
     * @param outputPath Path to save preview image
     * @return true if successful, false otherwise
     */
    virtual bool GenerateAtlasPreview(IMsdfAtlasGenerator* atlasGenerator, 
                                      const std::string& outputPath) = 0;

    /**
     * @brief Generate atlas preview as pixel data
     * @param atlasGenerator Atlas generator
     * @param outPixels Output pixel data (RGBA8)
     * @param outWidth Output width
     * @param outHeight Output height
     * @return true if successful, false otherwise
     */
    virtual bool GenerateAtlasPreview(IMsdfAtlasGenerator* atlasGenerator,
                                      std::vector<uint8_t>& outPixels,
                                      int& outWidth, int& outHeight) = 0;

    /**
     * @brief Get glyph statistics
     * @param glyphManager Glyph manager
     * @return GlyphManagerStats
     */
    virtual GlyphManagerStats GetGlyphStats(IGlyphManager* glyphManager) = 0;

    /**
     * @brief Get GPU batcher statistics
     * @param gpuBatcher GPU batcher
     * @return GpuBatcherStats
     */
    virtual GpuBatcherStats GetBatcherStats(IGpuBatcher* gpuBatcher) = 0;

    /**
     * @brief Get system health status
     * @return SystemHealth
     */
    virtual SystemHealth GetSystemHealth() = 0;

    /**
     * @brief Get diagnostic messages
     * @param level Minimum level to include
     * @return Vector of diagnostic messages
     */
    virtual std::vector<DiagnosticMessage> GetMessages(DiagnosticLevel level = DiagnosticLevel::Info) = 0;

    /**
     * @brief Clear diagnostic messages
     */
    virtual void ClearMessages() = 0;

    /**
     * @brief Add a diagnostic message
     * @param level Message level
     * @param category Message category
     * @param message Message text
     */
    virtual void AddMessage(DiagnosticLevel level, const std::string& category, 
                            const std::string& message) = 0;

    /**
     * @brief Export diagnostics to JSON
     * @param outputPath Path to save JSON file
     * @return true if successful, false otherwise
     */
    virtual bool ExportToJson(const std::string& outputPath) = 0;

    /**
     * @brief Export diagnostics to text
     * @param outputPath Path to save text file
     * @return true if successful, false otherwise
     */
    virtual bool ExportToText(const std::string& outputPath) = 0;

    /**
     * @brief Check if diagnostics is initialized
     * @return true if initialized, false otherwise
     */
    virtual bool IsInitialized() const = 0;
};

/**
 * @brief Standard font diagnostics implementation
 */
class APPLICATION_API FontDiagnostics : public IFontDiagnostics {
public:
    FontDiagnostics();
    ~FontDiagnostics() override;

    // Disable copying
    FontDiagnostics(const FontDiagnostics&) = delete;
    FontDiagnostics& operator=(const FontDiagnostics&) = delete;

    bool Initialize() override;
    void Shutdown() override;

    AtlasValidationResult ValidateAtlas(IMsdfAtlasGenerator* atlasGenerator) override;
    bool GenerateAtlasPreview(IMsdfAtlasGenerator* atlasGenerator, 
                              const std::string& outputPath) override;
    bool GenerateAtlasPreview(IMsdfAtlasGenerator* atlasGenerator,
                              std::vector<uint8_t>& outPixels,
                              int& outWidth, int& outHeight) override;

    GlyphManagerStats GetGlyphStats(IGlyphManager* glyphManager) override;
    GpuBatcherStats GetBatcherStats(IGpuBatcher* gpuBatcher) override;

    SystemHealth GetSystemHealth() override;

    std::vector<DiagnosticMessage> GetMessages(DiagnosticLevel level = DiagnosticLevel::Info) override;
    void ClearMessages() override;
    void AddMessage(DiagnosticLevel level, const std::string& category, 
                    const std::string& message) override;

    bool ExportToJson(const std::string& outputPath) override;
    bool ExportToText(const std::string& outputPath) override;

    bool IsInitialized() const override { return m_Initialized; }

private:
    /**
     * @brief Validate atlas UV coordinates
     * @param atlasGenerator Atlas generator
     * @param result Validation result to update
     */
    void ValidateAtlasUVs(IMsdfAtlasGenerator* atlasGenerator, AtlasValidationResult& result);

    /**
     * @brief Check for overlapping glyphs in atlas
     * @param atlasGenerator Atlas generator
     * @param result Validation result to update
     */
    void CheckOverlappingGlyphs(IMsdfAtlasGenerator* atlasGenerator, AtlasValidationResult& result);

    /**
     * @brief Calculate packing efficiency
     * @param atlasGenerator Atlas generator
     * @return Packing efficiency (0-1)
     */
    float CalculatePackingEfficiency(IMsdfAtlasGenerator* atlasGenerator);

    /**
     * @brief Get current timestamp
     * @return Timestamp in milliseconds
     */
    uint64_t GetTimestamp() const;

    std::vector<DiagnosticMessage> m_Messages;
    bool m_Initialized = false;
};

/**
 * @brief Font diagnostics utility functions
 */
namespace FontDiagnosticsUtils {
    /**
     * @brief Format diagnostic level as string
     */
    std::string FormatLevel(DiagnosticLevel level);

    /**
     * @brief Format system health as string
     */
    std::string FormatHealth(const SystemHealth& health);

    /**
     * @brief Format validation result as string
     */
    std::string FormatValidation(const AtlasValidationResult& result);

    /**
     * @brief Create a visual debug overlay for atlas
     * @param pixels Atlas pixels
     * @param width Width
     * @param height Height
     * @param outDebugPixels Output debug pixels
     */
    void CreateDebugOverlay(const std::vector<uint8_t>& pixels, int width, int height,
                            std::vector<uint8_t>& outDebugPixels);

    /**
     * @brief Draw glyph boundaries on debug overlay
     * @param placements Glyph placements
     * @param width Atlas width
     * @param height Atlas height
     * @param pixels Pixel data to draw on
     */
    void DrawGlyphBoundaries(const std::vector<AtlasGlyphPlacement>& placements,
                              int width, int height, std::vector<uint8_t>& pixels);

    /**
     * @brief Estimate GPU memory usage
     * @param glyphManager Glyph manager
     * @param atlasGenerator Atlas generator
     * @param gpuBatcher GPU batcher
     * @return Estimated memory usage in bytes
     */
    size_t EstimateGPUMemoryUsage(IGlyphManager* glyphManager,
                                   IMsdfAtlasGenerator* atlasGenerator,
                                   IGpuBatcher* gpuBatcher);

    /**
     * @brief Generate a diagnostic report
     * @param glyphManager Glyph manager
     * @param atlasGenerator Atlas generator
     * @param gpuBatcher GPU batcher
     * @return Formatted report string
     */
    std::string GenerateReport(IGlyphManager* glyphManager,
                               IMsdfAtlasGenerator* atlasGenerator,
                               IGpuBatcher* gpuBatcher);
};

} // namespace we::UI::Text
