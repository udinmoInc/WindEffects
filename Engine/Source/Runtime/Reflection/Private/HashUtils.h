#pragma once

#include <cstdint>
#include <string_view>

namespace we::runtime::reflection::detail {

/// FNV-1a 64-bit — deterministic across platforms and DLL boundaries.
[[nodiscard]] inline constexpr std::uint64_t Fnv1a64(std::string_view text) noexcept {
    constexpr std::uint64_t kOffset = 14695981039346656037ull;
    constexpr std::uint64_t kPrime = 1099511628211ull;
    std::uint64_t hash = kOffset;
    for (unsigned char c : text) {
        hash ^= static_cast<std::uint64_t>(c);
        hash *= kPrime;
    }
    // Reserve 0 as invalid.
    return hash == 0 ? 1ull : hash;
}

} // namespace we::runtime::reflection::detail
