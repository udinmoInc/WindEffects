#pragma once

#if defined(_WIN32)
#if defined(ENGINE_EXPORTS)
#define ENGINE_API __declspec(dllexport)
#else
#define ENGINE_API __declspec(dllimport)
#endif
#else
#define ENGINE_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
