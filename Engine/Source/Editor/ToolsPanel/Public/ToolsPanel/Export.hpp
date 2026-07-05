#pragma once

#if defined(_WIN32)
#if defined(TOOLSPANEL_EXPORTS)
#define TOOLSPANEL_API __declspec(dllexport)
#else
#define TOOLSPANEL_API __declspec(dllimport)
#endif
#else
#define TOOLSPANEL_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
