#pragma once

#include "WindEffects/Runtime/UI/Export.h"

#if defined(UIFRAMEWORK_EXPORTS)
    #define UIFRAMEWORK_API __declspec(dllexport)
#else
    #define UIFRAMEWORK_API __declspec(dllimport)
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif