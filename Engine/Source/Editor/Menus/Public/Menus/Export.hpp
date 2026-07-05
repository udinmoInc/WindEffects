#pragma once

#if defined(_WIN32)
#if defined(MENUS_EXPORTS)
#define MENUS_API __declspec(dllexport)
#else
#define MENUS_API __declspec(dllimport)
#endif
#else
#define MENUS_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
