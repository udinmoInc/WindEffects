#pragma once

#include "Text/Core/Errors.h"
#include "Text/Export.h"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::text::unicode {

enum class Encoding : uint8_t {
    Utf8,
    Utf16Le,
    Utf16Be,
    Utf32Le,
    Utf32Be,
};

enum class DecodeErrorReason : uint8_t {
    TruncatedSequence,
    InvalidContinuation,
    OverlongEncoding,
    SurrogateValue,
    OutOfRange,
    InvalidByte,
};

struct DecodeError {
    size_t byteOffset = 0;
    DecodeErrorReason reason = DecodeErrorReason::InvalidByte;
};

struct DecodeOutput {
    std::vector<Codepoint> codepoints;
    std::vector<DecodeError> errors;
    size_t bytesConsumed = 0;
};

class TEXT_API IUnicodeDecoder {
public:
    virtual ~IUnicodeDecoder() = default;

    [[nodiscard]] virtual DecodeOutput Decode(
        std::span<const std::byte> input,
        Encoding encoding) const = 0;

    [[nodiscard]] virtual bool IsValid(
        std::span<const std::byte> input,
        Encoding encoding) const = 0;

    static constexpr Codepoint ReplacementCharacter() { return 0xFFFD; }
    [[nodiscard]] static bool IsValidCodepoint(Codepoint codepoint);
};

class TEXT_API UnicodeDecoder final : public IUnicodeDecoder {
public:
    [[nodiscard]] DecodeOutput Decode(
        std::span<const std::byte> input,
        Encoding encoding) const override;

    [[nodiscard]] bool IsValid(
        std::span<const std::byte> input,
        Encoding encoding) const override;

private:
    [[nodiscard]] DecodeOutput DecodeUtf8(std::span<const std::byte> input) const;
    [[nodiscard]] DecodeOutput DecodeUtf16(std::span<const std::byte> input, bool bigEndian) const;
    [[nodiscard]] DecodeOutput DecodeUtf32(std::span<const std::byte> input, bool bigEndian) const;
};

[[nodiscard]] TEXT_API std::unique_ptr<IUnicodeDecoder> CreateUnicodeDecoder();

} // namespace we::runtime::text::unicode
