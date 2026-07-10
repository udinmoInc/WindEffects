#include "Rendering/Text/UnicodeDecoder.h"
#include "Core/Logger.h"

#include <algorithm>
#include <cstring>

namespace we::UI::Text {

// ============================================================================
// IUnicodeDecoder Static Methods
// ============================================================================

bool IUnicodeDecoder::IsValidCodepoint(uint32_t codepoint) {
    // Unicode codepoints range from 0 to 0x10FFFF
    // Surrogate code points (U+D800 to U+DFFF) are invalid
    if (codepoint > 0x10FFFF) {
        return false;
    }
    if (codepoint >= 0xD800 && codepoint <= 0xDFFF) {
        return false;
    }
    return true;
}

size_t IUnicodeDecoder::GetUtf8ByteCount(uint32_t codepoint) {
    if (!IsValidCodepoint(codepoint)) {
        return 0;
    }
    if (codepoint <= 0x7F) {
        return 1;
    } else if (codepoint <= 0x7FF) {
        return 2;
    } else if (codepoint <= 0xFFFF) {
        return 3;
    } else {
        return 4;
    }
}

bool IUnicodeDecoder::RequiresSurrogatePair(uint32_t codepoint) {
    return codepoint > 0xFFFF && IsValidCodepoint(codepoint);
}

bool IUnicodeDecoder::ToSurrogatePair(uint32_t codepoint, uint16_t& highSurrogate, uint16_t& lowSurrogate) {
    if (!RequiresSurrogatePair(codepoint)) {
        return false;
    }
    
    codepoint -= 0x10000;
    highSurrogate = static_cast<uint16_t>((codepoint >> 10) + 0xD800);
    lowSurrogate = static_cast<uint16_t>((codepoint & 0x3FF) + 0xDC00);
    return true;
}

uint32_t IUnicodeDecoder::FromSurrogatePair(uint16_t highSurrogate, uint16_t lowSurrogate) {
    if (highSurrogate < 0xD800 || highSurrogate > 0xDBFF) {
        return ReplacementCharacter();
    }
    if (lowSurrogate < 0xDC00 || lowSurrogate > 0xDFFF) {
        return ReplacementCharacter();
    }
    
    uint32_t codepoint = ((highSurrogate - 0xD800) << 10) + (lowSurrogate - 0xDC00) + 0x10000;
    return IsValidCodepoint(codepoint) ? codepoint : ReplacementCharacter();
}

// ============================================================================
// UnicodeDecoder Implementation
// ============================================================================

bool UnicodeDecoder::DecodeUtf8(std::string_view text, std::vector<uint32_t>& outCodepoints) const {
    outCodepoints.clear();
    outCodepoints.reserve(text.size()); // Reserve approximate space
    
    const auto* bytes = reinterpret_cast<const unsigned char*>(text.data());
    size_t i = 0;
    
    while (i < text.size()) {
        uint32_t codepoint = 0;
        size_t bytesConsumed = 0;
        
        if (!DecodeUtf8Char(bytes + i, text.size() - i, codepoint, bytesConsumed)) {
            outCodepoints.push_back(ReplacementCharacter());
            i += 1; // Skip invalid byte
            continue;
        }
        
        outCodepoints.push_back(codepoint);
        i += bytesConsumed;
    }
    
    return true;
}

bool UnicodeDecoder::DecodeUtf8Char(const unsigned char* bytes, size_t maxBytes, 
                                     uint32_t& outCodepoint, size_t& outBytesConsumed) {
    if (maxBytes == 0) {
        return false;
    }
    
    const unsigned char c = bytes[0];
    
    // 1-byte sequence (0xxxxxxx)
    if (c <= 0x7F) {
        outCodepoint = c;
        outBytesConsumed = 1;
        return true;
    }
    
    // 2-byte sequence (110xxxxx 10xxxxxx)
    if ((c & 0xE0) == 0xC0) {
        if (maxBytes < 2) {
            return false;
        }
        if ((bytes[1] & 0xC0) != 0x80) {
            return false;
        }
        
        outCodepoint = ((c & 0x1F) << 6) | (bytes[1] & 0x3F);
        outBytesConsumed = 2;
        
        // Overlong encoding check
        if (outCodepoint < 0x80) {
            return false;
        }
        return IsValidCodepoint(outCodepoint);
    }
    
    // 3-byte sequence (1110xxxx 10xxxxxx 10xxxxxx)
    if ((c & 0xF0) == 0xE0) {
        if (maxBytes < 3) {
            return false;
        }
        if ((bytes[1] & 0xC0) != 0x80 || (bytes[2] & 0xC0) != 0x80) {
            return false;
        }
        
        outCodepoint = ((c & 0x0F) << 12) | ((bytes[1] & 0x3F) << 6) | (bytes[2] & 0x3F);
        outBytesConsumed = 3;
        
        // Overlong encoding check
        if (outCodepoint < 0x800) {
            return false;
        }
        return IsValidCodepoint(outCodepoint);
    }
    
    // 4-byte sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
    if ((c & 0xF8) == 0xF0) {
        if (maxBytes < 4) {
            return false;
        }
        if ((bytes[1] & 0xC0) != 0x80 || (bytes[2] & 0xC0) != 0x80 || (bytes[3] & 0xC0) != 0x80) {
            return false;
        }
        
        outCodepoint = ((c & 0x07) << 18) | ((bytes[1] & 0x3F) << 12) | 
                       ((bytes[2] & 0x3F) << 6) | (bytes[3] & 0x3F);
        outBytesConsumed = 4;
        
        // Overlong encoding check
        if (outCodepoint < 0x10000) {
            return false;
        }
        return IsValidCodepoint(outCodepoint);
    }
    
    // Invalid UTF-8 start byte
    return false;
}

bool UnicodeDecoder::DecodeUtf16(const uint16_t* data, size_t length, std::vector<uint32_t>& outCodepoints) const {
    outCodepoints.clear();
    outCodepoints.reserve(length);
    
    for (size_t i = 0; i < length; ) {
        uint16_t unit = data[i];
        
        // Check for high surrogate
        if (unit >= 0xD800 && unit <= 0xDBFF) {
            if (i + 1 >= length) {
                outCodepoints.push_back(ReplacementCharacter());
                i += 1;
                continue;
            }
            
            uint16_t nextUnit = data[i + 1];
            if (nextUnit >= 0xDC00 && nextUnit <= 0xDFFF) {
                // Valid surrogate pair
                uint32_t codepoint = FromSurrogatePair(unit, nextUnit);
                outCodepoints.push_back(codepoint);
                i += 2;
            } else {
                // Unpaired high surrogate
                outCodepoints.push_back(ReplacementCharacter());
                i += 1;
            }
        } 
        // Check for low surrogate (unpaired)
        else if (unit >= 0xDC00 && unit <= 0xDFFF) {
            outCodepoints.push_back(ReplacementCharacter());
            i += 1;
        }
        // BMP character
        else {
            outCodepoints.push_back(unit);
            i += 1;
        }
    }
    
    return true;
}

bool UnicodeDecoder::DecodeUtf32(const uint32_t* data, size_t length, std::vector<uint32_t>& outCodepoints) const {
    outCodepoints.clear();
    outCodepoints.reserve(length);
    
    for (size_t i = 0; i < length; ++i) {
        uint32_t codepoint = data[i];
        if (IsValidCodepoint(codepoint)) {
            outCodepoints.push_back(codepoint);
        } else {
            outCodepoints.push_back(ReplacementCharacter());
        }
    }
    
    return true;
}

UnicodeEncoding UnicodeDecoder::DetectEncoding(const char* data, size_t byteLength) {
    if (byteLength < 2) {
        return UnicodeEncoding::UTF8;
    }
    
    const auto* bytes = reinterpret_cast<const unsigned char*>(data);
    
    // UTF-8 BOM: EF BB BF
    if (byteLength >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
        return UnicodeEncoding::UTF8;
    }
    
    // UTF-16 LE BOM: FF FE
    if (bytes[0] == 0xFF && bytes[1] == 0xFE) {
        return UnicodeEncoding::UTF16LE;
    }
    
    // UTF-16 BE BOM: FE FF
    if (bytes[0] == 0xFE && bytes[1] == 0xFF) {
        return UnicodeEncoding::UTF16BE;
    }
    
    // UTF-32 LE BOM: FF FE 00 00
    if (byteLength >= 4 && bytes[0] == 0xFF && bytes[1] == 0xFE && bytes[2] == 0x00 && bytes[3] == 0x00) {
        return UnicodeEncoding::UTF32LE;
    }
    
    // UTF-32 BE BOM: 00 00 FE FF
    if (byteLength >= 4 && bytes[0] == 0x00 && bytes[1] == 0x00 && bytes[2] == 0xFE && bytes[3] == 0xFF) {
        return UnicodeEncoding::UTF32BE;
    }
    
    // No BOM detected, assume UTF-8
    return UnicodeEncoding::UTF8;
}

DecodeResult UnicodeDecoder::DecodeAuto(const char* data, size_t byteLength) const {
    DecodeResult result;
    result.bytesConsumed = byteLength;
    
    if (byteLength == 0) {
        return result;
    }
    
    UnicodeEncoding encoding = DetectEncoding(data, byteLength);
    
    switch (encoding) {
        case UnicodeEncoding::UTF8:
            result.hasErrors = !DecodeUtf8(std::string_view(data, byteLength), result.codepoints);
            break;
            
        case UnicodeEncoding::UTF16LE: {
            const uint16_t* data16 = reinterpret_cast<const uint16_t*>(data);
            size_t length = byteLength / 2;
            result.hasErrors = !DecodeUtf16(data16, length, result.codepoints);
            break;
        }
            
        case UnicodeEncoding::UTF16BE: {
            const uint16_t* data16 = reinterpret_cast<const uint16_t*>(data);
            size_t length = byteLength / 2;
            
            // Convert BE to LE
            std::vector<uint16_t> data16LE(length);
            for (size_t i = 0; i < length; ++i) {
                data16LE[i] = ((data16[i] >> 8) & 0xFF) | ((data16[i] << 8) & 0xFF00);
            }
            
            result.hasErrors = !DecodeUtf16(data16LE.data(), length, result.codepoints);
            break;
        }
            
        case UnicodeEncoding::UTF32LE: {
            const uint32_t* data32 = reinterpret_cast<const uint32_t*>(data);
            size_t length = byteLength / 4;
            result.hasErrors = !DecodeUtf32(data32, length, result.codepoints);
            break;
        }
            
        case UnicodeEncoding::UTF32BE: {
            const uint32_t* data32 = reinterpret_cast<const uint32_t*>(data);
            size_t length = byteLength / 4;
            
            // Convert BE to LE
            std::vector<uint32_t> data32LE(length);
            for (size_t i = 0; i < length; ++i) {
                data32LE[i] = ((data32[i] >> 24) & 0xFF) | 
                              ((data32[i] >> 8) & 0xFF00) |
                              ((data32[i] << 8) & 0xFF0000) |
                              ((data32[i] << 24) & 0xFF000000);
            }
            
            result.hasErrors = !DecodeUtf32(data32LE.data(), length, result.codepoints);
            break;
        }
    }
    
    if (result.hasErrors) {
        result.errorMessage = "Unicode decoding errors detected";
        result.errorCount = std::count(result.codepoints.begin(), result.codepoints.end(), 
                                        ReplacementCharacter());
    }
    
    return result;
}

bool UnicodeDecoder::ValidateUtf8(std::string_view text) const {
    const auto* bytes = reinterpret_cast<const unsigned char*>(text.data());
    size_t i = 0;
    
    while (i < text.size()) {
        uint32_t codepoint = 0;
        size_t bytesConsumed = 0;
        
        if (!DecodeUtf8Char(bytes + i, text.size() - i, codepoint, bytesConsumed)) {
            return false;
        }
        
        i += bytesConsumed;
    }
    
    return true;
}

// ============================================================================
// Utf8Iterator Implementation
// ============================================================================

Utf8Iterator::Utf8Iterator(std::string_view text) : m_Text(text) {}

uint32_t Utf8Iterator::Next() {
    if (!HasNext()) {
        return 0;
    }
    
    const auto* bytes = reinterpret_cast<const unsigned char*>(m_Text.data());
    uint32_t codepoint = 0;
    size_t bytesConsumed = 0;
    
    if (!UnicodeDecoder::DecodeUtf8Char(bytes + m_Position, m_Text.size() - m_Position, 
                                         codepoint, bytesConsumed)) {
        m_Position += 1;
        return IUnicodeDecoder::ReplacementCharacter();
    }
    
    m_Position += bytesConsumed;
    return codepoint;
}

// ============================================================================
// Utf16Iterator Implementation
// ============================================================================

Utf16Iterator::Utf16Iterator(const uint16_t* data, size_t length, bool isBigEndian)
    : m_Data(data), m_Length(length), m_IsBigEndian(isBigEndian) {}

uint32_t Utf16Iterator::Next() {
    if (!HasNext()) {
        return 0;
    }
    
    uint16_t unit = m_Data[m_Position];
    
    // Convert from big-endian if needed
    if (m_IsBigEndian) {
        unit = ((unit >> 8) & 0xFF) | ((unit << 8) & 0xFF00);
    }
    
    // Check for high surrogate
    if (unit >= 0xD800 && unit <= 0xDBFF) {
        if (m_Position + 1 >= m_Length) {
            m_Position += 1;
            return IUnicodeDecoder::ReplacementCharacter();
        }
        
        uint16_t nextUnit = m_Data[m_Position + 1];
        if (m_IsBigEndian) {
            nextUnit = ((nextUnit >> 8) & 0xFF) | ((nextUnit << 8) & 0xFF00);
        }
        
        if (nextUnit >= 0xDC00 && nextUnit <= 0xDFFF) {
            // Combine surrogate pair
            uint32_t highSurrogate = unit - 0xD800;
            uint32_t lowSurrogate = nextUnit - 0xDC00;
            uint32_t codepoint = (highSurrogate << 10) + lowSurrogate + 0x10000;
            m_Position += 2;
            return codepoint;
        }
    }
    
    m_Position += 1;
    
    // Check for unpaired low surrogate
    if (unit >= 0xDC00 && unit <= 0xDFFF) {
        return IUnicodeDecoder::ReplacementCharacter();
    }
    
    return unit;
}

// ============================================================================
// UnicodeUtils Implementation
// ============================================================================

namespace UnicodeUtils {

bool IsControl(uint32_t cp) {
    // C0 controls (0-31, 127)
    if (cp <= 0x1F || cp == 0x7F) {
        return true;
    }
    // C1 controls (128-159)
    if (cp >= 0x80 && cp <= 0x9F) {
        return true;
    }
    // Other control characters
    if (cp == 0x2028 || cp == 0x2029) { // Line/paragraph separator
        return true;
    }
    return false;
}

bool IsWhitespace(uint32_t cp) {
    // Standard whitespace characters
    switch (cp) {
        case 0x0009: // TAB
        case 0x000A: // LINE FEED
        case 0x000B: // VERTICAL TAB
        case 0x000C: // FORM FEED
        case 0x000D: // CARRIAGE RETURN
        case 0x0020: // SPACE
        case 0x0085: // NEL
        case 0x00A0: // NO-BREAK SPACE
        case 0x1680: // OGHAM SPACE MARK
        case 0x2000: // EN QUAD
        case 0x2001: // EM QUAD
        case 0x2002: // EN SPACE
        case 0x2003: // EM SPACE
        case 0x2004: // THREE-PER-EM SPACE
        case 0x2005: // FOUR-PER-EM SPACE
        case 0x2006: // SIX-PER-EM SPACE
        case 0x2007: // FIGURE SPACE
        case 0x2008: // PUNCTUATION SPACE
        case 0x2009: // THIN SPACE
        case 0x200A: // HAIR SPACE
        case 0x2028: // LINE SEPARATOR
        case 0x2029: // PARAGRAPH SEPARATOR
        case 0x202F: // NARROW NO-BREAK SPACE
        case 0x205F: // MEDIUM MATHEMATICAL SPACE
        case 0x3000: // IDEOGRAPHIC SPACE
        case 0xFEFF: // ZERO WIDTH NO-BREAK SPACE
            return true;
        default:
            return false;
    }
}

bool IsDigit(uint32_t cp) {
    // ASCII digits
    if (cp >= 0x0030 && cp <= 0x0039) {
        return true;
    }
    // Other digit ranges (simplified)
    if (cp >= 0x0660 && cp <= 0x0669) { // Arabic-Indic
        return true;
    }
    if (cp >= 0x06F0 && cp <= 0x06F9) { // Persian
        return true;
    }
    if (cp >= 0x0966 && cp <= 0x096F) { // Devanagari
        return true;
    }
    if (cp >= 0xFF10 && cp <= 0xFF19) { // Fullwidth
        return true;
    }
    return false;
}

bool IsLetter(uint32_t cp) {
    // ASCII letters
    if ((cp >= 0x0041 && cp <= 0x005A) || (cp >= 0x0061 && cp <= 0x007A)) {
        return true;
    }
    // Latin-1 supplement
    if ((cp >= 0x00C0 && cp <= 0x00D6) || (cp >= 0x00D8 && cp <= 0x00F6) || (cp >= 0x00F8 && cp <= 0x00FF)) {
        return true;
    }
    // Latin extended (simplified)
    if (cp >= 0x0100 && cp <= 0x024F) {
        return true;
    }
    // Greek
    if ((cp >= 0x0370 && cp <= 0x03FF) || (cp >= 0x1F00 && cp <= 0x1FFF)) {
        return true;
    }
    // Cyrillic
    if (cp >= 0x0400 && cp <= 0x04FF) {
        return true;
    }
    // Arabic
    if (cp >= 0x0600 && cp <= 0x06FF) {
        return true;
    }
    // Hebrew
    if (cp >= 0x0590 && cp <= 0x05FF) {
        return true;
    }
    // CJK unified ideographs
    if (cp >= 0x4E00 && cp <= 0x9FFF) {
        return true;
    }
    // Hiragana
    if (cp >= 0x3040 && cp <= 0x309F) {
        return true;
    }
    // Katakana
    if (cp >= 0x30A0 && cp <= 0x30FF) {
        return true;
    }
    // Hangul
    if (cp >= 0xAC00 && cp <= 0xD7AF) {
        return true;
    }
    return false;
}

bool IsUppercase(uint32_t cp) {
    // ASCII uppercase
    if (cp >= 0x0041 && cp <= 0x005A) {
        return true;
    }
    // Latin-1 uppercase
    if ((cp >= 0x00C0 && cp <= 0x00D6) || (cp >= 0x00D8 && cp <= 0x00DE)) {
        return true;
    }
    // Greek uppercase
    if (cp >= 0x0391 && cp <= 0x03A1) {
        return true;
    }
    if (cp >= 0x03A3 && cp <= 0x03A9) {
        return true;
    }
    // Cyrillic uppercase
    if (cp >= 0x0410 && cp <= 0x042F) {
        return true;
    }
    return false;
}

bool IsLowercase(uint32_t cp) {
    // ASCII lowercase
    if (cp >= 0x0061 && cp <= 0x007A) {
        return true;
    }
    // Latin-1 lowercase
    if ((cp >= 0x00E0 && cp <= 0x00F6) || (cp >= 0x00F8 && cp <= 0x00FE)) {
        return true;
    }
    // Greek lowercase
    if (cp >= 0x03B1 && cp <= 0x03C9) {
        return true;
    }
    // Cyrillic lowercase
    if (cp >= 0x0430 && cp <= 0x044F) {
        return true;
    }
    return false;
}

uint32_t ToUppercase(uint32_t cp) {
    // ASCII
    if (cp >= 0x0061 && cp <= 0x007A) {
        return cp - 0x20;
    }
    // Latin-1 supplement (partial)
    if (cp >= 0x00E0 && cp <= 0x00F6) {
        return cp - 0x20;
    }
    if (cp >= 0x00F8 && cp <= 0x00FE) {
        return cp - 0x20;
    }
    // Greek (partial)
    if (cp >= 0x03B1 && cp <= 0x03C9) {
        return cp - 0x20;
    }
    // Cyrillic
    if (cp >= 0x0430 && cp <= 0x044F) {
        return cp - 0x20;
    }
    return cp; // No conversion available
}

uint32_t ToLowercase(uint32_t cp) {
    // ASCII
    if (cp >= 0x0041 && cp <= 0x005A) {
        return cp + 0x20;
    }
    // Latin-1 supplement (partial)
    if (cp >= 0x00C0 && cp <= 0x00D6) {
        return cp + 0x20;
    }
    if (cp >= 0x00D8 && cp <= 0x00DE) {
        return cp + 0x20;
    }
    // Greek (partial)
    if (cp >= 0x0391 && cp <= 0x03A1) {
        return cp + 0x20;
    }
    if (cp >= 0x03A3 && cp <= 0x03A9) {
        return cp + 0x20;
    }
    // Cyrillic
    if (cp >= 0x0410 && cp <= 0x042F) {
        return cp + 0x20;
    }
    return cp; // No conversion available
}

UnicodeUtils::ScriptCategory GetScriptCategory(uint32_t cp) {
    // Basic Latin and Latin-1
    if (cp <= 0x024F) {
        return ScriptCategory::Latin;
    }
    // Greek
    if (cp >= 0x0370 && cp <= 0x03FF) {
        return ScriptCategory::Greek;
    }
    // Cyrillic
    if (cp >= 0x0400 && cp <= 0x04FF) {
        return ScriptCategory::Cyrillic;
    }
    // Armenian
    if (cp >= 0x0530 && cp <= 0x058F) {
        return ScriptCategory::Armenian;
    }
    // Hebrew
    if (cp >= 0x0590 && cp <= 0x05FF) {
        return ScriptCategory::Hebrew;
    }
    // Arabic
    if (cp >= 0x0600 && cp <= 0x06FF) {
        return ScriptCategory::Arabic;
    }
    // Syriac
    if (cp >= 0x0700 && cp <= 0x074F) {
        return ScriptCategory::Syriac;
    }
    // Thaana
    if (cp >= 0x0780 && cp <= 0x07BF) {
        return ScriptCategory::Thaana;
    }
    // Devanagari
    if (cp >= 0x0900 && cp <= 0x097F) {
        return ScriptCategory::Devanagari;
    }
    // Bengali
    if (cp >= 0x0980 && cp <= 0x09FF) {
        return ScriptCategory::Bengali;
    }
    // Gurmukhi
    if (cp >= 0x0A00 && cp <= 0x0A7F) {
        return ScriptCategory::Gurmukhi;
    }
    // Gujarati
    if (cp >= 0x0A80 && cp <= 0x0AFF) {
        return ScriptCategory::Gujarati;
    }
    // Oriya
    if (cp >= 0x0B00 && cp <= 0x0B7F) {
        return ScriptCategory::Oriya;
    }
    // Tamil
    if (cp >= 0x0B80 && cp <= 0x0BFF) {
        return ScriptCategory::Tamil;
    }
    // Telugu
    if (cp >= 0x0C00 && cp <= 0x0C7F) {
        return ScriptCategory::Telugu;
    }
    // Kannada
    if (cp >= 0x0C80 && cp <= 0x0CFF) {
        return ScriptCategory::Kannada;
    }
    // Malayalam
    if (cp >= 0x0D00 && cp <= 0x0D7F) {
        return ScriptCategory::Malayalam;
    }
    // Thai
    if (cp >= 0x0E00 && cp <= 0x0E7F) {
        return ScriptCategory::Thai;
    }
    // Lao
    if (cp >= 0x0E80 && cp <= 0x0EFF) {
        return ScriptCategory::Lao;
    }
    // Tibetan
    if (cp >= 0x0F00 && cp <= 0x0FFF) {
        return ScriptCategory::Tibetan;
    }
    // Myanmar
    if (cp >= 0x1000 && cp <= 0x109F) {
        return ScriptCategory::Myanmar;
    }
    // Georgian
    if (cp >= 0x10A0 && cp <= 0x10FF) {
        return ScriptCategory::Georgian;
    }
    // Hangul
    if (cp >= 0xAC00 && cp <= 0xD7AF) {
        return ScriptCategory::Hangul;
    }
    // Ethiopic
    if (cp >= 0x1200 && cp <= 0x137F) {
        return ScriptCategory::Ethiopic;
    }
    // Cherokee
    if (cp >= 0x13A0 && cp <= 0x13FF) {
        return ScriptCategory::Cherokee;
    }
    // Canadian Aboriginal
    if (cp >= 0x1400 && cp <= 0x167F) {
        return ScriptCategory::Canadian;
    }
    // Ogham
    if (cp >= 0x1680 && cp <= 0x169F) {
        return ScriptCategory::Ogham;
    }
    // Runic
    if (cp >= 0x16A0 && cp <= 0x16FF) {
        return ScriptCategory::Runic;
    }
    // Khmer
    if (cp >= 0x1780 && cp <= 0x17FF) {
        return ScriptCategory::Khmer;
    }
    // Mongolian
    if (cp >= 0x1800 && cp <= 0x18AF) {
        return ScriptCategory::Mongolian;
    }
    // Hiragana
    if (cp >= 0x3040 && cp <= 0x309F) {
        return ScriptCategory::Hiragana;
    }
    // Katakana
    if (cp >= 0x30A0 && cp <= 0x30FF) {
        return ScriptCategory::Katakana;
    }
    // Bopomofo
    if (cp >= 0x3100 && cp <= 0x312F) {
        return ScriptCategory::Bopomofo;
    }
    // Han (CJK)
    if (cp >= 0x4E00 && cp <= 0x9FFF) {
        return ScriptCategory::Han;
    }
    // Yi
    if (cp >= 0xA000 && cp <= 0xA48F) {
        return ScriptCategory::Yi;
    }
    // Emoji
    if (cp >= 0x1F000 && cp <= 0x1FFFF) {
        return ScriptCategory::Emoji;
    }
    
    return ScriptCategory::Unknown;
}

bool IsRightToLeft(uint32_t cp) {
    // Arabic and Hebrew scripts are RTL
    if (cp >= 0x0590 && cp <= 0x05FF) { // Hebrew
        return true;
    }
    if (cp >= 0x0600 && cp <= 0x06FF) { // Arabic
        return true;
    }
    if (cp >= 0x0750 && cp <= 0x077F) { // Arabic Supplement
        return true;
    }
    if (cp >= 0xFB50 && cp <= 0xFDFF) { // Arabic Presentation Forms
        return true;
    }
    if (cp >= 0xFE70 && cp <= 0xFEFF) { // Arabic Presentation Forms-B
        return true;
    }
    // Other RTL scripts
    if (cp >= 0x0590 && cp <= 0x08FF) { // Hebrew, Arabic, Syriac
        return true;
    }
    return false;
}

bool IsCombiningMark(uint32_t cp) {
    // Combining diacritical marks
    if (cp >= 0x0300 && cp <= 0x036F) {
        return true;
    }
    // Combining diacritical marks supplement
    if (cp >= 0x1DC0 && cp <= 0x1DFF) {
        return true;
    }
    // Combining marks for symbols
    if (cp >= 0x20D0 && cp <= 0x20FF) {
        return true;
    }
    return false;
}

bool IsVariationSelector(uint32_t cp) {
    // Variation selectors
    if (cp >= 0xFE00 && cp <= 0xFE0F) {
        return true;
    }
    // Variation selectors supplement
    if (cp >= 0xE0100 && cp <= 0xE01EF) {
        return true;
    }
    return false;
}

int GetDisplayWidth(uint32_t cp) {
    // Most characters are single-width
    if (IsWhitespace(cp) && cp != 0x00A0) { // Non-breaking space has width
        return 0;
    }
    
    // CJK characters are typically double-width
    if (cp >= 0x4E00 && cp <= 0x9FFF) { // Han
        return 2;
    }
    if (cp >= 0x3040 && cp <= 0x30FF) { // Hiragana/Katakana
        return 2;
    }
    if (cp >= 0xAC00 && cp <= 0xD7AF) { // Hangul
        return 2;
    }
    if (cp >= 0xFF00 && cp <= 0xFFEF) { // Fullwidth/Halfwidth
        return 2;
    }
    
    // Emoji are typically double-width
    if (cp >= 0x1F000 && cp <= 0x1FFFF) {
        return 2;
    }
    
    return 1;
}

uint32_t Normalize(uint32_t cp) {
    // Basic NFC normalization (simplified)
    // In a full implementation, this would use a normalization table
    
    // Common composed forms
    switch (cp) {
        // Latin ligatures
        case 0x00C6: return 'A'; // Æ -> A
        case 0x00D0: return 'D'; // Ð -> D
        case 0x00DE: return 'T'; // Þ -> T
        case 0x00E6: return 'a'; // æ -> a
        case 0x00F0: return 'd'; // ð -> d
        case 0x00FE: return 't'; // þ -> t
        case 0x0132: return 'I'; // IJ -> I
        case 0x0133: return 'i'; // ij -> i
        case 0x0152: return 'O'; // Œ -> O
        case 0x0153: return 'o'; // œ -> o
        
        // German sharp s
        case 0x00DF: return 's'; // ß -> ss (simplified to single s)
        
        default:
            return cp;
    }
}

} // namespace UnicodeUtils

} // namespace we::UI::Text
