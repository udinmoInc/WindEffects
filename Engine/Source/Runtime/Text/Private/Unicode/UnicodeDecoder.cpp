#include "Text/Unicode/UnicodeDecoder.h"

#include <bit>
#include <cstring>

namespace we::runtime::text::unicode {

namespace {

uint16_t ReadU16Le(const std::byte* bytes)
{
    uint16_t value = 0;
    std::memcpy(&value, bytes, sizeof(value));
    return value;
}

uint16_t ReadU16Be(const std::byte* bytes)
{
    const uint16_t value = ReadU16Le(bytes);
    return static_cast<uint16_t>((value << 8) | (value >> 8));
}

uint32_t ReadU32Le(const std::byte* bytes)
{
    uint32_t value = 0;
    std::memcpy(&value, bytes, sizeof(value));
    return value;
}

uint32_t ReadU32Be(const std::byte* bytes)
{
    const uint32_t value = ReadU32Le(bytes);
    return ((value & 0x000000FFu) << 24)
        | ((value & 0x0000FF00u) << 8)
        | ((value & 0x00FF0000u) >> 8)
        | ((value & 0xFF000000u) >> 24);
}

void PushReplacement(DecodeOutput& output, size_t byteOffset, DecodeErrorReason reason)
{
    output.codepoints.push_back(ReplacementCharacter());
    output.errors.push_back({byteOffset, reason});
}

} // namespace

bool IUnicodeDecoder::IsValidCodepoint(const Codepoint codepoint)
{
    if (codepoint > 0x10FFFF) {
        return false;
    }
    return codepoint < 0xD800 || codepoint > 0xDFFF;
}

DecodeOutput UnicodeDecoder::Decode(
    const std::span<const std::byte> input,
    const Encoding encoding) const
{
    switch (encoding) {
    case Encoding::Utf8:
        return DecodeUtf8(input);
    case Encoding::Utf16Le:
        return DecodeUtf16(input, false);
    case Encoding::Utf16Be:
        return DecodeUtf16(input, true);
    case Encoding::Utf32Le:
        return DecodeUtf32(input, false);
    case Encoding::Utf32Be:
        return DecodeUtf32(input, true);
    }
    return {};
}

bool UnicodeDecoder::IsValid(
    const std::span<const std::byte> input,
    const Encoding encoding) const
{
    return Decode(input, encoding).errors.empty();
}

DecodeOutput UnicodeDecoder::DecodeUtf8(const std::span<const std::byte> input) const
{
    DecodeOutput output;
    output.codepoints.reserve(input.size());

    size_t index = 0;
    while (index < input.size()) {
        const auto byte = static_cast<uint8_t>(input[index]);
        size_t sequenceLength = 1;
        Codepoint codepoint = 0;

        if (byte <= 0x7F) {
            codepoint = byte;
        } else if ((byte & 0xE0) == 0xC0) {
            sequenceLength = 2;
            if (index + 1 >= input.size()) {
                PushReplacement(output, index, DecodeErrorReason::TruncatedSequence);
                ++index;
                continue;
            }
            const auto b1 = static_cast<uint8_t>(input[index + 1]);
            if ((b1 & 0xC0) != 0x80) {
                PushReplacement(output, index, DecodeErrorReason::InvalidContinuation);
                ++index;
                continue;
            }
            codepoint = ((byte & 0x1F) << 6) | (b1 & 0x3F);
            if (codepoint < 0x80) {
                PushReplacement(output, index, DecodeErrorReason::OverlongEncoding);
                ++index;
                continue;
            }
        } else if ((byte & 0xF0) == 0xE0) {
            sequenceLength = 3;
            if (index + 2 >= input.size()) {
                PushReplacement(output, index, DecodeErrorReason::TruncatedSequence);
                ++index;
                continue;
            }
            const auto b1 = static_cast<uint8_t>(input[index + 1]);
            const auto b2 = static_cast<uint8_t>(input[index + 2]);
            if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) {
                PushReplacement(output, index, DecodeErrorReason::InvalidContinuation);
                ++index;
                continue;
            }
            codepoint = ((byte & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
            if (codepoint < 0x800 || (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
                PushReplacement(output, index, DecodeErrorReason::OverlongEncoding);
                ++index;
                continue;
            }
        } else if ((byte & 0xF8) == 0xF0) {
            sequenceLength = 4;
            if (index + 3 >= input.size()) {
                PushReplacement(output, index, DecodeErrorReason::TruncatedSequence);
                ++index;
                continue;
            }
            const auto b1 = static_cast<uint8_t>(input[index + 1]);
            const auto b2 = static_cast<uint8_t>(input[index + 2]);
            const auto b3 = static_cast<uint8_t>(input[index + 3]);
            if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) {
                PushReplacement(output, index, DecodeErrorReason::InvalidContinuation);
                ++index;
                continue;
            }
            codepoint = ((byte & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
            if (codepoint < 0x10000 || codepoint > 0x10FFFF) {
                PushReplacement(output, index, DecodeErrorReason::OutOfRange);
                ++index;
                continue;
            }
        } else {
            PushReplacement(output, index, DecodeErrorReason::InvalidByte);
            ++index;
            continue;
        }

        if (!IsValidCodepoint(codepoint)) {
            PushReplacement(output, index, DecodeErrorReason::OutOfRange);
            index += sequenceLength;
            continue;
        }

        output.codepoints.push_back(codepoint);
        index += sequenceLength;
    }

    output.bytesConsumed = input.size();
    return output;
}

DecodeOutput UnicodeDecoder::DecodeUtf16(
    const std::span<const std::byte> input,
    const bool bigEndian) const
{
    DecodeOutput output;
    if (input.size() % 2 != 0) {
        output.errors.push_back({input.size() - 1, DecodeErrorReason::TruncatedSequence});
    }

    size_t index = 0;
    while (index + 1 < input.size()) {
        const uint16_t unit = bigEndian
            ? ReadU16Be(reinterpret_cast<const std::byte*>(&input[index]))
            : ReadU16Le(reinterpret_cast<const std::byte*>(&input[index]));

        if (unit >= 0xD800 && unit <= 0xDBFF) {
            if (index + 3 >= input.size()) {
                PushReplacement(output, index, DecodeErrorReason::TruncatedSequence);
                break;
            }
            const uint16_t low = bigEndian
                ? ReadU16Be(reinterpret_cast<const std::byte*>(&input[index + 2]))
                : ReadU16Le(reinterpret_cast<const std::byte*>(&input[index + 2]));
            if (low < 0xDC00 || low > 0xDFFF) {
                PushReplacement(output, index, DecodeErrorReason::InvalidContinuation);
                index += 2;
                continue;
            }
            const Codepoint codepoint = 0x10000 + ((static_cast<uint32_t>(unit - 0xD800) << 10) | (low - 0xDC00));
            output.codepoints.push_back(codepoint);
            index += 4;
            continue;
        }

        if (unit >= 0xDC00 && unit <= 0xDFFF) {
            PushReplacement(output, index, DecodeErrorReason::SurrogateValue);
            index += 2;
            continue;
        }

        output.codepoints.push_back(unit);
        index += 2;
    }

    output.bytesConsumed = input.size();
    return output;
}

DecodeOutput UnicodeDecoder::DecodeUtf32(
    const std::span<const std::byte> input,
    const bool bigEndian) const
{
    DecodeOutput output;
    if (input.size() % 4 != 0) {
        output.errors.push_back({input.size() - 1, DecodeErrorReason::TruncatedSequence});
    }

    for (size_t index = 0; index + 3 < input.size(); index += 4) {
        const Codepoint codepoint = bigEndian
            ? ReadU32Be(reinterpret_cast<const std::byte*>(&input[index]))
            : ReadU32Le(reinterpret_cast<const std::byte*>(&input[index]));

        if (!IsValidCodepoint(codepoint)) {
            PushReplacement(output, index, DecodeErrorReason::OutOfRange);
            continue;
        }

        output.codepoints.push_back(codepoint);
    }

    output.bytesConsumed = input.size();
    return output;
}

std::unique_ptr<IUnicodeDecoder> CreateUnicodeDecoder()
{
    return std::make_unique<UnicodeDecoder>();
}

} // namespace we::runtime::text::unicode
