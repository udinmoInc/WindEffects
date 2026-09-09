// ==============================================================================
// WindEffects — TerrainEditor — Export
// Public API surface for the TerrainEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#if defined(_WIN32)
#if defined(TERRAINEDITOR_EXPORTS)
#define TERRAINEDITOR_API __declspec(dllexport)
#else
#define TERRAINEDITOR_API __declspec(dllimport)
#endif
#else
#define TERRAINEDITOR_API
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif

namespace we::runtime::terrain {}
namespace we::runtime::scene {}
namespace we::runtime::world {}
namespace we::runtime::reflection {}
namespace we::editor::undo {}
namespace we::editor::viewportedit {}
namespace we::editor::property {}

namespace we::editor::terrain {
namespace runtime_terrain = ::we::runtime::terrain;
namespace scene = ::we::runtime::scene;
namespace world = ::we::runtime::world;
namespace reflection = ::we::runtime::reflection;
namespace undo = ::we::editor::undo;
namespace viewportedit = ::we::editor::viewportedit;
namespace property = ::we::editor::property;
} // namespace we::editor::terrain
