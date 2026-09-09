// ==============================================================================
// WindEffects — PropertyEditor — Export
// Public API surface for the PropertyEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
