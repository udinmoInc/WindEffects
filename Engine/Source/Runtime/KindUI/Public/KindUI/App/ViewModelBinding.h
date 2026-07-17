#pragma once

#include "KindUI/App/ViewHost.h"
#include "KindUI/Core/Observable.h"

namespace we::runtime::kindui {

/// Convenience helpers for wiring ViewModels to ViewHost refresh.
inline void BindViewRefresh(ViewHost& host, auto&... sources) {
    (host.Observe(sources), ...);
}

inline void BindViewRefreshList(ViewHost& host, auto&... lists) {
    (host.ObserveList(lists), ...);
}

} // namespace we::runtime::kindui
