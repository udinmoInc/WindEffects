#pragma once

#if defined(_WIN32)
#if defined(CONTENTBROWSER_EXPORTS)
#define CONTENTBROWSER_API __declspec(dllexport)
#else
#define CONTENTBROWSER_API __declspec(dllimport)
#endif
#else
#define CONTENTBROWSER_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif

namespace we::runtime::reflection {}
namespace we::runtime::serialization {}
namespace we::runtime::assetruntime {}
namespace we::runtime::assetimporter {}
namespace we::runtime::assetpipeline {}
namespace we::editor::undo {}
namespace we::editor::property {}
namespace we::editor::viewportedit {}

namespace we::editor::contentbrowser {
namespace reflection = ::we::runtime::reflection;
namespace serialization = ::we::runtime::serialization;
namespace assetruntime = ::we::runtime::assetruntime;
namespace assetimporter = ::we::runtime::assetimporter;
namespace assetpipeline = ::we::runtime::assetpipeline;
namespace undo = ::we::editor::undo;
namespace property = ::we::editor::property;
namespace viewportedit = ::we::editor::viewportedit;
} // namespace we::editor::contentbrowser
