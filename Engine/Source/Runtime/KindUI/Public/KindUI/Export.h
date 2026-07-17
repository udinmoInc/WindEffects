#pragma once

#if defined(_WIN32)
#if defined(KINDUI_EXPORTS)
#define KINDUI_API __declspec(dllexport)
#else
#define KINDUI_API __declspec(dllimport)
#endif
#else
#define KINDUI_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif