#pragma once

#if defined(_WIN32)
#if defined(TERRAIN_EXPORTS)
#define TERRAIN_API __declspec(dllexport)
#else
#define TERRAIN_API __declspec(dllimport)
#endif
#else
#define TERRAIN_API
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
namespace we::rhi {
class IRHIDevice;
class IRHICommandList;
}

namespace we::runtime::terrain {
namespace reflection = ::we::runtime::reflection;
namespace serialization = ::we::runtime::serialization;
namespace world = ::we::runtime::world;
namespace scene = ::we::runtime::scene;
namespace ecs = ::we::runtime::ecs;
namespace assetruntime = ::we::runtime::assetruntime;
namespace rhi = ::we::rhi;
} // namespace we::runtime::terrain
