#pragma once

#include "Application/Export.h"

#include "Rendering/Text/GlyphManager.h"
#include "Rendering/Text/GpuBatcher.h"
#include "Rendering/Text/FontSystem.h"
#include "Rendering/Text/RenderingBackend.h"

#include <string>
#include <vector>
#include <functional>
#include <system_error>

namespace we::UI::Text {

/**
 * @brief Validation severity level
 */
enum class ValidationSeverity : uint8_t {
    Info,
    Warning,
    Error,
    Critical
};

/**
 * @brief Validation result
 */
struct ValidationResult {
    bool isValid = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::vector<std::string> info;
    
    /**
     * @brief Add an error message
     */
    void AddError(const std::string& message) {
        errors.push_back(message);
        isValid = false;
    }
    
    /**
     * @brief Add a warning message
     */
    void AddWarning(const std::string& message) {
        warnings.push_back(message);
    }
    
    /**
     * @brief Add an info message
     */
    void AddInfo(const std::string& message) {
        info.push_back(message);
    }
    
    /**
     * @brief Get all messages
     */
    std::vector<std::string> GetAllMessages() const;
    
    /**
     * @brief Get message count by severity
     */
    size_t GetMessageCount(ValidationSeverity severity) const;
};

/**
 * @brief Validation context for tracking validation state
 */
class ValidationContext {
public:
    ValidationContext();
    
    /**
     * @brief Begin a validation scope
     * @param scopeName Name of the validation scope
     */
    void BeginScope(const std::string& scopeName);
    
    /**
     * @brief End the current validation scope
     */
    void EndScope();
    
    /**
     * @brief Get current scope path
     */
    std::string GetCurrentScope() const;
    
    /**
     * @brief Set custom error callback
     */
    void SetErrorCallback(std::function<void(ValidationSeverity, const std::string&)> callback);
    
    /**
     * @brief Report a validation message
     */
    void Report(ValidationSeverity severity, const std::string& message);
    
    /**
     * @brief Get all reported messages
     */
    const std::vector<std::pair<ValidationSeverity, std::string>>& GetMessages() const;
    
    /**
     * @brief Clear all messages
     */
    void ClearMessages();
    
    /**
     * @brief Check if any errors were reported
     */
    bool HasErrors() const;
    
    /**
     * @brief Check if any warnings were reported
     */
    bool HasWarnings() const;
    
private:
    std::vector<std::string> m_ScopeStack;
    std::function<void(ValidationSeverity, const std::string&)> m_ErrorCallback;
    std::vector<std::pair<ValidationSeverity, std::string>> m_Messages;
};

/**
 * @brief Font system validator
 * 
 * Provides comprehensive validation for all font system components.
 * Fails gracefully with detailed error messages rather than silent failures.
 */
class APPLICATION_API FontSystemValidator {
public:
    /**
     * @brief Validate font file
     * @param fontPath Path to font file
     * @param context Validation context
     * @return ValidationResult
     */
    static ValidationResult ValidateFontFile(const std::string& fontPath, 
                                              ValidationContext& context);
    
    /**
     * @brief Validate Unicode text
     * @param text UTF-8 text to validate
     * @param context Validation context
     * @return ValidationResult
     */
    static ValidationResult ValidateUnicodeText(std::string_view text,
                                                 ValidationContext& context);
    
    /**
     * @brief Validate atlas data
     * @param pixels Atlas pixel data
     * @param width Atlas width
     * @param height Atlas height
     * @param context Validation context
     * @return ValidationResult
     */
    static ValidationResult ValidateAtlasData(const std::vector<uint8_t>& pixels,
                                              int width, int height,
                                              ValidationContext& context);
};

/**
 * @brief Validation utility functions
 */
namespace ValidationUtils {
    /**
     * @brief Check if a value is within range
     */
    template<typename T>
    bool IsInRange(T value, T min, T max) {
        return value >= min && value <= max;
    }
    
    /**
     * @brief Check if a value is finite
     */
    bool IsFinite(float value);
    bool IsFinite(double value);
    
    /**
     * @brief Check if a string is valid UTF-8
     */
    bool IsValidUtf8(std::string_view text);
    
    /**
     * @brief Check if a file exists and is readable
     */
    bool IsReadableFile(const std::string& path);
    
    /**
     * @brief Check if a directory exists and is writable
     */
    bool IsWritableDirectory(const std::string& path);
    
    /**
     * @brief Get file size in bytes
     */
    size_t GetFileSize(const std::string& path);
    
    /**
     * @brief Check if file size is within reasonable limits
     */
    bool IsFileSizeValid(const std::string& path, size_t maxSize = 100 * 1024 * 1024);
    
    /**
     * @brief Validate memory allocation size
     */
    bool IsAllocationSizeValid(size_t size, size_t maxSize = 1 * 1024 * 1024 * 1024);
    
    /**
     * @brief Format validation message with scope
     */
    std::string FormatMessage(const std::string& scope, const std::string& message);
    
    /**
     * @brief Create a validation result from a boolean
     */
    ValidationResult FromBool(bool success, const std::string& errorMessage = "");
    
    /**
     * @brief Combine multiple validation results
     */
    ValidationResult CombineResults(const std::vector<ValidationResult>& results);
}

/**
 * @brief Startup validator for strict startup validation
 */
class APPLICATION_API StartupValidator {
public:
    /**
     * @brief Validate all startup requirements
     * @param config Rendering configuration
     * @return ValidationResult with detailed information
     */
    static ValidationResult ValidateAll(const TextRenderConfig& config);
    
    /**
     * @brief Validate FreeType library availability
     */
    static ValidationResult ValidateFreeType();
    
    /**
     * @brief Validate msdf-atlas-gen library availability
     */
    static ValidationResult ValidateMsdfGen();
    
    /**
     * @brief Validate graphics API availability
     */
    static ValidationResult ValidateGraphicsApi(GraphicsApi api);
    
    /**
     * @brief Validate system font availability
     */
    static ValidationResult ValidateSystemFonts();
    
    /**
     * @brief Validate file system permissions
     */
    static ValidationResult ValidateFileSystem();
    
    /**
     * @brief Validate memory availability
     */
    static ValidationResult ValidateMemory();
    
    /**
     * @brief Validate GPU capabilities
     */
    static ValidationResult ValidateGpuCapabilities();
};

} // namespace we::UI::Text
