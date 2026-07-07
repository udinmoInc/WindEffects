#pragma once

#if defined(_WIN32)
#if defined(ENVIRONMENT_EXPORTS)
#define ENVIRONMENT_API __declspec(dllexport)
#else
#define ENVIRONMENT_API __declspec(dllimport)
#endif
#else
#define ENVIRONMENT_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
