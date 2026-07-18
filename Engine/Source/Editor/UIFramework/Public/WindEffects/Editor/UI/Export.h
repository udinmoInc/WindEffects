#pragma once

#include "KindUI/Export.h"

#if defined(_WIN32)
#if defined(UIFRAMEWORK_EXPORTS)
#define UIFRAMEWORK_API __declspec(dllexport)
#else
#define UIFRAMEWORK_API __declspec(dllimport)
#endif
#else
#define UIFRAMEWORK_API __attribute__((visibility("default")))
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
