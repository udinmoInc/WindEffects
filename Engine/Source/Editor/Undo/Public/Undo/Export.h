// ==============================================================================
// WindEffects — Undo — Export
// Public API surface for the Undo module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
