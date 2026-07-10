#include "Rendering/Text/FontLoader.h"
#include "Core/Logger.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace we::UI::Text {

// ============================================================================
// FontLoader Implementation
// ============================================================================

FontLoader::FontLoader() = default;

FontLoader::~FontLoader() {
    Shutdown();
}

bool FontLoader::Initialize() {
    if (m_FreeTypeLibrary) {
        HE_WARN("FontLoader: Already initialized");
        return true;
    }

    FT_Error error = FT_Init_FreeType(&m_FreeTypeLibrary);
    if (error != 0) {
        HE_ERROR("FontLoader: FT_Init_FreeType failed with error " + std::to_string(error));
        return false;
    }

    HE_INFO("FontLoader: Initialized FreeType library");
    return true;
}

void FontLoader::Shutdown() {
    if (m_FreeTypeLibrary) {
        FT_Done_FreeType(m_FreeTypeLibrary);
        m_FreeTypeLibrary = nullptr;
        HE_INFO("FontLoader: Shutdown FreeType library");
    }
}

std::string FontLoader::ResolveFontPath(const std::string& fontPath) const {
    if (fontPath.empty()) {
        return {};
    }

    // Check if path is already absolute and exists
    if (std::filesystem::exists(fontPath)) {
        return fontPath;
    }

    // Try search paths
    const std::vector<std::string> searchPaths = FontLoaderUtils::GetDefaultSearchPaths();
    for (const auto& basePath : searchPaths) {
        std::string fullPath = basePath + "/" + fontPath;
        if (std::filesystem::exists(fullPath)) {
            return fullPath;
        }
    }

    // Try direct path as-is
    if (std::filesystem::exists(fontPath)) {
        return fontPath;
    }

    HE_WARN("FontLoader: Font file not found: " + fontPath);
    return fontPath;
}

FontLoadResult FontLoader::LoadFont(const std::string& fontPath, uint32_t faceIndex,
                                    const FontLoaderConfig& config) {
    FontLoadResult result;

    if (!m_FreeTypeLibrary) {
        result.errorMessage = "FontLoader not initialized";
        HE_ERROR("FontLoader: " + result.errorMessage);
        return result;
    }

    std::string resolvedPath = ResolveFontPath(fontPath);
    if (resolvedPath.empty() || !std::filesystem::exists(resolvedPath)) {
        result.errorMessage = "Font file not found: " + fontPath;
        HE_ERROR("FontLoader: " + result.errorMessage);
        return result;
    }

    FT_Face face = nullptr;
    FT_Error error = FT_New_Face(m_FreeTypeLibrary, resolvedPath.c_str(), faceIndex, &face);
    if (error != 0) {
        result.errorMessage = "FT_New_Face failed with error " + std::to_string(error) + " for: " + resolvedPath;
        HE_ERROR("FontLoader: " + result.errorMessage);
        return result;
    }

    if (!face) {
        result.errorMessage = "FT_New_Face returned null face for: " + resolvedPath;
        HE_ERROR("FontLoader: " + result.errorMessage);
        return result;
    }

    ApplyConfig(face, config);

    result.success = true;
    result.face = face;
    result.info = ExtractFontInfo(face);

    HE_INFO("FontLoader: Loaded font '" + result.info.familyName + "' from " + resolvedPath);
    return result;
}

FontLoadResult FontLoader::LoadFontFromMemory(const void* data, size_t dataSize, uint32_t faceIndex,
                                              const FontLoaderConfig& config) {
    FontLoadResult result;

    if (!m_FreeTypeLibrary) {
        result.errorMessage = "FontLoader not initialized";
        HE_ERROR("FontLoader: " + result.errorMessage);
        return result;
    }

    if (!data || dataSize == 0) {
        result.errorMessage = "Invalid font data";
        HE_ERROR("FontLoader: " + result.errorMessage);
        return result;
    }

    FT_Face face = nullptr;
    FT_Error error = FT_New_Memory_Face(m_FreeTypeLibrary, 
                                        static_cast<const FT_Byte*>(data),
                                        static_cast<FT_Long>(dataSize),
                                        faceIndex, &face);
    if (error != 0) {
        result.errorMessage = "FT_New_Memory_Face failed with error " + std::to_string(error);
        HE_ERROR("FontLoader: " + result.errorMessage);
        return result;
    }

    if (!face) {
        result.errorMessage = "FT_New_Memory_Face returned null face";
        HE_ERROR("FontLoader: " + result.errorMessage);
        return result;
    }

    ApplyConfig(face, config);

    result.success = true;
    result.face = face;
    result.info = ExtractFontInfo(face);

    HE_INFO("FontLoader: Loaded font from memory '" + result.info.familyName + "'");
    return result;
}

void FontLoader::UnloadFont(FT_FaceRec_* face) {
    if (face) {
        FT_Done_Face(face);
    }
}

void FontLoader::ApplyConfig(FT_FaceRec_* face, const FontLoaderConfig& config) const {
    if (!face) {
        return;
    }

    // Enable kerning if requested
    if (config.loadKerning) {
        FT_Error error = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
        if (error != 0) {
            HE_WARN("FontLoader: Failed to select Unicode charmap");
        }
    }
}

bool FontLoader::SetFontSize(FT_FaceRec_* face, uint32_t width, uint32_t height) {
    if (!face) {
        return false;
    }

    FT_Error error = FT_Set_Pixel_Sizes(face, width, height);
    if (error != 0) {
        HE_ERROR("FontLoader: FT_Set_Pixel_Sizes failed with error " + std::to_string(error));
        return false;
    }

    return true;
}

bool FontLoader::SetFontSizePoints(FT_FaceRec_* face, uint32_t width, uint32_t height,
                                    uint32_t horizontalDPI, uint32_t verticalDPI) {
    if (!face) {
        return false;
    }

    FT_Error error = FT_Set_Char_Size(face, width * 64, height * 64, horizontalDPI, verticalDPI);
    if (error != 0) {
        HE_ERROR("FontLoader: FT_Set_Char_Size failed with error " + std::to_string(error));
        return false;
    }

    return true;
}

FontInfo FontLoader::ExtractFontInfo(FT_FaceRec_* face) const {
    FontInfo info;

    if (!face) {
        return info;
    }

    // Basic information
    if (face->family_name) {
        info.familyName = face->family_name;
    }
    if (face->style_name) {
        info.styleName = face->style_name;
    }

    // Face information
    info.faceIndex = static_cast<uint16_t>(face->face_index);
    info.faceCount = static_cast<uint16_t>(face->num_faces);
    info.glyphCount = face->num_glyphs;

    // Flags
    info.isFixedPitch = (face->face_flags & FT_FACE_FLAG_FIXED_WIDTH) != 0;
    info.hasVerticalMetrics = (face->face_flags & FT_FACE_FLAG_VERTICAL) != 0;

    // Extract style information from Mac style or PS name
    if (face->style_flags & FT_STYLE_FLAG_ITALIC) {
        info.style = FontStyle::Italic;
    }

    // Extract weight from style flags or use regular as default
    if (face->style_flags & FT_STYLE_FLAG_BOLD) {
        info.weight = FontWeight::Bold;
    }

    // Extract SFNT table information
    if (FT_IS_SFNT(face)) {
        // Try to get more detailed information from SFNT tables
        // This would require accessing specific tables like OS/2, head, etc.
        // For now, we use basic FreeType information
    }

    // Check for variable font
    info.isVariableFont = FT_HAS_MULTIPLE_MASTERS(face) != 0;

    // Extract metrics
    info.metrics = ExtractFontMetrics(face);

    return info;
}

FontMetrics FontLoader::ExtractFontMetrics(FT_FaceRec_* face) const {
    FontMetrics metrics;

    if (!face || !face->size) {
        return metrics;
    }

    const float scale = 1.0f / 64.0f; // Convert from 26.6 fixed-point to float

    // Size metrics
    metrics.unitsPerEm = static_cast<float>(face->units_per_EM);
    metrics.ascender = static_cast<float>(face->size->metrics.ascender) * scale;
    metrics.descender = static_cast<float>(face->size->metrics.descender) * scale;
    metrics.lineHeight = static_cast<float>(face->size->metrics.height) * scale;

    // Underline metrics
    if (face->underline_position != 0 || face->underline_thickness != 0) {
        metrics.underlinePosition = static_cast<float>(face->underline_position) * scale;
        metrics.underlineThickness = static_cast<float>(face->underline_thickness) * scale;
    }

    // Bounding box
    metrics.bboxMinX = static_cast<float>(face->bbox.xMin) * scale;
    metrics.bboxMinY = static_cast<float>(face->bbox.yMin) * scale;
    metrics.bboxMaxX = static_cast<float>(face->bbox.xMax) * scale;
    metrics.bboxMaxY = static_cast<float>(face->bbox.yMax) * scale;

    // Note: OS/2 table extraction removed due to FreeType API compatibility
    // xHeight and capHeight will use default estimates

    return metrics;
}

FontInfo FontLoader::GetFontInfo(FT_FaceRec_* face) const {
    return ExtractFontInfo(face);
}

FontMetrics FontLoader::GetFontMetrics(FT_FaceRec_* face) const {
    return ExtractFontMetrics(face);
}

FontFormat FontLoader::DetectFormat(const std::string& fontPath) const {
    std::filesystem::path path(fontPath);
    std::string extension = path.extension().string();

    // Convert to lowercase for comparison
    std::transform(extension.begin(), extension.end(), extension.begin(), [](int c) { return static_cast<char>(::tolower(c)); });

    if (extension == ".ttf" || extension == ".ttc") {
        return FontFormat::TrueType;
    } else if (extension == ".otf") {
        return FontFormat::OpenType;
    } else if (extension == ".woff") {
        return FontFormat::WOFF;
    } else if (extension == ".woff2") {
        return FontFormat::WOFF2;
    } else if (extension == ".pfb" || extension == ".pfa") {
        return FontFormat::Type1;
    } else if (extension == ".bdf" || extension == ".pcf") {
        return FontFormat::Bitmap;
    }

    return FontFormat::Unknown;
}

bool FontLoader::ValidateFontFile(const std::string& fontPath) const {
    if (!m_FreeTypeLibrary) {
        return false;
    }

    std::string resolvedPath = ResolveFontPath(fontPath);
    if (resolvedPath.empty() || !std::filesystem::exists(resolvedPath)) {
        return false;
    }

    // Try to load the font to validate it
    FT_Face face = nullptr;
    FT_Error error = FT_New_Face(m_FreeTypeLibrary, resolvedPath.c_str(), 0, &face);
    if (error != 0 || !face) {
        return false;
    }

    FT_Done_Face(face);
    return true;
}

uint32_t FontLoader::GetFaceCount(const std::string& fontPath) const {
    if (!m_FreeTypeLibrary) {
        return 0;
    }

    std::string resolvedPath = ResolveFontPath(fontPath);
    if (resolvedPath.empty() || !std::filesystem::exists(resolvedPath)) {
        return 0;
    }

    FT_Face face = nullptr;
    FT_Error error = FT_New_Face(m_FreeTypeLibrary, resolvedPath.c_str(), 0, &face);
    if (error != 0 || !face) {
        return 0;
    }

    uint32_t faceCount = static_cast<uint32_t>(face->num_faces);
    FT_Done_Face(face);

    return faceCount;
}

// ============================================================================
// FontValidator Implementation
// ============================================================================

bool FontValidator::Validate(const std::string& fontPath, std::vector<std::string>& outErrors) {
    outErrors.clear();

    // Check if file exists
    if (!std::filesystem::exists(fontPath)) {
        outErrors.push_back("Font file does not exist: " + fontPath);
        return false;
    }

    // Check file size
    std::ifstream file(fontPath, std::ios::binary | std::ios::ate);
    if (!file) {
        outErrors.push_back("Cannot open font file: " + fontPath);
        return false;
    }

    std::streamsize fileSize = file.tellg();
    if (fileSize == 0) {
        outErrors.push_back("Font file is empty: " + fontPath);
        return false;
    }

    if (fileSize > 100 * 1024 * 1024) { // 100 MB limit
        outErrors.push_back("Font file is too large (>100MB): " + fontPath);
        return false;
    }

    // Try to load with FreeType
    FT_Library library = nullptr;
    if (FT_Init_FreeType(&library) != 0) {
        outErrors.push_back("Failed to initialize FreeType");
        return false;
    }

    FT_Face face = nullptr;
    FT_Error error = FT_New_Face(library, fontPath.c_str(), 0, &face);
    if (error != 0) {
        outErrors.push_back("FreeType failed to load font (error " + std::to_string(error) + ")");
        FT_Done_FreeType(library);
        return false;
    }

    if (!face) {
        outErrors.push_back("FreeType returned null face");
        FT_Done_FreeType(library);
        return false;
    }

    // Validate face properties
    if (face->num_glyphs == 0) {
        outErrors.push_back("Font has no glyphs");
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return false;
    }

    if (!face->family_name) {
        outErrors.push_back("Font has no family name");
    }

    // Check for Unicode charmap
    bool hasUnicode = false;
    for (int i = 0; i < face->num_charmaps; ++i) {
        if (face->charmaps[i]->encoding == FT_ENCODING_UNICODE) {
            hasUnicode = true;
            break;
        }
    }

    if (!hasUnicode) {
        outErrors.push_back("Font does not have Unicode charmap");
    }

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return outErrors.empty();
}

bool FontValidator::QuickValidate(const std::string& fontPath) {
    std::vector<std::string> errors;
    return Validate(fontPath, errors);
}

bool FontValidator::GetFileInfo(const std::string& fontPath, FontInfo& outInfo) {
    FT_Library library = nullptr;
    if (FT_Init_FreeType(&library) != 0) {
        return false;
    }

    FT_Face face = nullptr;
    if (FT_New_Face(library, fontPath.c_str(), 0, &face) != 0) {
        FT_Done_FreeType(library);
        return false;
    }

    // Extract basic info
    if (face) {
        if (face->family_name) {
            outInfo.familyName = face->family_name;
        }
        if (face->style_name) {
            outInfo.styleName = face->style_name;
        }
        outInfo.faceIndex = static_cast<uint16_t>(face->face_index);
        outInfo.faceCount = static_cast<uint16_t>(face->num_faces);
        outInfo.glyphCount = face->num_glyphs;
        outInfo.isFixedPitch = (face->face_flags & FT_FACE_FLAG_FIXED_WIDTH) != 0;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return true;
}

// ============================================================================
// FontLoaderUtils Implementation
// ============================================================================

FontWeight FontLoaderUtils::ValueToWeight(uint16_t value) {
    // Round to nearest 100
    uint16_t rounded = ((value + 50) / 100) * 100;
    rounded = std::clamp(rounded, static_cast<uint16_t>(100), static_cast<uint16_t>(900));

    switch (rounded) {
        case 100: return FontWeight::Thin;
        case 200: return FontWeight::ExtraLight;
        case 300: return FontWeight::Light;
        case 400: return FontWeight::Regular;
        case 500: return FontWeight::Medium;
        case 600: return FontWeight::SemiBold;
        case 700: return FontWeight::Bold;
        case 800: return FontWeight::ExtraBold;
        case 900: return FontWeight::Black;
        default: return FontWeight::Regular;
    }
}

std::vector<std::string> FontLoaderUtils::GetDefaultSearchPaths() {
    std::vector<std::string> paths;

    // Engine asset paths
    paths.push_back("Assets/Fonts");
    paths.push_back("Fonts");
    paths.push_back("../Assets/Fonts");
    paths.push_back("../Fonts");

    // Platform-specific system font paths
#ifdef _WIN32
    paths.push_back("C:/Windows/Fonts");
#elif defined(__APPLE__)
    paths.push_back("/System/Library/Fonts");
    paths.push_back("/Library/Fonts");
    paths.push_back("~/Library/Fonts");
#elif defined(__linux__)
    paths.push_back("/usr/share/fonts");
    paths.push_back("/usr/local/share/fonts");
    paths.push_back("~/.local/share/fonts");
    paths.push_back("~/.fonts");
#endif

    return paths;
}

std::string FontLoaderUtils::FindSystemFont(const std::string& familyName, const std::string& style) {
    std::vector<std::string> searchPaths = GetDefaultSearchPaths();

    for (const auto& basePath : searchPaths) {
        if (!std::filesystem::exists(basePath)) {
            continue;
        }

        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(basePath)) {
                if (!entry.is_regular_file()) {
                    continue;
                }

                std::string path = entry.path().string();
                std::string filename = entry.path().filename().string();

                // Check if filename contains family name (case-insensitive)
                std::string lowerFilename = filename;
                std::string lowerFamilyName = familyName;
                std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), [](int c) { return static_cast<char>(::tolower(c)); });
                std::transform(lowerFamilyName.begin(), lowerFamilyName.end(), lowerFamilyName.begin(), [](int c) { return static_cast<char>(::tolower(c)); });

                if (lowerFilename.find(lowerFamilyName) != std::string::npos) {
                    // Check style if specified
                    if (!style.empty()) {
                        std::string lowerStyle = style;
                        std::transform(lowerStyle.begin(), lowerStyle.end(), lowerStyle.begin(), [](int c) { return static_cast<char>(::tolower(c)); });
                        if (lowerFilename.find(lowerStyle) != std::string::npos) {
                            return path;
                        }
                    } else {
                        return path;
                    }
                }
            }
        } catch (const std::filesystem::filesystem_error&) {
            // Skip directories we can't access
            continue;
        }
    }

    return "";
}

std::vector<std::string> FontLoaderUtils::ListFontsInDirectory(const std::string& directory) {
    std::vector<std::string> fontFiles;

    if (!std::filesystem::exists(directory)) {
        return fontFiles;
    }

    const std::vector<std::string> extensions = {".ttf", ".otf", ".woff", ".woff2", ".ttc"};

    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            std::string extension = entry.path().extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), [](int c) { return static_cast<char>(::tolower(c)); });

            for (const auto& validExt : extensions) {
                if (extension == validExt) {
                    fontFiles.push_back(entry.path().string());
                    break;
                }
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        // Skip directories we can't access
    }

    return fontFiles;
}

bool FontLoaderUtils::SupportsScript(FT_FaceRec_* face, const std::string& script) {
    if (!face) {
        return false;
    }

    // This is a simplified check - a full implementation would check
    // the GSUB/GPOS tables and Unicode ranges in the OS/2 table
    // For now, we'll do a basic check by looking for some common characters

    uint32_t testCodepoint = 0;

    // Map script tags to test characters
    if (script == "latn") {
        testCodepoint = 'A';
    } else if (script == "arab") {
        testCodepoint = 0x0627; // ARABIC LETTER ALEF
    } else if (script == "cyrl") {
        testCodepoint = 0x0410; // CYRILLIC CAPITAL LETTER A
    } else if (script == "grek") {
        testCodepoint = 0x0391; // GREEK CAPITAL LETTER ALPHA
    } else if (script == "hebr") {
        testCodepoint = 0x05D0; // HEBREW LETTER ALEF
    } else if (script == "deva" || script == "dev2") {
        testCodepoint = 0x0905; // DEVANAGARI LETTER A
    } else if (script == "hani" || script == "kana" || script == "hang") {
        testCodepoint = 0x4E00; // CJK UNIFIED IDEOGRAPH
    } else {
        return false;
    }

    FT_UInt glyphIndex = FT_Get_Char_Index(face, testCodepoint);
    return glyphIndex != 0;
}

std::vector<std::pair<uint32_t, uint32_t>> FontLoaderUtils::GetUnicodeRanges(FT_FaceRec_* face) {
    std::vector<std::pair<uint32_t, uint32_t>> ranges;

    if (!face) {
        return ranges;
    }

    // This is a simplified implementation
    // A full implementation would parse the OS/2 table's Unicode ranges
    // For now, we'll scan the charmap to find actual ranges

    // Get the charmap
    FT_CharMap charmap = face->charmap;
    if (!charmap) {
        return ranges;
    }

    // Scan through common Unicode ranges
    // This is expensive for fonts with many glyphs, so we limit it
    const std::vector<std::pair<uint32_t, uint32_t>> commonRanges = {
        {0x0000, 0x007F}, // Basic Latin
        {0x0080, 0x00FF}, // Latin-1 Supplement
        {0x0100, 0x017F}, // Latin Extended-A
        {0x0180, 0x024F}, // Latin Extended-B
        {0x0370, 0x03FF}, // Greek and Coptic
        {0x0400, 0x04FF}, // Cyrillic
        {0x0590, 0x05FF}, // Hebrew
        {0x0600, 0x06FF}, // Arabic
        {0x0900, 0x097F}, // Devanagari
        {0x4E00, 0x9FFF}, // CJK Unified Ideographs
        {0xAC00, 0xD7AF}, // Hangul Syllables
        {0x3040, 0x309F}, // Hiragana
        {0x30A0, 0x30FF}, // Katakana
    };

    for (const auto& range : commonRanges) {
        bool hasGlyphInRange = false;
        for (uint32_t cp = range.first; cp <= range.second && !hasGlyphInRange; cp += 16) {
            if (FT_Get_Char_Index(face, cp) != 0) {
                hasGlyphInRange = true;
            }
        }
        if (hasGlyphInRange) {
            ranges.push_back(range);
        }
    }

    return ranges;
}

} // namespace we::UI::Text
