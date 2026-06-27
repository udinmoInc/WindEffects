#pragma once

#include <string>

namespace HouseEngine::UI {

// Clean, 2px rounded stroke SVG icons matching standard AAA conventions.
// The SVG definitions should use <svg width="48" height="48" viewBox="0 0 48 48">

const std::string g_SvgFolder = R"(
<svg width="48" height="48" viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
<path d="M4.0 12.0 C4.0 9.8 5.8 8.0 8.0 8.0 L18.0 8.0 C19.1 8.0 20.1 8.4 20.8 9.2 L24.0 12.0 L40.0 12.0 C42.2 12.0 44.0 13.8 44.0 16.0 L44.0 36.0 C44.0 38.2 42.2 40.0 40.0 40.0 L8.0 40.0 C5.8 40.0 4.0 38.2 4.0 36.0 L4.0 12.0 Z" fill="#D4B86A" stroke="#B89B48" stroke-width="2" stroke-linejoin="round"/>
</svg>
)";

const std::string g_SvgBlueprint = R"(
<svg width="48" height="48" viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
<rect x="8" y="6" width="32" height="36" rx="4" fill="#3B82F6" stroke="#2563EB" stroke-width="2"/>
<circle cx="24" cy="24" r="8" fill="#FFFFFF"/>
<path d="M24 16 L24 32 M16 24 L32 24" stroke="#3B82F6" stroke-width="2" stroke-linecap="round"/>
</svg>
)";

const std::string g_SvgMaterial = R"(
<svg width="48" height="48" viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
<circle cx="24" cy="24" r="16" fill="#10B981" stroke="#059669" stroke-width="2"/>
<path d="M12 24 C12 16 20 12 24 12" stroke="#FFFFFF" stroke-width="3" stroke-linecap="round" fill="none"/>
</svg>
)";

const std::string g_SvgStaticMesh = R"(
<svg width="48" height="48" viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
<path d="M24 6 L40 14 L40 34 L24 42 L8 34 L8 14 Z" fill="#06B6D4" stroke="#0891B2" stroke-width="2" stroke-linejoin="round"/>
<path d="M24 6 L24 24 L40 14 M24 24 L8 14 M24 24 L24 42" stroke="#0891B2" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
</svg>
)";

const std::string g_SvgTexture = R"(
<svg width="48" height="48" viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
<rect x="6" y="6" width="36" height="36" rx="4" fill="#EF4444" stroke="#DC2626" stroke-width="2"/>
<path d="M6 32 L18 20 L28 30 L36 22 L42 28" stroke="#FFFFFF" stroke-width="3" stroke-linecap="round" stroke-linejoin="round" fill="none"/>
<circle cx="16" cy="16" r="3" fill="#FFFFFF"/>
</svg>
)";

const std::string g_SvgMap = R"(
<svg width="48" height="48" viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
<rect x="6" y="8" width="36" height="32" rx="3" fill="#F59E0B" stroke="#D97706" stroke-width="2"/>
<path d="M18 8 L18 40 M30 8 L30 40 M6 24 L42 24" stroke="#D97706" stroke-width="2" stroke-linecap="round"/>
</svg>
)";

const std::string g_SvgGeneric = R"(
<svg width="48" height="48" viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
<rect x="10" y="6" width="28" height="36" rx="3" fill="#9CA3AF" stroke="#6B7280" stroke-width="2"/>
<path d="M16 16 L32 16 M16 24 L32 24 M16 32 L26 32" stroke="#FFFFFF" stroke-width="3" stroke-linecap="round"/>
</svg>
)";

} // namespace HouseEngine::UI
