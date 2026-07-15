#pragma once

// Include this AFTER any Windows SDK headers (`windows.h`, etc.).
// Win32 A/W macros collide with Platform API names.

#ifdef CreateWindow
#undef CreateWindow
#endif
#ifdef CreateWindowA
#undef CreateWindowA
#endif
#ifdef CreateWindowW
#undef CreateWindowW
#endif
#ifdef LoadLibrary
#undef LoadLibrary
#endif
#ifdef LoadLibraryA
#undef LoadLibraryA
#endif
#ifdef LoadLibraryW
#undef LoadLibraryW
#endif
#ifdef GetMonitorInfo
#undef GetMonitorInfo
#endif
#ifdef GetEnvironmentVariable
#undef GetEnvironmentVariable
#endif
#ifdef SetEnvironmentVariable
#undef SetEnvironmentVariable
#endif
#ifdef GetComputerName
#undef GetComputerName
#endif
#ifdef GetUserName
#undef GetUserName
#endif
#ifdef CreateEvent
#undef CreateEvent
#endif
#ifdef CreateEventA
#undef CreateEventA
#endif
#ifdef CreateEventW
#undef CreateEventW
#endif
#ifdef MessageBox
#undef MessageBox
#endif
#ifdef MessageBoxA
#undef MessageBoxA
#endif
#ifdef MessageBoxW
#undef MessageBoxW
#endif
