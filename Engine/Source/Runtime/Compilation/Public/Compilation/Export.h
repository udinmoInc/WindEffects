// ==============================================================================
// WindEffects — Compilation — Export
// Public API surface for the Compilation module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
