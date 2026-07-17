#pragma once

#if defined(_WIN32)
#if defined(UI_EXPORTS)
#define UI_API __declspec(dllexport)
#else
#define UI_API __declspec(dllimport)
#endif
#else
#define UI_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif