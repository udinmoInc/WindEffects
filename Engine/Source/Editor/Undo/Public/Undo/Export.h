#pragma once

#if defined(_WIN32)
#if defined(UNDO_EXPORTS)
#define UNDO_API __declspec(dllexport)
#else
#define UNDO_API __declspec(dllimport)
#endif
#else
#define UNDO_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif

namespace we::runtime::reflection {}
namespace we::runtime::serialization {}
namespace we::runtime::world {}
namespace we::editor::property {}

namespace we::editor::undo {
namespace reflection = ::we::runtime::reflection;
namespace serialization = ::we::runtime::serialization;
namespace world = ::we::runtime::world;
namespace property = ::we::editor::property;
} // namespace we::editor::undo
