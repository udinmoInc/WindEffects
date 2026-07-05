#pragma once

#if defined(_WIN32)
#if defined(WORLD_EXPORTS)
#define WORLD_API __declspec(dllexport)
#else
#define WORLD_API __declspec(dllimport)
#endif
#else
#define WORLD_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
