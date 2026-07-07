#pragma once

#if defined(_WIN32)
#if defined(MAINFRAME_EXPORTS)
#define MAINFRAME_API __declspec(dllexport)
#else
#define MAINFRAME_API __declspec(dllimport)
#endif
#else
#define MAINFRAME_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
