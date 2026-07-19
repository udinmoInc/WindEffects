#pragma once

#if defined(_WIN32)
#if defined(PREFAB_EXPORTS)
#define PREFAB_API __declspec(dllexport)
#else
#define PREFAB_API __declspec(dllimport)
#endif
#else
#define PREFAB_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif

namespace we::runtime::reflection {}
namespace we::runtime::serialization {}
namespace we::runtime::world {}
namespace we::runtime::scene {}
namespace we::runtime::ecs {}
namespace we::runtime::assetruntime {}
namespace we::runtime::assetimporter {}

namespace we::runtime::prefab {
namespace reflection = ::we::runtime::reflection;
namespace serialization = ::we::runtime::serialization;
namespace world = ::we::runtime::world;
namespace scene = ::we::runtime::scene;
namespace ecs = ::we::runtime::ecs;
namespace assetruntime = ::we::runtime::assetruntime;
namespace assetimporter = ::we::runtime::assetimporter;
} // namespace we::runtime::prefab
