#pragma once

#if defined(_WIN32)
#if defined(PREFABEDITOR_EXPORTS)
#define PREFABEDITOR_API __declspec(dllexport)
#else
#define PREFABEDITOR_API __declspec(dllimport)
#endif
#else
#define PREFABEDITOR_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif

namespace we::runtime::prefab {}
namespace we::editor::viewportedit {}
namespace we::editor::outliner {}
namespace we::editor::contentbrowser {}

namespace we::editor::prefab {
namespace runtime = ::we::runtime::prefab;
namespace viewportedit = ::we::editor::viewportedit;
namespace outliner = ::we::editor::outliner;
namespace contentbrowser = ::we::editor::contentbrowser;
} // namespace we::editor::prefab
