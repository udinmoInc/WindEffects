#pragma once

#include "KindUI/Export.h"

#if defined(_WIN32)
#if defined(EDITORSHELL_EXPORTS)
#define EDITORSHELL_API __declspec(dllexport)
#else
#define EDITORSHELL_API __declspec(dllimport)
#endif
#else
#define EDITORSHELL_API __attribute__((visibility("default")))
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif
