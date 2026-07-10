#include "Rendering/Text/FontSystem.h"
#include "Core/Logger.h"

#include <algorithm>

namespace we::UI::Text {

// ============================================================================
// FontSystem Implementation
// ============================================================================

FontSystem::FontSystem() = default;

FontSystem::~FontSystem() {
    Shutdown();
}

bool FontSystem::Initialize(const TextRenderConfig& config) {
    if (m_Initialized) {
        HE_WARN("FontSystem: Already initialized");
        return true;
    }

    // Validate configuration
    std::vector<std::string> errors;
    if (!FontSystemUtils::ValidateConfig(config, errors)) {
        for (const auto& error : errors) {
            HE_ERROR("FontSystem: Config error - " + error);
        }
        return false;
    }

    m_Config = config;
    m_CurrentFontPath = config.fontPath;
    m_CurrentFallbackPath = config.fallbackFontPath;

    if (!InitializeModules(config)) {
        HE_ERROR("FontSystem: Failed to initialize modules");
        ShutdownModules();
        return false;
    }

    // Load primary font
    if (!m_CurrentFontPath.empty()) {
        if (!LoadFont(m_CurrentFontPath)) {
            HE_ERROR("FontSystem: Failed to load primary font: " + m_CurrentFontPath);
            ShutdownModules();
            return false;
        }
    }

    // Load fallback font if specified
    if (!m_CurrentFallbackPath.empty()) {
        if (!SetFallbackFont(m_CurrentFallbackPath)) {
            HE_WARN("FontSystem: Failed to load fallback font: " + m_CurrentFallbackPath);
        }
    }

    m_Stats = {};
    m_CurrentFrame = 0;
    m_AtlasDirty = false;
    m_Initialized = true;

    HE_INFO("FontSystem: Initialized with font '" + m_CurrentFontPath + "'");
    return true;
}

void FontSystem::Shutdown() {
    if (!m_Initialized) {
        return;
    }

    ShutdownModules();

    m_PrimaryFace = nullptr;
    m_FallbackFace = nullptr;
    m_AtlasTexture = {};
    m_Stats = {};
    m_Initialized = false;

    HE_INFO("FontSystem: Shutdown");
}

bool FontSystem::InitializeModules(const TextRenderConfig& config) {
    // Create font loader
    m_FontLoader = std::make_unique<FontLoader>();
    if (!m_FontLoader->Initialize()) {
        HE_ERROR("FontSystem: Failed to initialize font loader");
        return false;
    }

    // Create Unicode decoder
    m_UnicodeDecoder = std::make_unique<UnicodeDecoder>();

    // Create glyph manager
    m_GlyphManager = std::make_unique<GlyphManager>();
    GlyphManagerConfig glyphConfig;
    glyphConfig.maxCacheSize = 4096;
    glyphConfig.enableLRUEviction = true;
    glyphConfig.trackStatistics = true;
    glyphConfig.autoLoadMissing = true;
    glyphConfig.preloadCommonGlyphs = true;
    if (!m_GlyphManager->Initialize(glyphConfig)) {
        HE_ERROR("FontSystem: Failed to initialize glyph manager");
        return false;
    }

    // Create MSDF atlas generator
    m_AtlasGenerator = std::make_unique<MsdfAtlasGenerator>();
    MsdfAtlasConfig atlasConfig;
    atlasConfig.initialWidth = config.atlasWidth;
    atlasConfig.initialHeight = config.atlasHeight;
    atlasConfig.emSize = config.fontSize;
    atlasConfig.pixelRange = config.msdfPixelRange;
    atlasConfig.mode = MsdfMode::MSDF;
    atlasConfig.edgeColoring = EdgeColoring::Simple;
    atlasConfig.packingConstraint = PackingConstraint::PowerOfTwoSquare;
    atlasConfig.spacing = 2;
    atlasConfig.overlapSupport = true;
    atlasConfig.scanlinePass = true;
    atlasConfig.generateAlpha = true;
    atlasConfig.invertY = true;
    if (!m_AtlasGenerator->Initialize(atlasConfig)) {
        HE_ERROR("FontSystem: Failed to initialize atlas generator");
        return false;
    }

    // Create GPU batcher
    m_GpuBatcher = std::make_unique<GpuBatcher>();
    GpuBatcherConfig batcherConfig;
    batcherConfig.maxVertices = 65536;
    batcherConfig.maxIndices = 65536;
    batcherConfig.maxBatches = 1024;
    batcherConfig.usePersistentMapping = true;
    batcherConfig.enableBatching = config.enableBatching;
    batcherConfig.enableSorting = true;
    batcherConfig.trackStatistics = true;
    if (!m_GpuBatcher->Initialize(batcherConfig)) {
        HE_ERROR("FontSystem: Failed to initialize GPU batcher");
        return false;
    }

    // Create rendering backend
    m_RenderingBackend = RenderingBackendFactory::CreateBackend(config.graphicsApi);
    if (!m_RenderingBackend) {
        HE_ERROR("FontSystem: Failed to create rendering backend");
        return false;
    }

    if (!m_RenderingBackend->Initialize(config.graphicsApi)) {
        HE_ERROR("FontSystem: Failed to initialize rendering backend");
        return false;
    }

    HE_INFO("FontSystem: All modules initialized successfully");
    return true;
}

void FontSystem::ShutdownModules() {
    if (m_GpuBatcher) {
        m_GpuBatcher->Shutdown();
        m_GpuBatcher.reset();
    }

    if (m_AtlasGenerator) {
        m_AtlasGenerator->Shutdown();
        m_AtlasGenerator.reset();
    }

    if (m_GlyphManager) {
        m_GlyphManager->Shutdown();
        m_GlyphManager.reset();
    }

    if (m_FontLoader) {
        m_FontLoader->Shutdown();
        m_FontLoader.reset();
    }

    m_UnicodeDecoder.reset();

    if (m_RenderingBackend) {
        m_RenderingBackend->Shutdown();
        m_RenderingBackend.reset();
    }
}

bool FontSystem::LoadFont(const std::string& fontPath) {
    if (!m_FontLoader) {
        return false;
    }

    FontLoaderConfig loaderConfig;
    FontLoadResult result = m_FontLoader->LoadFont(fontPath, 0, loaderConfig);
    
    if (!result.success) {
        HE_ERROR("FontSystem: Failed to load font: " + result.errorMessage);
        return false;
    }

    m_PrimaryFace = result.face;
    m_CurrentFontPath = fontPath;

    // Update modules with new font face
    if (m_GlyphManager) {
        m_GlyphManager->SetFontFace(m_PrimaryFace);
    }
    if (m_AtlasGenerator) {
        m_AtlasGenerator->SetFontFace(m_PrimaryFace);
    }

    // Set font size
    if (m_PrimaryFace) {
        m_FontLoader->SetFontSize(m_PrimaryFace, 0, static_cast<uint32_t>(m_Config.fontSize));
    }

    // Preload common glyphs
    if (m_GlyphManager) {
        m_GlyphManager->PreloadCommonGlyphs();
    }

    // Regenerate atlas
    m_AtlasDirty = true;

    HE_INFO("FontSystem: Loaded font '" + result.info.familyName + "' from " + fontPath);
    return true;
}

bool FontSystem::SetFallbackFont(const std::string& fontPath) {
    if (!m_FontLoader) {
        return false;
    }

    FontLoaderConfig loaderConfig;
    FontLoadResult result = m_FontLoader->LoadFont(fontPath, 0, loaderConfig);
    
    if (!result.success) {
        HE_WARN("FontSystem: Failed to load fallback font: " + result.errorMessage);
        return false;
    }

    m_FallbackFace = result.face;
    m_CurrentFallbackPath = fontPath;

    HE_INFO("FontSystem: Loaded fallback font '" + result.info.familyName + "'");
    return true;
}

bool FontSystem::ReloadFont() {
    if (m_CurrentFontPath.empty()) {
        return false;
    }

    // Unload current font
    if (m_PrimaryFace && m_FontLoader) {
        m_FontLoader->UnloadFont(m_PrimaryFace);
        m_PrimaryFace = nullptr;
    }

    // Reload font
    return LoadFont(m_CurrentFontPath);
}

bool FontSystem::SetFontSize(float size) {
    m_Config.fontSize = size;

    if (m_PrimaryFace && m_FontLoader) {
        if (!m_FontLoader->SetFontSize(m_PrimaryFace, 0, static_cast<uint32_t>(size))) {
            return false;
        }
    }

    if (m_FallbackFace && m_FontLoader) {
        m_FontLoader->SetFontSize(m_FallbackFace, 0, static_cast<uint32_t>(size));
    }

    // Update atlas generator config
    if (m_AtlasGenerator) {
        MsdfAtlasConfig config = m_AtlasGenerator->GetConfig();
        config.emSize = size;
        m_AtlasGenerator->UpdateConfig(config);
    }

    // Mark atlas as dirty for regeneration
    m_AtlasDirty = true;

    return true;
}

float FontSystem::GetFontSize() const {
    return m_Config.fontSize;
}

FontMetrics FontSystem::GetFontMetrics() const {
    if (m_PrimaryFace && m_FontLoader) {
        return m_FontLoader->GetFontMetrics(m_PrimaryFace);
    }
    return {};
}

bool FontSystem::EnsureGlyphsForText(std::string_view text) {
    if (!m_UnicodeDecoder || !m_GlyphManager) {
        return false;
    }

    // Decode text to codepoints
    std::vector<uint32_t> codepoints;
    m_UnicodeDecoder->DecodeUtf8(text, codepoints);

    // Load glyphs
    size_t loaded = m_GlyphManager->LoadGlyphs(codepoints);
    
    // Add codepoints to atlas generator
    if (m_AtlasGenerator) {
        m_AtlasGenerator->AddCodepoints(codepoints);
    }

    return loaded > 0;
}

bool FontSystem::RegenerateAtlasIfNeeded() {
    if (!m_AtlasDirty || !m_AtlasGenerator) {
        return true;
    }

    std::vector<uint32_t> codepoints = m_GlyphManager->GetCachedCodepoints();
    MsdfAtlasResult result = m_AtlasGenerator->RegenerateAtlas();

    if (!result.success) {
        HE_ERROR("FontSystem: Atlas regeneration failed: " + result.errorMessage);
        return false;
    }

    // Update glyph manager with new atlas UVs
    for (const auto& placement : m_AtlasGenerator->GetAllPlacements()) {
        m_GlyphManager->UpdateAtlasUV(placement.codepoint,
                                     placement.atlasLeft, placement.atlasBottom,
                                     placement.atlasRight, placement.atlasTop,
                                     placement.atlasX, placement.atlasY,
                                     placement.atlasWidth, placement.atlasHeight);
    }

    m_AtlasDirty = false;

    HE_INFO("FontSystem: Regenerated atlas " + std::to_string(result.atlasWidth) + 
            "x" + std::to_string(result.atlasHeight));
    return true;
}

bool FontSystem::UploadAtlasIfNeeded() {
    if (!m_RenderingBackend || !m_AtlasGenerator) {
        return false;
    }

    const auto& pixels = m_AtlasGenerator->GetAtlasPixels();
    if (pixels.empty()) {
        return false;
    }

    auto [width, height] = m_AtlasGenerator->GetAtlasDimensions();
    
    // Create or update atlas texture
    if (m_AtlasTexture.handle == 0) {
        m_AtlasTexture = m_RenderingBackend->CreateMsdfAtlas(pixels, width, height);
    } else {
        m_RenderingBackend->UpdateTexture(m_AtlasTexture, pixels.data(), pixels.size());
    }

    return true;
}

void FontSystem::BeginFrame() {
    m_CurrentFrame++;

    if (m_GpuBatcher) {
        m_GpuBatcher->BeginFrame();
    }

    if (m_GlyphManager) {
        m_GlyphManager->SetCurrentFrame(m_CurrentFrame);
    }
}

void FontSystem::EndFrame() {
    // Regenerate atlas if needed
    RegenerateAtlasIfNeeded();

    // Upload atlas to GPU
    UploadAtlasIfNeeded();

    // End GPU batcher frame
    if (m_GpuBatcher) {
        m_GpuBatcher->EndFrame();
    }

    // Upload vertex/index buffers
    if (m_GpuBatcher && m_GpuBatcher->IsDirty()) {
        m_GpuBatcher->UploadBuffers();
    }

    UpdateStatistics();
}

bool FontSystem::RenderText(std::string_view text, float x, float y, const float color[4]) {
    if (!m_Initialized || !m_GlyphManager || !m_AtlasGenerator || !m_GpuBatcher) {
        return false;
    }

    // Ensure glyphs are loaded
    if (!EnsureGlyphsForText(text)) {
        HE_WARN("FontSystem: Failed to ensure glyphs for text");
    }

    // Regenerate atlas if needed
    RegenerateAtlasIfNeeded();

    // Decode text to codepoints
    std::vector<uint32_t> codepoints;
    m_UnicodeDecoder->DecodeUtf8(text, codepoints);

    // Build glyph quads
    std::vector<GlyphQuad> quads;
    quads.reserve(codepoints.size());

    float penX = x;
    float penY = y;

    for (uint32_t codepoint : codepoints) {
        GlyphMetrics metrics;
        if (!m_GlyphManager->GetGlyph(codepoint, metrics)) {
            penX += m_Config.fontSize * 0.5f; // Advance for missing glyph
            continue;
        }

        AtlasGlyphPlacement placement;
        if (!m_AtlasGenerator->GetGlyphPlacement(codepoint, placement)) {
            penX += metrics.advanceX;
            continue;
        }

        GlyphQuad quad;
        m_GpuBatcher->BuildQuad(placement, penX, penY, color, m_Config.edgeSoftness, 0, quad);
        quads.push_back(quad);

        penX += placement.advance;
    }

    // Add quads to batcher
    m_GpuBatcher->AddGlyphs(quads);

    return true;
}

float FontSystem::MeasureText(std::string_view text) const {
    float width = 0.0f;
    float height = 0.0f;
    MeasureText(text, width, height);
    return width;
}

void FontSystem::MeasureText(std::string_view text, float& outWidth, float& outHeight) const {
    outWidth = 0.0f;
    outHeight = 0.0f;

    if (!m_UnicodeDecoder || !m_GlyphManager) {
        return;
    }

    std::vector<uint32_t> codepoints;
    m_UnicodeDecoder->DecodeUtf8(text, codepoints);

    float penX = 0.0f;
    float maxHeight = 0.0f;

    for (uint32_t codepoint : codepoints) {
        GlyphMetrics metrics;
        if (m_GlyphManager->GetGlyph(codepoint, metrics)) {
            penX += metrics.advanceX;
            maxHeight = std::max(maxHeight, metrics.height);
        } else {
            penX += m_Config.fontSize * 0.5f;
        }
    }

    outWidth = penX;
    outHeight = maxHeight;
}

TextRenderStats FontSystem::GetStatistics() const {
    TextRenderStats stats = m_Stats;

    // Collect statistics from modules
    if (m_AtlasGenerator) {
        stats.atlasGlyphCount = m_AtlasGenerator->GetCodepointCount();
        auto [width, height] = m_AtlasGenerator->GetAtlasDimensions();
        stats.atlasWidth = width;
        stats.atlasHeight = height;
    }

    if (m_GlyphManager) {
        GlyphManagerStats glyphStats = m_GlyphManager->GetStatistics();
        stats.cacheHits = glyphStats.cacheHits;
        stats.cacheMisses = glyphStats.cacheMisses;
        stats.cacheHitRate = glyphStats.hitRate;
    }

    if (m_GpuBatcher) {
        GpuBatcherStats batcherStats = m_GpuBatcher->GetStatistics();
        stats.drawCalls = batcherStats.totalDrawCalls;
        stats.verticesRendered = batcherStats.totalVertices;
        stats.indicesRendered = batcherStats.totalIndices;
    }

    return stats;
}

void FontSystem::ResetStatistics() {
    m_Stats = {};

    if (m_GlyphManager) {
        m_GlyphManager->ResetStatistics();
    }

    if (m_GpuBatcher) {
        m_GpuBatcher->ResetStatistics();
    }
}

void FontSystem::UpdateStatistics() {
    // Statistics are collected on-demand in GetStatistics()
}

// ============================================================================
// FontSystemUtils Implementation
// ============================================================================

namespace FontSystemUtils {

TextRenderConfig GetDefaultConfig() {
    TextRenderConfig config;
    config.fontSize = 18.0f;
    config.atlasWidth = 1024;
    config.atlasHeight = 1024;
    config.msdfPixelRange = 4.0f;
    config.edgeSoftness = 1.0f;
    config.graphicsApi = RenderingBackendFactory::GetDefaultApi();
    config.enableBatching = true;
    config.enableCaching = true;
    config.asyncLoading = false;
    return config;
}

TextRenderConfig GetHighQualityConfig() {
    TextRenderConfig config = GetDefaultConfig();
    config.atlasWidth = 2048;
    config.atlasHeight = 2048;
    config.msdfPixelRange = 8.0f;
    config.edgeSoftness = 0.5f;
    return config;
}

TextRenderConfig GetPerformanceConfig() {
    TextRenderConfig config = GetDefaultConfig();
    config.atlasWidth = 512;
    config.atlasHeight = 512;
    config.msdfPixelRange = 2.0f;
    config.edgeSoftness = 1.5f;
    config.enableBatching = false;
    return config;
}

bool ValidateConfig(const TextRenderConfig& config, std::vector<std::string>& outErrors) {
    outErrors.clear();

    if (config.fontSize <= 0.0f || config.fontSize > 256.0f) {
        outErrors.push_back("Font size must be between 0 and 256");
    }

    if (config.atlasWidth <= 0 || config.atlasHeight <= 0) {
        outErrors.push_back("Atlas dimensions must be positive");
    }

    if (config.atlasWidth > 16384 || config.atlasHeight > 16384) {
        outErrors.push_back("Atlas dimensions too large (>16384)");
    }

    if (config.msdfPixelRange <= 0.0f || config.msdfPixelRange > 64.0f) {
        outErrors.push_back("MSDF pixel range must be between 0 and 64");
    }

    if (config.edgeSoftness < 0.0f) {
        outErrors.push_back("Edge softness must be non-negative");
    }

    if (!RenderingBackendFactory::IsApiSupported(config.graphicsApi)) {
        outErrors.push_back("Graphics API not supported on this platform");
    }

    return outErrors.empty();
}

std::pair<int, int> CalculateOptimalAtlasSize(float fontSize, size_t glyphCount) {
    // Estimate glyph size
    const float avgGlyphWidth = fontSize * 0.6f;
    const float avgGlyphHeight = fontSize;

    // Calculate total area needed
    const float totalArea = static_cast<float>(glyphCount) * avgGlyphWidth * avgGlyphHeight;

    // Add padding
    const float paddedArea = totalArea * 1.5f;

    // Calculate square dimensions
    const float side = std::sqrt(paddedArea);

    // Round up to power of two
    int size = 1;
    while (size < static_cast<int>(side)) {
        size *= 2;
    }

    // Clamp to reasonable limits
    size = std::clamp(size, 256, 4096);

    return {size, size};
}

std::string GetSystemDefaultFont() {
    // Try to find a common system font
    std::vector<std::string> commonFonts = {
        "Arial.ttf",
        "Roboto-Regular.ttf",
        "OpenSans-Regular.ttf",
        "DejaVuSans.ttf",
        "LiberationSans-Regular.ttf",
        "NotoSans-Regular.ttf"
    };

    for (const auto& fontName : commonFonts) {
        std::string path = FontLoaderUtils::FindSystemFont(fontName);
        if (!path.empty()) {
            return path;
        }
    }

    return "";
}

bool IsValidFontFile(const std::string& fontPath) {
    return FontValidator::QuickValidate(fontPath);
}

} // namespace FontSystemUtils

} // namespace we::UI::Text
