#pragma once

#if defined(_WIN32)
#if defined(VIEWPORT_EXPORTS)
#define VIEWPORT_API __declspec(dllexport)
#else
#define VIEWPORT_API __declspec(dllimport)
#endif
#else
#define VIEWPORT_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
