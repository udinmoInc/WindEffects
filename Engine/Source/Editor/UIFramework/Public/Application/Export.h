#pragma once

#if defined(_WIN32)
#if defined(APPLICATION_EXPORTS)
#define UIFRAMEWORK_API __declspec(dllexport)
#else
#define UIFRAMEWORK_API __declspec(dllimport)
#endif
#else
#define UIFRAMEWORK_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
