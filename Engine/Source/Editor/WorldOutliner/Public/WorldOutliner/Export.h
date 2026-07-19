#pragma once

#if defined(_WIN32)
#if defined(WORLDOUTLINER_EXPORTS)
#define WORLDOUTLINER_API __declspec(dllexport)
#else
#define WORLDOUTLINER_API __declspec(dllimport)
#endif
#else
#define WORLDOUTLINER_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif

namespace we::runtime::reflection {}
namespace we::runtime::serialization {}
namespace we::runtime::world {}
namespace we::runtime::scene {}
namespace we::editor::undo {}
namespace we::editor::property {}
namespace we::editor::viewportedit {}

namespace we::editor::outliner {
namespace reflection = ::we::runtime::reflection;
namespace serialization = ::we::runtime::serialization;
namespace world = ::we::runtime::world;
namespace scene = ::we::runtime::scene;
namespace undo = ::we::editor::undo;
namespace property = ::we::editor::property;
namespace viewportedit = ::we::editor::viewportedit;
} // namespace we::editor::outliner
