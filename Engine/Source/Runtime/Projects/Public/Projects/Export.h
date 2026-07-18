#pragma once

#if defined(_WIN32)
#if defined(PROJECTS_EXPORTS)
#define PROJECTS_API __declspec(dllexport)
#else
#define PROJECTS_API __declspec(dllimport)
#endif
#else
#define PROJECTS_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
