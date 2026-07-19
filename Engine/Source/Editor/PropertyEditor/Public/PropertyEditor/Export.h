#pragma once

#if defined(_WIN32)
#if defined(PROPERTYEDITOR_EXPORTS)
#define PROPERTYEDITOR_API __declspec(dllexport)
#else
#define PROPERTYEDITOR_API __declspec(dllimport)
#endif
#else
#define PROPERTYEDITOR_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif

// Shorthand used throughout PropertyEditor public/private headers.
namespace we::runtime::reflection {}
namespace we::runtime::serialization {}

namespace we::editor::property {
namespace reflection = ::we::runtime::reflection;
namespace serialization = ::we::runtime::serialization;
} // namespace we::editor::property
