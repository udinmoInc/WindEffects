#pragma once

#ifdef _WIN32
// Define these BEFORE including any Windows headers to prevent macro conflicts
// These must be defined before any includes to take effect
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Also define these to prevent additional conflicts
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#endif

// Include Windows headers with proper ordering
#include <windows.h>
#include <shlobj.h>
#endif
