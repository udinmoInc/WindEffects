#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace we::core {

class Localization {
public:
    static Localization& Get(); // Can be injected via ServiceLocator

    void LoadStrings(const std::unordered_map<std::string, std::string>& dictionary);
    [[nodiscard]] std::string_view GetString(std::string_view key, std::string_view defaultVal = "") const;

private:
    Localization() = default;
    ~Localization() = default;

    std::unordered_map<std::string, std::string> m_Strings;
};

} // namespace we::core
