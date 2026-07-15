#pragma once

#if defined(_WIN32)
#if defined(PLATFORM_EXPORTS)
#define PLATFORM_API __declspec(dllexport)
#else
#define PLATFORM_API __declspec(dllimport)
#endif
#else
#define PLATFORM_API __attribute__((visibility("default")))
#endif
