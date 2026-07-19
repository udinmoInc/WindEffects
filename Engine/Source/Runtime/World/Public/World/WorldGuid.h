#pragma once

#include "World/Export.h"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace we::runtime::world {

/// Stable 128-bit world-runtime identity (layout-compatible with AssetGuid / ecs::Uuid).
struct WORLD_API WorldGuid {
    std::array<std::uint8_t, 16> bytes{};

    [[nodiscard]] static WorldGuid Generate();
    [[nodiscard]] static WorldGuid Nil() noexcept;
    [[nodiscard]] static std::optional<WorldGuid> Parse(std::string_view text);
    [[nodiscard]] static WorldGuid FromBytes(const std::array<std::uint8_t, 16>& value) noexcept;

    [[nodiscard]] bool IsNil() const noexcept;
    [[nodiscard]] std::string ToString() const;
    [[nodiscard]] bool operator==(const WorldGuid& other) const noexcept;
    [[nodiscard]] bool operator!=(const WorldGuid& other) const noexcept { return !(*this == other); }
};

struct WORLD_API WorldGuidHash {
    [[nodiscard]] std::size_t operator()(const WorldGuid& guid) const noexcept;
};

} // namespace we::runtime::world
