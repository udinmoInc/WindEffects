#pragma once

#if defined(_WIN32)
#if defined(VIEWPORTEDIT_EXPORTS)
#define VIEWPORTEDIT_API __declspec(dllexport)
#else
#define VIEWPORTEDIT_API __declspec(dllimport)
#endif
#else
#define VIEWPORTEDIT_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif

namespace we::runtime::reflection {}
namespace we::runtime::serialization {}
namespace we::runtime::world {}
namespace we::runtime::scene {}
namespace we::runtime::engine {}
namespace we::editor::undo {}
namespace we::editor::property {}
namespace we::editor::viewport {}

namespace we::editor::viewportedit {
namespace reflection = ::we::runtime::reflection;
namespace serialization = ::we::runtime::serialization;
namespace world = ::we::runtime::world;
namespace scene = ::we::runtime::scene;
namespace engine = ::we::runtime::engine;
namespace undo = ::we::editor::undo;
namespace property = ::we::editor::property;
namespace viewport = ::we::editor::viewport;
} // namespace we::editor::viewportedit
