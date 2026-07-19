#pragma once

#if defined(_WIN32)
#if defined(COMPILATION_EXPORTS)
#define COMPILATION_API __declspec(dllexport)
#else
#define COMPILATION_API __declspec(dllimport)
#endif
#else
#define COMPILATION_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif

namespace we::runtime::reflection {}
namespace we::runtime::serialization {}
namespace we::runtime::assetruntime {}
namespace we::runtime::assetimporter {}

namespace we::runtime::compilation {
namespace reflection = ::we::runtime::reflection;
namespace serialization = ::we::runtime::serialization;
namespace assetruntime = ::we::runtime::assetruntime;
namespace assetimporter = ::we::runtime::assetimporter;
} // namespace we::runtime::compilation
