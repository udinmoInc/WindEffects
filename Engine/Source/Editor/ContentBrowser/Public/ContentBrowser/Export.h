#pragma once

#if defined(_WIN32)
#if defined(CONTENTBROWSER_EXPORTS)
#define CONTENTBROWSER_API __declspec(dllexport)
#else
#define CONTENTBROWSER_API __declspec(dllimport)
#endif
#else
#define CONTENTBROWSER_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
