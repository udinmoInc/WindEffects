#include "Rendering/Text/Validation.h"
#include "Rendering/Text/FontSystem.h"
#include "Core/Logger.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4458 4505)
#endif
#include <msdf-atlas-gen/msdf-atlas-gen.h>
#include <msdfgen.h>
#include <msdfgen-ext.h>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include <filesystem>
#include <fstream>
#include <cmath>

namespace we::UI::Text {

// ============================================================================
// ValidationResult Implementation
// ============================================================================

std::vector<std::string> ValidationResult::GetAllMessages() const {
    std::vector<std::string> allMessages;
    allMessages.reserve(errors.size() + warnings.size() + info.size());
    
    for (const auto& msg : errors) {
        allMessages.push_back("[ERROR] " + msg);
    }
    for (const auto& msg : warnings) {
        allMessages.push_back("[WARNING] " + msg);
    }
    for (const auto& msg : info) {
        allMessages.push_back("[INFO] " + msg);
    }
    
    return allMessages;
}

size_t ValidationResult::GetMessageCount(ValidationSeverity severity) const {
    switch (severity) {
        case ValidationSeverity::Error:
        case ValidationSeverity::Critical:
            return errors.size();
        case ValidationSeverity::Warning:
            return warnings.size();
        case ValidationSeverity::Info:
            return info.size();
        default:
            return 0;
    }
}

// ============================================================================
// ValidationContext Implementation
// ============================================================================

ValidationContext::ValidationContext() = default;

void ValidationContext::BeginScope(const std::string& scopeName) {
    m_ScopeStack.push_back(scopeName);
}

void ValidationContext::EndScope() {
    if (!m_ScopeStack.empty()) {
        m_ScopeStack.pop_back();
    }
}

std::string ValidationContext::GetCurrentScope() const {
    if (m_ScopeStack.empty()) {
        return "";
    }
    
    std::string scope;
    for (size_t i = 0; i < m_ScopeStack.size(); ++i) {
        if (i > 0) {
            scope += ".";
        }
        scope += m_ScopeStack[i];
    }
    return scope;
}

void ValidationContext::SetErrorCallback(std::function<void(ValidationSeverity, const std::string&)> callback) {
    m_ErrorCallback = callback;
}

void ValidationContext::Report(ValidationSeverity severity, const std::string& message) {
    std::string fullMessage = message;
    std::string scope = GetCurrentScope();
    if (!scope.empty()) {
        fullMessage = "[" + scope + "] " + message;
    }
    
    m_Messages.emplace_back(severity, fullMessage);
    
    if (m_ErrorCallback) {
        m_ErrorCallback(severity, fullMessage);
    }
}

const std::vector<std::pair<ValidationSeverity, std::string>>& ValidationContext::GetMessages() const {
    return m_Messages;
}

void ValidationContext::ClearMessages() {
    m_Messages.clear();
}

bool ValidationContext::HasErrors() const {
    for (const auto& [severity, message] : m_Messages) {
        if (severity == ValidationSeverity::Error || severity == ValidationSeverity::Critical) {
            return true;
        }
    }
    return false;
}

bool ValidationContext::HasWarnings() const {
    for (const auto& [severity, message] : m_Messages) {
        if (severity == ValidationSeverity::Warning) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// FontSystemValidator Implementation
// ============================================================================

ValidationResult FontSystemValidator::ValidateFontFile(const std::string& fontPath,
                                                         ValidationContext& context) {
    context.BeginScope("FontFile");
    ValidationResult result;
    
    // Check if file exists
    if (!std::filesystem::exists(fontPath)) {
        result.AddError("Font file does not exist: " + fontPath);
        context.Report(ValidationSeverity::Critical, "Font file does not exist: " + fontPath);
        context.EndScope();
        return result;
    }
    
    // Check file size
    size_t fileSize = ValidationUtils::GetFileSize(fontPath);
    if (fileSize == 0) {
        result.AddError("Font file is empty: " + fontPath);
        context.Report(ValidationSeverity::Critical, "Font file is empty: " + fontPath);
    } else if (!ValidationUtils::IsFileSizeValid(fontPath)) {
        result.AddError("Font file is too large: " + fontPath + " (" + std::to_string(fileSize) + " bytes)");
        context.Report(ValidationSeverity::Error, "Font file is too large: " + fontPath);
    }
    
    // Validate with FreeType
    std::vector<std::string> fontErrors;
    if (!FontValidator::Validate(fontPath, fontErrors)) {
        for (const auto& error : fontErrors) {
            result.AddError(error);
            context.Report(ValidationSeverity::Error, error);
        }
    }
    
    // Check file extension
    std::filesystem::path path(fontPath);
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](int c) { return static_cast<char>(::tolower(c)); });
    
    const std::vector<std::string> validExtensions = {".ttf", ".otf", ".woff", ".woff2", ".ttc"};
    bool validExtension = false;
    for (const auto& validExt : validExtensions) {
        if (ext == validExt) {
            validExtension = true;
            break;
        }
    }
    
    if (!validExtension) {
        result.AddWarning("Unusual font file extension: " + ext);
        context.Report(ValidationSeverity::Warning, "Unusual font file extension: " + ext);
    }
    
    context.EndScope();
    return result;
}

ValidationResult FontSystemValidator::ValidateUnicodeText(std::string_view text,
                                                             ValidationContext& context) {
    context.BeginScope("UnicodeText");
    ValidationResult result;
    
    if (text.empty()) {
        result.AddInfo("Empty text string");
        context.EndScope();
        return result;
    }
    
    // Validate UTF-8 encoding
    if (!ValidationUtils::IsValidUtf8(text)) {
        result.AddError("Invalid UTF-8 encoding in text");
        context.Report(ValidationSeverity::Error, "Invalid UTF-8 encoding in text");
    }
    
    // Check for invalid codepoints
    std::vector<uint32_t> codepoints;
    UnicodeDecoder decoder;
    decoder.DecodeUtf8(text, codepoints);
    
    size_t invalidCount = 0;
    for (uint32_t cp : codepoints) {
        if (!IUnicodeDecoder::IsValidCodepoint(cp)) {
            invalidCount++;
        }
    }
    
    if (invalidCount > 0) {
        result.AddError("Found " + std::to_string(invalidCount) + " invalid Unicode codepoints");
        context.Report(ValidationSeverity::Error, "Found invalid Unicode codepoints");
    }
    
    // Check for surrogate pairs in UTF-8 (invalid)
    for (uint32_t cp : codepoints) {
        if (cp >= 0xD800 && cp <= 0xDFFF) {
            result.AddError("Invalid surrogate pair in UTF-8 text");
            context.Report(ValidationSeverity::Error, "Invalid surrogate pair in UTF-8 text");
            break;
        }
    }
    
    context.EndScope();
    return result;
}

// Removed ValidateGlyphMetrics - simplified validation

ValidationResult FontSystemValidator::ValidateAtlasData(const std::vector<uint8_t>& pixels,
                                                           int width, int height,
                                                           ValidationContext& context) {
    context.BeginScope("AtlasData");
    ValidationResult result;
    
    // Check dimensions
    if (width <= 0 || height <= 0) {
        result.AddError("Invalid atlas dimensions: " + std::to_string(width) + "x" + std::to_string(height));
        context.Report(ValidationSeverity::Critical, "Invalid atlas dimensions");
        context.EndScope();
        return result;
    }
    
    if (width > 16384 || height > 16384) {
        result.AddError("Atlas dimensions too large: " + std::to_string(width) + "x" + std::to_string(height));
        context.Report(ValidationSeverity::Error, "Atlas dimensions too large");
    }
    
    // Check pixel data size
    size_t expectedSize = static_cast<size_t>(width * height * 4); // RGBA8
    if (pixels.size() != expectedSize) {
        result.AddError("Atlas pixel data size mismatch: expected " + std::to_string(expectedSize) + 
                      ", got " + std::to_string(pixels.size()));
        context.Report(ValidationSeverity::Error, "Atlas pixel data size mismatch");
    }
    
    // Check for valid pixel values (all should be 0-255)
    for (size_t i = 0; i < pixels.size(); ++i) {
        // RGBA8 format, all values valid by definition
    }
    
    // Check power-of-two dimensions (recommended for MSDF)
    bool isPowerOfTwo = (width & (width - 1)) == 0 && (height & (height - 1)) == 0;
    if (!isPowerOfTwo) {
        result.AddWarning("Atlas dimensions are not power-of-two: " + std::to_string(width) + "x" + std::to_string(height));
        context.Report(ValidationSeverity::Warning, "Atlas dimensions are not power-of-two");
    }
    
    context.EndScope();
    return result;
}

// Removed ValidateVertexData, ValidateBatchData, ValidateShader, ValidateConfig, ValidateFontSystem, ValidateStartup

// ============================================================================
// ValidationUtils Implementation
// ============================================================================

namespace ValidationUtils {

bool IsFinite(float value) {
    return std::isfinite(value);
}

bool IsFinite(double value) {
    return std::isfinite(value);
}

bool IsValidUtf8(std::string_view text) {
    UnicodeDecoder decoder;
    return decoder.ValidateUtf8(text);
}

bool IsReadableFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    return file.good();
}

bool IsWritableDirectory(const std::string& path) {
    std::filesystem::path dirPath(path);
    if (!std::filesystem::exists(dirPath)) {
        return false;
    }
    if (!std::filesystem::is_directory(dirPath)) {
        return false;
    }
    
    // Try to create a test file
    std::string testFile = path + "/.write_test";
    std::ofstream test(testFile);
    if (!test.good()) {
        return false;
    }
    test.close();
    std::filesystem::remove(testFile);
    
    return true;
}

size_t GetFileSize(const std::string& path) {
    std::error_code ec;
    auto size = std::filesystem::file_size(path, ec);
    if (ec) {
        return 0;
    }
    return static_cast<size_t>(size);
}

bool IsFileSizeValid(const std::string& path, size_t maxSize) {
    size_t size = GetFileSize(path);
    return size > 0 && size <= maxSize;
}

bool IsAllocationSizeValid(size_t size, size_t maxSize) {
    return size > 0 && size <= maxSize;
}

std::string FormatMessage(const std::string& scope, const std::string& message) {
    if (scope.empty()) {
        return message;
    }
    return "[" + scope + "] " + message;
}

ValidationResult FromBool(bool success, const std::string& errorMessage) {
    ValidationResult result;
    if (!success) {
        result.AddError(errorMessage);
    }
    return result;
}

ValidationResult CombineResults(const std::vector<ValidationResult>& results) {
    ValidationResult combined;
    
    for (const auto& result : results) {
        if (!result.isValid) {
            combined.isValid = false;
        }
        combined.errors.insert(combined.errors.end(), result.errors.begin(), result.errors.end());
        combined.warnings.insert(combined.warnings.end(), result.warnings.begin(), result.warnings.end());
        combined.info.insert(combined.info.end(), result.info.begin(), result.info.end());
    }
    
    return combined;
}

} // namespace ValidationUtils

// ============================================================================
// StartupValidator Implementation
// ============================================================================

ValidationResult StartupValidator::ValidateAll(const TextRenderConfig& config) {
    ValidationContext context;
    ValidationResult result;
    
    context.BeginScope("StartupValidation");
    
    // Validate FreeType
    ValidationResult freeTypeResult = ValidateFreeType();
    if (!freeTypeResult.isValid) {
        result.AddError("FreeType validation failed");
        for (const auto& msg : freeTypeResult.errors) {
            result.AddError("FreeType: " + msg);
        }
    }
    
    // Validate msdf-atlas-gen
    ValidationResult msdfResult = ValidateMsdfGen();
    if (!msdfResult.isValid) {
        result.AddError("msdf-atlas-gen validation failed");
        for (const auto& msg : msdfResult.errors) {
            result.AddError("msdf-atlas-gen: " + msg);
        }
    }
    
    // Validate graphics API
    ValidationResult apiResult = ValidateGraphicsApi(config.graphicsApi);
    if (!apiResult.isValid) {
        result.AddError("Graphics API validation failed");
        for (const auto& msg : apiResult.errors) {
            result.AddError("Graphics API: " + msg);
        }
    }
    
    context.EndScope();
    return result;
}

ValidationResult StartupValidator::ValidateFreeType() {
    ValidationResult result;
    
    // Try to initialize FreeType
    FT_Library library = nullptr;
    FT_Error error = FT_Init_FreeType(&library);
    
    if (error != 0) {
        result.AddError("Failed to initialize FreeType library (error " + std::to_string(error) + ")");
    } else {
        result.AddInfo("FreeType library initialized successfully");
        FT_Done_FreeType(library);
    }
    
    return result;
}

ValidationResult StartupValidator::ValidateMsdfGen() {
    ValidationResult result;
    
    // Try to initialize msdfgen
    msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
    
    if (!ft) {
        result.AddError("Failed to initialize msdfgen FreeType handle");
    } else {
        result.AddInfo("msdfgen initialized successfully");
        msdfgen::deinitializeFreetype(ft);
    }
    
    return result;
}

ValidationResult StartupValidator::ValidateGraphicsApi(GraphicsApi api) {
    ValidationResult result;
    
    if (!RenderingBackendFactory::IsApiSupported(api)) {
        result.AddError("Graphics API not supported on this platform");
        return result;
    }
    
    result.AddInfo("Graphics API is supported");
    return result;
}

// Removed ValidateSystemFonts, ValidateFileSystem, ValidateMemory, ValidateGpuCapabilities

} // namespace we::UI::Text
