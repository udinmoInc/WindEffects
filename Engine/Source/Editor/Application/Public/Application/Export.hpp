#pragma once

#if defined(_WIN32)
#if defined(APPLICATION_EXPORTS)
#define APPLICATION_API __declspec(dllexport)
#else
#define APPLICATION_API __declspec(dllimport)
#endif
#else
#define APPLICATION_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
