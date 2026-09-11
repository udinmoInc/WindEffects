// ==============================================================================
// WindEffects — Core — LoopExecutionTrace
// Opt-in (WE_LOOP_TRACE=1) entry/exit + duration probes for editor loop diagnosis.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Core/Export.h"

#include <cstdint>
#include <string>
#include <string_view>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

namespace we::runtime::core {

/// Thread-local scoped / free-function tracers for message-pump ↔ Vulkan desync.
/// Enable with environment variable WE_LOOP_TRACE=1 (any non-empty non-"0" value).
class CORE_API LoopExecutionTrace {
public:
    [[nodiscard]] static bool IsEnabled();

    static void Enter(std::string_view name, std::string_view detail = {});
    static void Exit(std::string_view name, std::string_view detail = {});
    static void Event(std::string_view name, std::string_view detail = {});
    static void MutexWait(std::string_view name, double waitMs, double warnMs = 1.0);
    static void GateState(std::string_view where,
        bool needsLayout,
        bool needsPaint,
        size_t frameEvents,
        size_t pendingHint = 0);

    struct CORE_API Scoped {
        explicit Scoped(std::string_view name, std::string_view detail = {});
        ~Scoped();
        Scoped(const Scoped&) = delete;
        Scoped& operator=(const Scoped&) = delete;
    private:
        std::string m_Name;
        bool m_Active = false;
    };
};

} // namespace we::runtime::core

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
