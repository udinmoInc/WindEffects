#pragma once

#if defined(_WIN32)
#if defined(REFLECTION_EXPORTS)
#define REFLECTION_API __declspec(dllexport)
#else
#define REFLECTION_API __declspec(dllimport)
#endif
#else
#define REFLECTION_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
