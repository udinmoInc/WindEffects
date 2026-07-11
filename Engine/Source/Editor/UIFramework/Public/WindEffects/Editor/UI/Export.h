#pragma once

#if defined(UIFRAMEWORK_EXPORTS)
    #define UIFRAMEWORK_API __declspec(dllexport)
#else
    #define UIFRAMEWORK_API __declspec(dllimport)
#endif
