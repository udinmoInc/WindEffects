#pragma once

#include "ContentBrowser/Export.h"
#include "ContentBrowser/IContentBrowser.h"

#include <memory>

namespace we::editor::contentbrowser {

/// Editor-session binding for panel factories that cannot take constructor DI.
class CONTENTBROWSER_API ContentBrowserSession {
public:
    static void Install(std::shared_ptr<IContentBrowserRuntime> runtime);
    static void Clear() noexcept;

    [[nodiscard]] static IContentBrowserRuntime* Runtime() noexcept;
    [[nodiscard]] static IContentBrowser* Browser() noexcept;
    [[nodiscard]] static std::shared_ptr<IContentBrowserRuntime> RuntimeShared() noexcept;
    [[nodiscard]] static bool IsInstalled() noexcept;
};

} // namespace we::editor::contentbrowser
