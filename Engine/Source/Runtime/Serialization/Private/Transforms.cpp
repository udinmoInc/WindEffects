#include "Serialization/Transforms.h"

#include <cstring>
#include <mutex>

namespace we::runtime::serialization {
namespace {

std::mutex& Mutex() {
    static std::mutex mutex;
    return mutex;
}

CompressionBackend& CompressionSlot() {
    static CompressionBackend backend{};
    return backend;
}

EncryptionBackend& EncryptionSlot() {
    static EncryptionBackend backend{};
    return backend;
}

bool IdentityCompress(
    const std::uint8_t* input,
    std::size_t inputSize,
    std::vector<std::uint8_t>& output,
    void*)
{
    output.assign(input, input + inputSize);
    return true;
}

bool IdentityDecompress(
    const std::uint8_t* input,
    std::size_t inputSize,
    std::size_t,
    std::vector<std::uint8_t>& output,
    void*)
{
    output.assign(input, input + inputSize);
    return true;
}

bool IdentityEncrypt(
    const std::uint8_t* input,
    std::size_t inputSize,
    std::vector<std::uint8_t>& output,
    void*)
{
    output.assign(input, input + inputSize);
    return true;
}

bool IdentityDecrypt(
    const std::uint8_t* input,
    std::size_t inputSize,
    std::vector<std::uint8_t>& output,
    void*)
{
    output.assign(input, input + inputSize);
    return true;
}

} // namespace

void SetCompressionBackend(CompressionBackend backend) {
    std::lock_guard lock(Mutex());
    CompressionSlot() = backend;
}

CompressionBackend GetCompressionBackend() {
    std::lock_guard lock(Mutex());
    CompressionBackend backend = CompressionSlot();
    if (!backend.compress) {
        backend.name = "identity";
        backend.compress = &IdentityCompress;
        backend.decompress = &IdentityDecompress;
    }
    return backend;
}

void SetEncryptionBackend(EncryptionBackend backend) {
    std::lock_guard lock(Mutex());
    EncryptionSlot() = backend;
}

EncryptionBackend GetEncryptionBackend() {
    std::lock_guard lock(Mutex());
    EncryptionBackend backend = EncryptionSlot();
    if (!backend.encrypt) {
        backend.name = "identity";
        backend.encrypt = &IdentityEncrypt;
        backend.decrypt = &IdentityDecrypt;
    }
    return backend;
}

bool CompressBytes(
    const std::uint8_t* input,
    std::size_t inputSize,
    std::vector<std::uint8_t>& output)
{
    if (!input && inputSize > 0) {
        return false;
    }
    const CompressionBackend backend = GetCompressionBackend();
    return backend.compress(input, inputSize, output, backend.userData);
}

bool DecompressBytes(
    const std::uint8_t* input,
    std::size_t inputSize,
    std::size_t expectedUncompressedSize,
    std::vector<std::uint8_t>& output)
{
    if (!input && inputSize > 0) {
        return false;
    }
    const CompressionBackend backend = GetCompressionBackend();
    return backend.decompress(input, inputSize, expectedUncompressedSize, output, backend.userData);
}

bool EncryptBytes(
    const std::uint8_t* input,
    std::size_t inputSize,
    std::vector<std::uint8_t>& output)
{
    if (!input && inputSize > 0) {
        return false;
    }
    const EncryptionBackend backend = GetEncryptionBackend();
    return backend.encrypt(input, inputSize, output, backend.userData);
}

bool DecryptBytes(
    const std::uint8_t* input,
    std::size_t inputSize,
    std::vector<std::uint8_t>& output)
{
    if (!input && inputSize > 0) {
        return false;
    }
    const EncryptionBackend backend = GetEncryptionBackend();
    return backend.decrypt(input, inputSize, output, backend.userData);
}

} // namespace we::runtime::serialization
