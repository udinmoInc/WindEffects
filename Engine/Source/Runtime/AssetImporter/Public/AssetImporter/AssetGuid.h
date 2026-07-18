#pragma once

#include "AssetImporter/Export.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace we::runtime::assetimporter {

/// Stable 128-bit asset identity. Independent of path; survives renames/moves.
struct ASSETIMPORTER_API AssetGuid {
    std::array<uint8_t, 16> bytes{};

    [[nodiscard]] static AssetGuid Generate();
    [[nodiscard]] static std::optional<AssetGuid> Parse(std::string_view text);
    [[nodiscard]] static AssetGuid FromBytes(const std::array<uint8_t, 16>& value);

    [[nodiscard]] bool IsNil() const noexcept;
    [[nodiscard]] std::string ToString() const;
    [[nodiscard]] bool operator==(const AssetGuid& other) const noexcept;
    [[nodiscard]] bool operator!=(const AssetGuid& other) const noexcept { return !(*this == other); }
};

struct ASSETIMPORTER_API AssetGuidHash {
    std::size_t operator()(const AssetGuid& guid) const noexcept;
};

} // namespace we::runtime::assetimporter
