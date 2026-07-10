#pragma once

#include "Application/Export.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <system_error>

namespace we::UI::Text {

/**
 * @brief Unicode encoding formats
 */
enum class UnicodeEncoding : uint8_t {
    UTF8,
    UTF16LE,
    UTF16BE,
    UTF32LE,
    UTF32BE
};

/**
 * @brief Result of Unicode decoding operation
 */
struct DecodeResult {
    std::vector<uint32_t> codepoints;
    size_t bytesConsumed = 0;
    bool hasErrors = false;
    size_t errorCount = 0;
    std::string errorMessage;
};

/**
 * @brief Unicode decoder interface
 * 
 * Provides decoding of UTF-8, UTF-16, and UTF-32 encoded text to Unicode codepoints.
 * Includes validation, error recovery, and detailed error reporting.
 */
class APPLICATION_API IUnicodeDecoder {
public:
    virtual ~IUnicodeDecoder() = default;

    /**
     * @brief Decode UTF-8 text to Unicode codepoints
     * @param text UTF-8 encoded text
     * @param outCodepoints Output vector for decoded codepoints
     * @return true if decoding succeeded, false if errors occurred
     */
    virtual bool DecodeUtf8(std::string_view text, std::vector<uint32_t>& outCodepoints) const = 0;

    /**
     * @brief Decode UTF-16 text to Unicode codepoints
     * @param data UTF-16 encoded data (little-endian)
     * @param length Number of UTF-16 code units
     * @param outCodepoints Output vector for decoded codepoints
     * @return true if decoding succeeded, false if errors occurred
     */
    virtual bool DecodeUtf16(const uint16_t* data, size_t length, std::vector<uint32_t>& outCodepoints) const = 0;

    /**
     * @brief Decode UTF-32 text to Unicode codepoints
     * @param data UTF-32 encoded data (little-endian)
     * @param length Number of UTF-32 code units
     * @param outCodepoints Output vector for decoded codepoints
     * @return true if decoding succeeded, false if errors occurred
     */
    virtual bool DecodeUtf32(const uint32_t* data, size_t length, std::vector<uint32_t>& outCodepoints) const = 0;

    /**
     * @brief Decode text with automatic encoding detection
     * @param data Raw text data
     * @param byteLength Length in bytes
     * @param outCodepoints Output vector for decoded codepoints
     * @return DecodeResult with detailed information
     */
    virtual DecodeResult DecodeAuto(const char* data, size_t byteLength) const = 0;

    /**
     * @brief Validate UTF-8 encoded text
     * @param text UTF-8 encoded text
     * @return true if valid UTF-8, false otherwise
     */
    virtual bool ValidateUtf8(std::string_view text) const = 0;

    /**
     * @brief Get the replacement character for invalid sequences
     * @return Unicode replacement character (U+FFFD)
     */
    static constexpr uint32_t ReplacementCharacter() { return 0xFFFD; }

    /**
     * @brief Check if a codepoint is valid Unicode
     * @param codepoint Unicode codepoint
     * @return true if valid, false otherwise
     */
    static bool IsValidCodepoint(uint32_t codepoint);

    /**
     * @brief Get the number of bytes needed to encode a codepoint in UTF-8
     * @param codepoint Unicode codepoint
     * @return Number of bytes (1-4), or 0 if invalid
     */
    static size_t GetUtf8ByteCount(uint32_t codepoint);

    /**
     * @brief Check if a codepoint requires surrogate pair in UTF-16
     * @param codepoint Unicode codepoint
     * @return true if surrogate pair needed, false otherwise
     */
    static bool RequiresSurrogatePair(uint32_t codepoint);

    /**
     * @brief Split a codepoint into UTF-16 surrogate pair
     * @param codepoint Unicode codepoint (must be > 0xFFFF)
     * @param highSurrogate Output high surrogate
     * @param lowSurrogate Output low surrogate
     * @return true if successful, false if invalid codepoint
     */
    static bool ToSurrogatePair(uint32_t codepoint, uint16_t& highSurrogate, uint16_t& lowSurrogate);

    /**
     * @brief Combine UTF-16 surrogate pair into codepoint
     * @param highSurrogate High surrogate
     * @param lowSurrogate Low surrogate
     * @return Combined codepoint, or ReplacementCharacter if invalid
     */
    static uint32_t FromSurrogatePair(uint16_t highSurrogate, uint16_t lowSurrogate);
};

/**
 * @brief Standard Unicode decoder implementation
 */
class APPLICATION_API UnicodeDecoder : public IUnicodeDecoder {
public:
    UnicodeDecoder() = default;
    ~UnicodeDecoder() override = default;

    bool DecodeUtf8(std::string_view text, std::vector<uint32_t>& outCodepoints) const override;
    bool DecodeUtf16(const uint16_t* data, size_t length, std::vector<uint32_t>& outCodepoints) const override;
    bool DecodeUtf32(const uint32_t* data, size_t length, std::vector<uint32_t>& outCodepoints) const override;
    DecodeResult DecodeAuto(const char* data, size_t byteLength) const override;
    bool ValidateUtf8(std::string_view text) const override;

    /**
     * @brief Decode single UTF-8 character
     * @param bytes UTF-8 byte sequence
     * @param maxBytes Maximum bytes available (1-4)
     * @param outCodepoint Output codepoint
     * @param outBytesConsumed Output bytes consumed
     * @return true if successful, false if invalid
     */
    static bool DecodeUtf8Char(const unsigned char* bytes, size_t maxBytes, 
                                uint32_t& outCodepoint, size_t& outBytesConsumed);

    /**
     * @brief Detect encoding from BOM (Byte Order Mark)
     * @param data Raw data
     * @param byteLength Length in bytes
     * @return Detected encoding, or UTF8 if no BOM
     */
    static UnicodeEncoding DetectEncoding(const char* data, size_t byteLength);
};

/**
 * @brief UTF-8 iterator for efficient codepoint iteration
 */
class Utf8Iterator {
public:
    explicit Utf8Iterator(std::string_view text);
    
    /**
     * @brief Advance to next codepoint
     * @return Current codepoint, or 0 if at end
     */
    uint32_t Next();

    /**
     * @brief Check if more codepoints available
     * @return true if more available, false otherwise
     */
    bool HasNext() const { return m_Position < m_Text.size(); }

    /**
     * @brief Reset iterator to beginning
     */
    void Reset() { m_Position = 0; }

    /**
     * @brief Get current byte position
     * @return Current byte position in text
     */
    size_t GetPosition() const { return m_Position; }

private:
    std::string_view m_Text;
    size_t m_Position = 0;
};

/**
 * @brief UTF-16 iterator for efficient codepoint iteration
 */
class Utf16Iterator {
public:
    Utf16Iterator(const uint16_t* data, size_t length, bool isBigEndian = false);

    /**
     * @brief Advance to next codepoint
     * @return Current codepoint, or 0 if at end
     */
    uint32_t Next();

    /**
     * @brief Check if more codepoints available
     * @return true if more available, false otherwise
     */
    bool HasNext() const { return m_Position < m_Length; }

    /**
     * @brief Reset iterator to beginning
     */
    void Reset() { m_Position = 0; }

private:
    const uint16_t* m_Data;
    size_t m_Length;
    size_t m_Position = 0;
    bool m_IsBigEndian;
};

/**
 * @brief Unicode utility functions
 */
namespace UnicodeUtils {
    /**
     * @brief Check if codepoint is ASCII (0-127)
     */
    inline bool IsAscii(uint32_t cp) { return cp <= 0x7F; }

    /**
     * @brief Check if codepoint is Latin-1 supplement (128-255)
     */
    inline bool IsLatin1Supplement(uint32_t cp) { return cp >= 0x80 && cp <= 0xFF; }

    /**
     * @brief Check if codepoint is in Basic Multilingual Plane (0-65535)
     */
    inline bool IsBMP(uint32_t cp) { return cp <= 0xFFFF; }

    /**
     * @brief Check if codepoint is a control character
     */
    bool IsControl(uint32_t cp);

    /**
     * @brief Check if codepoint is whitespace
     */
    bool IsWhitespace(uint32_t cp);

    /**
     * @brief Check if codepoint is a digit
     */
    bool IsDigit(uint32_t cp);

    /**
     * @brief Check if codepoint is a letter
     */
    bool IsLetter(uint32_t cp);

    /**
     * @brief Check if codepoint is uppercase
     */
    bool IsUppercase(uint32_t cp);

    /**
     * @brief Check if codepoint is lowercase
     */
    bool IsLowercase(uint32_t cp);

    /**
     * @brief Convert codepoint to uppercase
     */
    uint32_t ToUppercase(uint32_t cp);

    /**
     * @brief Convert codepoint to lowercase
     */
    uint32_t ToLowercase(uint32_t cp);

    /**
     * @brief Get script category for codepoint (e.g., Latin, Arabic, CJK)
     */
    enum class ScriptCategory {
        Unknown,
        Latin,
        Greek,
        Cyrillic,
        Armenian,
        Hebrew,
        Arabic,
        Syriac,
        Thaana,
        Devanagari,
        Bengali,
        Gurmukhi,
        Gujarati,
        Oriya,
        Tamil,
        Telugu,
        Kannada,
        Malayalam,
        Sinhala,
        Thai,
        Lao,
        Tibetan,
        Myanmar,
        Georgian,
        Hangul,
        Ethiopic,
        Cherokee,
        Canadian,
        Ogham,
        Runic,
        Khmer,
        Mongolian,
        Hiragana,
        Katakana,
        Bopomofo,
        Han,
        Yi,
        OldItalic,
        Gothic,
        Deseret,
        Hanunoo,
        Tagalog,
        Tagbanwa,
        Buhid,
        Limbu,
        TaiLe,
        NewTaiLue,
        Buginese,
        CJK,
        Emoji
    };

    ScriptCategory GetScriptCategory(uint32_t cp);

    /**
     * @brief Check if codepoint is right-to-left
     */
    bool IsRightToLeft(uint32_t cp);

    /**
     * @brief Check if codepoint is a combining mark
     */
    bool IsCombiningMark(uint32_t cp);

    /**
     * @brief Check if codepoint is a variation selector
     */
    bool IsVariationSelector(uint32_t cp);

    /**
     * @brief Get display width of codepoint (for monospace fonts)
     */
    int GetDisplayWidth(uint32_t cp);

    /**
     * @brief Normalize codepoint (NFC form)
     */
    uint32_t Normalize(uint32_t cp);
};

} // namespace we::UI::Text
