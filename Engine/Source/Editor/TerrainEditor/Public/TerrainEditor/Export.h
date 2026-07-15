#pragma once

#if defined(_WIN32)
#if defined(TERRAINEDITOR_EXPORTS)
#define TERRAINEDITOR_API __declspec(dllexport)
#else
#define TERRAINEDITOR_API __declspec(dllimport)
#endif
#else
#define TERRAINEDITOR_API
#endif
