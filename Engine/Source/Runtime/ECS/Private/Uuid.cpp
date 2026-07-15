#include "ECS/Components/CoreComponents.h"

#include <cstdio>
#include <random>
#include <sstream>

namespace we::runtime::ecs {

Uuid Uuid::Generate() {
    Uuid id{};
    static thread_local std::mt19937_64 rng{ std::random_device{}() };
    std::uniform_int_distribution<int> dist(0, 255);
    for (auto& b : id.bytes) {
        b = static_cast<std::uint8_t>(dist(rng));
    }
    // RFC 4122 variant/version bits (version 4).
    id.bytes[6] = static_cast<std::uint8_t>((id.bytes[6] & 0x0F) | 0x40);
    id.bytes[8] = static_cast<std::uint8_t>((id.bytes[8] & 0x3F) | 0x80);
    return id;
}

Uuid Uuid::FromString(const std::string& text) {
    Uuid id{};
    unsigned int values[16]{};
    if (std::sscanf(text.c_str(),
            "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            &values[0], &values[1], &values[2], &values[3],
            &values[4], &values[5], &values[6], &values[7],
            &values[8], &values[9], &values[10], &values[11],
            &values[12], &values[13], &values[14], &values[15]) == 16) {
        for (int i = 0; i < 16; ++i) {
            id.bytes[static_cast<std::size_t>(i)] = static_cast<std::uint8_t>(values[i]);
        }
    }
    return id;
}

std::string Uuid::ToString() const {
    char buf[64];
    std::snprintf(buf, sizeof(buf),
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        bytes[0], bytes[1], bytes[2], bytes[3],
        bytes[4], bytes[5], bytes[6], bytes[7],
        bytes[8], bytes[9], bytes[10], bytes[11],
        bytes[12], bytes[13], bytes[14], bytes[15]);
    return buf;
}

bool Uuid::IsNil() const {
    for (std::uint8_t b : bytes) {
        if (b != 0) {
            return false;
        }
    }
    return true;
}

} // namespace we::runtime::ecs
