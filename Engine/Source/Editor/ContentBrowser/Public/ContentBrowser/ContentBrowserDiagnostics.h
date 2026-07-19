#pragma once

#include "ContentBrowser/Export.h"

#include <atomic>
#include <cstdint>
#include <string>

namespace we::editor::contentbrowser {

struct CONTENTBROWSER_API ContentBrowserDiagnosticsSnapshot {
    std::uint64_t refreshCount = 0;
    std::uint64_t selectionChanges = 0;
    std::uint64_t commandCount = 0;
    std::uint64_t transactionsRecorded = 0;
    std::uint64_t searchPasses = 0;
    std::uint64_t thumbnailRequests = 0;
    std::uint64_t visibleAssets = 0;
    std::uint64_t assetCount = 0;
    std::uint64_t refreshMicros = 0;
};

class CONTENTBROWSER_API ContentBrowserDiagnostics {
public:
    static ContentBrowserDiagnostics& Get() noexcept;
    void Reset() noexcept;
    [[nodiscard]] ContentBrowserDiagnosticsSnapshot Snapshot() const noexcept;
    [[nodiscard]] std::string FormatSummary() const;

    void OnRefresh(std::uint64_t micros, std::uint64_t assets, std::uint64_t visible) noexcept;
    void OnSelectionChanged(std::uint64_t count) noexcept;
    void OnCommand() noexcept;
    void OnTransaction() noexcept;
    void OnSearchPass() noexcept;
    void OnThumbnailRequest() noexcept;

private:
    std::atomic<std::uint64_t> m_Refresh{0};
    std::atomic<std::uint64_t> m_Selection{0};
    std::atomic<std::uint64_t> m_Commands{0};
    std::atomic<std::uint64_t> m_Transactions{0};
    std::atomic<std::uint64_t> m_Search{0};
    std::atomic<std::uint64_t> m_Thumbs{0};
    std::atomic<std::uint64_t> m_Visible{0};
    std::atomic<std::uint64_t> m_Assets{0};
    std::atomic<std::uint64_t> m_RefreshMicros{0};
};

} // namespace we::editor::contentbrowser
