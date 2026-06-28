#include "Localization.hpp"

namespace we::core {

Localization& Localization::Get() {
    static Localization instance;
    return instance;
}

void Localization::LoadStrings(const std::unordered_map<std::string, std::string>& dictionary) {
    for (const auto& [key, val] : dictionary) {
        m_Strings[key] = val;
    }
}

std::string_view Localization::GetString(std::string_view key, std::string_view defaultVal) const {
    auto it = m_Strings.find(std::string(key));
    if (it != m_Strings.end()) {
        return it->second;
    }
    return defaultVal.empty() ? key : defaultVal;
}

} // namespace we::core
