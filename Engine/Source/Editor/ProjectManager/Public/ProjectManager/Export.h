#pragma once

#if defined(_WIN32)
#if defined(PROJECTMANAGER_EXPORTS)
#define PROJECTMANAGER_API __declspec(dllexport)
#else
#define PROJECTMANAGER_API __declspec(dllimport)
#endif
#else
#define PROJECTMANAGER_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
