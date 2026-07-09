#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace we::UI {

// Writes uncompressed 32-bit BMP (BGRA) for atlas / icon pipeline debugging.
bool SaveBmpRgba(const std::string& path, const std::vector<uint8_t>& rgba, uint32_t width, uint32_t height);

} // namespace we::UI
