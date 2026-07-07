#pragma once

#if defined(_WIN32)
#if defined(SCENE_EXPORTS)
#define SCENE_API __declspec(dllexport)
#else
#define SCENE_API __declspec(dllimport)
#endif
#else
#define SCENE_API
#endif
