#pragma once

#if defined(_WIN32)
#if defined(ICONS_EXPORTS)
#define ICONS_API __declspec(dllexport)
#else
#define ICONS_API __declspec(dllimport)
#endif
#else
#define ICONS_API
#endif
