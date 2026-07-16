#pragma once

#include "ECS/Types.h"

#include <array>
#include <cstdint>
#include <cstring>

namespace we::runtime::ecs {

// Fixed 256-bit component signature for fast archetype matching.
struct ComponentMask {
    std::array<std::uint64_t, kComponentMaskWords> words{};

    constexpr ComponentMask() = default;

    constexpr void Set(std::uint32_t typeId, bool value = true) {
        const std::uint32_t word = typeId / 64u;
        const std::uint64_t bit = 1ull << (typeId % 64u);
        if (value) {
            words[word] |= bit;
        } else {
            words[word] &= ~bit;
        }
    }

    [[nodiscard]] constexpr bool Test(std::uint32_t typeId) const {
        const std::uint32_t word = typeId / 64u;
        const std::uint64_t bit = 1ull << (typeId % 64u);
        return (words[word] & bit) != 0;
    }

    [[nodiscard]] constexpr bool Any() const {
        for (std::uint64_t w : words) {
            if (w != 0) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] constexpr bool Empty() const { return !Any(); }

    [[nodiscard]] constexpr bool Contains(const ComponentMask& required) const {
        for (std::size_t i = 0; i < kComponentMaskWords; ++i) {
            if ((words[i] & required.words[i]) != required.words[i]) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] constexpr bool Intersects(const ComponentMask& other) const {
        for (std::size_t i = 0; i < kComponentMaskWords; ++i) {
            if ((words[i] & other.words[i]) != 0) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] constexpr bool CompatibleWith(const ComponentMask& required,
        const ComponentMask& excluded) const {
        return Contains(required) && !Intersects(excluded);
    }

    constexpr ComponentMask& operator|=(const ComponentMask& other) {
        for (std::size_t i = 0; i < kComponentMaskWords; ++i) {
            words[i] |= other.words[i];
        }
        return *this;
    }

    constexpr ComponentMask& operator&=(const ComponentMask& other) {
        for (std::size_t i = 0; i < kComponentMaskWords; ++i) {
            words[i] &= other.words[i];
        }
        return *this;
    }

    [[nodiscard]] constexpr bool operator==(const ComponentMask& other) const {
        return words == other.words;
    }

    [[nodiscard]] constexpr bool operator!=(const ComponentMask& other) const {
        return !(*this == other);
    }

    [[nodiscard]] std::uint32_t Count() const {
        std::uint32_t n = 0;
        for (std::uint64_t w : words) {
            while (w != 0) {
                n += static_cast<std::uint32_t>(w & 1ull);
                w >>= 1;
            }
        }
        return n;
    }
};

} // namespace we::runtime::ecs
