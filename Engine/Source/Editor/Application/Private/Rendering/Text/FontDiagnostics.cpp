#include "Rendering/Text/FontDiagnostics.h"
#include "Core/Logger.h"

#include <chrono>
#include <fstream>
#include <algorithm>

namespace we::UI::Text {

// ============================================================================
// FontDiagnostics Implementation
// ============================================================================

FontDiagnostics::FontDiagnostics() = default;

FontDiagnostics::~FontDiagnostics() {
    Shutdown();
}

bool FontDiagnostics::Initialize() {
    if (m_Initialized) {
        HE_WARN("FontDiagnostics: Already initialized");
        return true;
    }

    m_Messages.clear();
    m_Initialized = true;

    HE_INFO("FontDiagnostics: Initialized");
    return true;
}

void FontDiagnostics::Shutdown() {
    if (!m_Initialized) {
        return;
    }

    m_Messages.clear();
    m_Initialized = false;

    HE_INFO("FontDiagnostics: Shutdown");
}

AtlasValidationResult FontDiagnostics::ValidateAtlas(IMsdfAtlasGenerator* atlasGenerator) {
    AtlasValidationResult result;

    if (!atlasGenerator) {
        result.isValid = false;
        AddMessage(DiagnosticLevel::Error, "Atlas", "Atlas generator is null");
        return result;
    }

    auto [width, height] = atlasGenerator->GetAtlasDimensions();
    result.totalArea = static_cast<float>(width * height);

    // Get all placements
    std::vector<AtlasGlyphPlacement> placements = atlasGenerator->GetAllPlacements();
    result.glyphCount = placements.size();

    // Validate UV coordinates
    ValidateAtlasUVs(atlasGenerator, result);

    // Check for overlapping glyphs
    CheckOverlappingGlyphs(atlasGenerator, result);

    // Calculate packing efficiency
    result.packingEfficiency = CalculatePackingEfficiency(atlasGenerator);
    result.usedArea = result.totalArea * result.packingEfficiency;

    // Determine overall validity
    result.isValid = (result.invalidUVs == 0 && result.overlappingGlyphs == 0);

    if (!result.isValid) {
        AddMessage(DiagnosticLevel::Error, "Atlas", "Atlas validation failed");
    } else {
        AddMessage(DiagnosticLevel::Info, "Atlas", "Atlas validation passed");
    }

    return result;
}

void FontDiagnostics::ValidateAtlasUVs(IMsdfAtlasGenerator* atlasGenerator, AtlasValidationResult& result) {
    std::vector<AtlasGlyphPlacement> placements = atlasGenerator->GetAllPlacements();

    for (const auto& placement : placements) {
        // Check UV bounds [0, 1]
        if (placement.atlasLeft < 0.0f || placement.atlasLeft > 1.0f ||
            placement.atlasRight < 0.0f || placement.atlasRight > 1.0f ||
            placement.atlasBottom < 0.0f || placement.atlasBottom > 1.0f ||
            placement.atlasTop < 0.0f || placement.atlasTop > 1.0f) {
            result.invalidUVs++;
            AddMessage(DiagnosticLevel::Warning, "Atlas", 
                      "Glyph U+ " + std::to_string(placement.codepoint) + " has invalid UV coordinates");
        }

        // Check if left < right and bottom < top
        if (placement.atlasLeft >= placement.atlasRight ||
            placement.atlasBottom >= placement.atlasTop) {
            result.invalidUVs++;
            AddMessage(DiagnosticLevel::Error, "Atlas",
                      "Glyph U+ " + std::to_string(placement.codepoint) + " has inverted UV coordinates");
        }
    }
}

void FontDiagnostics::CheckOverlappingGlyphs(IMsdfAtlasGenerator* atlasGenerator, AtlasValidationResult& result) {
    std::vector<AtlasGlyphPlacement> placements = atlasGenerator->GetAllPlacements();

    // Simple overlap check - compare bounding boxes
    for (size_t i = 0; i < placements.size(); ++i) {
        for (size_t j = i + 1; j < placements.size(); ++j) {
            const auto& a = placements[i];
            const auto& b = placements[j];

            // Check if rectangles overlap
            bool overlapX = (a.atlasLeft < b.atlasRight) && (a.atlasRight > b.atlasLeft);
            bool overlapY = (a.atlasBottom < b.atlasTop) && (a.atlasTop > b.atlasBottom);

            if (overlapX && overlapY) {
                result.overlappingGlyphs++;
                AddMessage(DiagnosticLevel::Warning, "Atlas",
                          "Glyphs U+ " + std::to_string(a.codepoint) + 
                          " and U+ " + std::to_string(b.codepoint) + " overlap");
            }
        }
    }
}

float FontDiagnostics::CalculatePackingEfficiency(IMsdfAtlasGenerator* atlasGenerator) {
    std::vector<AtlasGlyphPlacement> placements = atlasGenerator->GetAllPlacements();
    auto [width, height] = atlasGenerator->GetAtlasDimensions();

    float totalGlyphArea = 0.0f;
    for (const auto& placement : placements) {
        float glyphWidth = placement.atlasRight - placement.atlasLeft;
        float glyphHeight = placement.atlasTop - placement.atlasBottom;
        totalGlyphArea += glyphWidth * glyphHeight;
    }

    float totalAtlasArea = static_cast<float>(width * height);
    if (totalAtlasArea > 0.0f) {
        return totalGlyphArea / totalAtlasArea;
    }
    return 0.0f;
}

bool FontDiagnostics::GenerateAtlasPreview(IMsdfAtlasGenerator* atlasGenerator, 
                                           const std::string& outputPath) {
    std::vector<uint8_t> pixels;
    int width = 0;
    int height = 0;

    if (!GenerateAtlasPreview(atlasGenerator, pixels, width, height)) {
        return false;
    }

    // Save as simple bitmap (placeholder - would use stb_image or similar)
    std::ofstream file(outputPath, std::ios::binary);
    if (!file) {
        HE_ERROR("FontDiagnostics: Failed to open output file: " + outputPath);
        return false;
    }

    // Write simple header (placeholder format)
    file.write("ATLAS", 5);
    file.write(reinterpret_cast<const char*>(&width), sizeof(width));
    file.write(reinterpret_cast<const char*>(&height), sizeof(height));
    file.write(reinterpret_cast<const char*>(pixels.data()), pixels.size());
    file.close();

    HE_INFO("FontDiagnostics: Saved atlas preview to " + outputPath);
    return true;
}

bool FontDiagnostics::GenerateAtlasPreview(IMsdfAtlasGenerator* atlasGenerator,
                                           std::vector<uint8_t>& outPixels,
                                           int& outWidth, int& outHeight) {
    if (!atlasGenerator) {
        return false;
    }

    const auto& atlasPixels = atlasGenerator->GetAtlasPixels();
    auto [width, height] = atlasGenerator->GetAtlasDimensions();

    if (atlasPixels.empty() || width == 0 || height == 0) {
        return false;
    }

    // Copy atlas pixels
    outPixels = atlasPixels;
    outWidth = width;
    outHeight = height;

    return true;
}

GlyphManagerStats FontDiagnostics::GetGlyphStats(IGlyphManager* glyphManager) {
    if (glyphManager) {
        return glyphManager->GetStatistics();
    }
    return {};
}

GpuBatcherStats FontDiagnostics::GetBatcherStats(IGpuBatcher* gpuBatcher) {
    if (gpuBatcher) {
        return gpuBatcher->GetStatistics();
    }
    return {};
}

SystemHealth FontDiagnostics::GetSystemHealth() {
    SystemHealth health;
    health.isHealthy = true;
    health.worstLevel = DiagnosticLevel::Info;

    // Check for critical errors in messages
    for (const auto& msg : m_Messages) {
        if (msg.level == DiagnosticLevel::Critical) {
            health.isHealthy = false;
            health.worstLevel = DiagnosticLevel::Critical;
            break;
        }
        if (msg.level == DiagnosticLevel::Error && health.worstLevel < DiagnosticLevel::Error) {
            health.isHealthy = false;
            health.worstLevel = DiagnosticLevel::Error;
        }
    }

    return health;
}

std::vector<DiagnosticMessage> FontDiagnostics::GetMessages(DiagnosticLevel level) {
    std::vector<DiagnosticMessage> filtered;
    
    for (const auto& msg : m_Messages) {
        if (static_cast<int>(msg.level) >= static_cast<int>(level)) {
            filtered.push_back(msg);
        }
    }
    
    return filtered;
}

void FontDiagnostics::ClearMessages() {
    m_Messages.clear();
}

void FontDiagnostics::AddMessage(DiagnosticLevel level, const std::string& category, 
                                  const std::string& message) {
    DiagnosticMessage msg;
    msg.level = level;
    msg.category = category;
    msg.message = message;
    msg.timestamp = GetTimestamp();
    
    m_Messages.push_back(msg);
}

bool FontDiagnostics::ExportToJson(const std::string& outputPath) {
    std::ofstream file(outputPath);
    if (!file) {
        HE_ERROR("FontDiagnostics: Failed to open output file: " + outputPath);
        return false;
    }

    file << "{\n";
    file << "  \"messages\": [\n";

    for (size_t i = 0; i < m_Messages.size(); ++i) {
        const auto& msg = m_Messages[i];
        file << "    {\n";
        file << "      \"level\": \"" << FontDiagnosticsUtils::FormatLevel(msg.level) << "\",\n";
        file << "      \"category\": \"" << msg.category << "\",\n";
        file << "      \"message\": \"" << msg.message << "\",\n";
        file << "      \"timestamp\": " << msg.timestamp << "\n";
        file << "    }";
        if (i < m_Messages.size() - 1) {
            file << ",";
        }
        file << "\n";
    }

    file << "  ]\n";
    file << "}\n";
    file.close();

    HE_INFO("FontDiagnostics: Exported diagnostics to JSON: " + outputPath);
    return true;
}

bool FontDiagnostics::ExportToText(const std::string& outputPath) {
    std::ofstream file(outputPath);
    if (!file) {
        HE_ERROR("FontDiagnostics: Failed to open output file: " + outputPath);
        return false;
    }

    file << "Font Diagnostics Report\n";
    file << "======================\n\n";

    for (const auto& msg : m_Messages) {
        file << "[" << FontDiagnosticsUtils::FormatLevel(msg.level) << "] ";
        file << msg.category << ": " << msg.message << "\n";
    }

    file.close();

    HE_INFO("FontDiagnostics: Exported diagnostics to text: " + outputPath);
    return true;
}

uint64_t FontDiagnostics::GetTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

// ============================================================================
// FontDiagnosticsUtils Implementation
// ============================================================================

namespace FontDiagnosticsUtils {

std::string FormatLevel(DiagnosticLevel level) {
    switch (level) {
        case DiagnosticLevel::Info: return "INFO";
        case DiagnosticLevel::Warning: return "WARNING";
        case DiagnosticLevel::Error: return "ERROR";
        case DiagnosticLevel::Critical: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

std::string FormatHealth(const SystemHealth& health) {
    if (health.isHealthy) {
        return "Healthy";
    }
    return "Unhealthy (" + FormatLevel(health.worstLevel) + ")";
}

std::string FormatValidation(const AtlasValidationResult& result) {
    std::string output = "Atlas Validation Result:\n";
    output += "  Valid: " + std::string(result.isValid ? "Yes" : "No") + "\n";
    output += "  Glyph Count: " + std::to_string(result.glyphCount) + "\n";
    output += "  Overlapping Glyphs: " + std::to_string(result.overlappingGlyphs) + "\n";
    output += "  Invalid UVs: " + std::to_string(result.invalidUVs) + "\n";
    output += "  Packing Efficiency: " + std::to_string(result.packingEfficiency * 100.0f) + "%\n";
    output += "  Used Area: " + std::to_string(result.usedArea) + " pixels\n";
    output += "  Total Area: " + std::to_string(result.totalArea) + " pixels\n";
    return output;
}

void CreateDebugOverlay(const std::vector<uint8_t>& pixels, int width, int height,
                        std::vector<uint8_t>& outDebugPixels) {
    // Copy original pixels
    outDebugPixels = pixels;

    // Draw grid lines every 32 pixels
    for (int y = 0; y < height; y += 32) {
        for (int x = 0; x < width; ++x) {
            size_t index = (y * width + x) * 4;
            if (index + 3 < outDebugPixels.size()) {
                outDebugPixels[index + 0] = 64;  // Dark red grid
                outDebugPixels[index + 1] = 64;
                outDebugPixels[index + 2] = 64;
            }
        }
    }

    for (int x = 0; x < width; x += 32) {
        for (int y = 0; y < height; ++y) {
            size_t index = (y * width + x) * 4;
            if (index + 3 < outDebugPixels.size()) {
                outDebugPixels[index + 0] = 64;
                outDebugPixels[index + 1] = 64;
                outDebugPixels[index + 2] = 64;
            }
        }
    }
}

void DrawGlyphBoundaries(const std::vector<AtlasGlyphPlacement>& placements,
                          int width, int height, std::vector<uint8_t>& pixels) {
    for (const auto& placement : placements) {
        int x0 = static_cast<int>(placement.atlasLeft * width);
        int y0 = static_cast<int>(placement.atlasBottom * height);
        int x1 = static_cast<int>(placement.atlasRight * width);
        int y1 = static_cast<int>(placement.atlasTop * height);

        // Draw rectangle outline in green
        for (int x = x0; x <= x1 && x < width; ++x) {
            if (y0 >= 0 && y0 < height) {
                size_t index = (y0 * width + x) * 4;
                if (index + 3 < pixels.size()) {
                    pixels[index + 1] = 255;  // Green
                }
            }
            if (y1 >= 0 && y1 < height) {
                size_t index = (y1 * width + x) * 4;
                if (index + 3 < pixels.size()) {
                    pixels[index + 1] = 255;  // Green
                }
            }
        }

        for (int y = y0; y <= y1 && y < height; ++y) {
            if (x0 >= 0 && x0 < width) {
                size_t index = (y * width + x0) * 4;
                if (index + 3 < pixels.size()) {
                    pixels[index + 1] = 255;  // Green
                }
            }
            if (x1 >= 0 && x1 < width) {
                size_t index = (y * width + x1) * 4;
                if (index + 3 < pixels.size()) {
                    pixels[index + 1] = 255;  // Green
                }
            }
        }
    }
}

size_t EstimateGPUMemoryUsage(IGlyphManager* glyphManager,
                               IMsdfAtlasGenerator* atlasGenerator,
                               IGpuBatcher* gpuBatcher) {
    size_t total = 0;

    if (atlasGenerator) {
        auto [width, height] = atlasGenerator->GetAtlasDimensions();
        const auto& pixels = atlasGenerator->GetAtlasPixels();
        total += pixels.size();  // Atlas texture
    }

    if (gpuBatcher) {
        GpuBatcherStats stats = gpuBatcher->GetStatistics();
        total += stats.memoryUsageBytes;  // Vertex/index buffers
    }

    return total;
}

std::string GenerateReport(IGlyphManager* glyphManager,
                           IMsdfAtlasGenerator* atlasGenerator,
                           IGpuBatcher* gpuBatcher) {
    std::string report = "Font Rendering System Diagnostics Report\n";
    report += "==========================================\n\n";

    if (glyphManager) {
        GlyphManagerStats glyphStats = glyphManager->GetStatistics();
        report += "Glyph Manager:\n";
        report += "  Total Glyphs: " + std::to_string(glyphStats.totalGlyphs) + "\n";
        report += "  Cache Hits: " + std::to_string(glyphStats.cacheHits) + "\n";
        report += "  Cache Misses: " + std::to_string(glyphStats.cacheMisses) + "\n";
        report += "  Hit Rate: " + std::to_string(glyphStats.hitRate * 100.0f) + "%\n";
        report += "  Memory Usage: " + std::to_string(glyphStats.memoryUsageBytes) + " bytes\n\n";
    }

    if (atlasGenerator) {
        auto [width, height] = atlasGenerator->GetAtlasDimensions();
        report += "Atlas Generator:\n";
        report += "  Dimensions: " + std::to_string(width) + "x" + std::to_string(height) + "\n";
        report += "  Glyph Count: " + std::to_string(atlasGenerator->GetCodepointCount()) + "\n\n";
    }

    if (gpuBatcher) {
        GpuBatcherStats batcherStats = gpuBatcher->GetStatistics();
        report += "GPU Batcher:\n";
        report += "  Total Batches: " + std::to_string(batcherStats.totalBatches) + "\n";
        report += "  Total Vertices: " + std::to_string(batcherStats.totalVertices) + "\n";
        report += "  Total Indices: " + std::to_string(batcherStats.totalIndices) + "\n";
        report += "  Draw Calls: " + std::to_string(batcherStats.totalDrawCalls) + "\n";
        report += "  Memory Usage: " + std::to_string(batcherStats.memoryUsageBytes) + " bytes\n\n";
    }

    report += "Estimated GPU Memory: " + 
              std::to_string(EstimateGPUMemoryUsage(glyphManager, atlasGenerator, gpuBatcher)) + 
              " bytes\n";

    return report;
}

} // namespace FontDiagnosticsUtils

} // namespace we::UI::Text
