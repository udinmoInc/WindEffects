#pragma once

#include "Serialization/Export.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace we::runtime::serialization {

/// Optional compression backend. Identity (no-op) is the default.
using CompressFn = bool (*)(
    const std::uint8_t* input,
    std::size_t inputSize,
    std::vector<std::uint8_t>& output,
    void* userData);

using DecompressFn = bool (*)(
    const std::uint8_t* input,
    std::size_t inputSize,
    std::size_t expectedUncompressedSize,
    std::vector<std::uint8_t>& output,
    void* userData);

struct SERIALIZATION_API CompressionBackend {
    const char* name = "identity";
    CompressFn compress = nullptr;
    DecompressFn decompress = nullptr;
    void* userData = nullptr;
};

SERIALIZATION_API void SetCompressionBackend(CompressionBackend backend);
[[nodiscard]] SERIALIZATION_API CompressionBackend GetCompressionBackend();

/// Optional encryption backend. Identity (no-op) is the default.
using EncryptFn = bool (*)(
    const std::uint8_t* input,
    std::size_t inputSize,
    std::vector<std::uint8_t>& output,
    void* userData);

using DecryptFn = bool (*)(
    const std::uint8_t* input,
    std::size_t inputSize,
    std::vector<std::uint8_t>& output,
    void* userData);

struct SERIALIZATION_API EncryptionBackend {
    const char* name = "identity";
    EncryptFn encrypt = nullptr;
    DecryptFn decrypt = nullptr;
    void* userData = nullptr;
};

SERIALIZATION_API void SetEncryptionBackend(EncryptionBackend backend);
[[nodiscard]] SERIALIZATION_API EncryptionBackend GetEncryptionBackend();

/// Apply configured compression (falls back to copy).
[[nodiscard]] SERIALIZATION_API bool CompressBytes(
    const std::uint8_t* input,
    std::size_t inputSize,
    std::vector<std::uint8_t>& output);

[[nodiscard]] SERIALIZATION_API bool DecompressBytes(
    const std::uint8_t* input,
    std::size_t inputSize,
    std::size_t expectedUncompressedSize,
    std::vector<std::uint8_t>& output);

[[nodiscard]] SERIALIZATION_API bool EncryptBytes(
    const std::uint8_t* input,
    std::size_t inputSize,
    std::vector<std::uint8_t>& output);

[[nodiscard]] SERIALIZATION_API bool DecryptBytes(
    const std::uint8_t* input,
    std::size_t inputSize,
    std::vector<std::uint8_t>& output);

} // namespace we::runtime::serialization
