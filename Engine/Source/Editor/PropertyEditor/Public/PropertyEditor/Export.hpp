#pragma once

#if defined(_WIN32)
#if defined(PROPERTYEDITOR_EXPORTS)
#define PROPERTYEDITOR_API __declspec(dllexport)
#else
#define PROPERTYEDITOR_API __declspec(dllimport)
#endif
#else
#define PROPERTYEDITOR_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
