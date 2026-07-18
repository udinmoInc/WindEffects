#pragma once

#include <cstdint>
#include <string_view>

namespace we::runtime::serialization::detail {

[[nodiscard]] inline constexpr std::uint64_t Fnv1a64(std::string_view text) noexcept {
    constexpr std::uint64_t kOffset = 14695981039346656037ull;
    constexpr std::uint64_t kPrime = 1099511628211ull;
    std::uint64_t hash = kOffset;
    for (unsigned char c : text) {
        hash ^= static_cast<std::uint64_t>(c);
        hash *= kPrime;
    }
    return hash == 0 ? 1ull : hash;
}

[[nodiscard]] inline std::uint64_t Fnv1a64Bytes(const std::uint8_t* data, std::size_t size) noexcept {
    constexpr std::uint64_t kOffset = 14695981039346656037ull;
    constexpr std::uint64_t kPrime = 1099511628211ull;
    std::uint64_t hash = kOffset;
    for (std::size_t i = 0; i < size; ++i) {
        hash ^= data[i];
        hash *= kPrime;
    }
    return hash == 0 ? 1ull : hash;
}

} // namespace we::runtime::serialization::detail
