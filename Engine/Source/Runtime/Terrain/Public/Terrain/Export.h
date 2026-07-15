#pragma once

#if defined(_WIN32)
#if defined(TERRAIN_EXPORTS)
#define TERRAIN_API __declspec(dllexport)
#else
#define TERRAIN_API __declspec(dllimport)
#endif
#else
#define TERRAIN_API
#endif
