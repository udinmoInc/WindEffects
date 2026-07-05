#pragma once

#include "Core/Export.hpp"
#include <string>
#include <string_view>
#include <unordered_map>

namespace we::core {

class Localization {
public:
    CORE_API static Localization& Get(); // Can be injected via ServiceLocator

    CORE_API void LoadStrings(const std::unordered_map<std::string, std::string>& dictionary);
    [[nodiscard]] CORE_API std::string_view GetString(std::string_view key, std::string_view defaultVal = "") const;

private:
    Localization() = default;
    ~Localization() = default;

    std::unordered_map<std::string, std::string> m_Strings;
};

} // namespace we::core
