#include "ContentBrowser/ContentBrowserDiagnostics.h"

#include <sstream>

namespace we::editor::contentbrowser {

ContentBrowserDiagnostics& ContentBrowserDiagnostics::Get() noexcept {
    static ContentBrowserDiagnostics instance;
    return instance;
}

void ContentBrowserDiagnostics::Reset() noexcept {
    m_Refresh.store(0);
    m_Selection.store(0);
    m_Commands.store(0);
    m_Transactions.store(0);
    m_Search.store(0);
    m_Thumbs.store(0);
    m_Visible.store(0);
    m_Assets.store(0);
    m_RefreshMicros.store(0);
}

ContentBrowserDiagnosticsSnapshot ContentBrowserDiagnostics::Snapshot() const noexcept {
    ContentBrowserDiagnosticsSnapshot s;
    s.refreshCount = m_Refresh.load();
    s.selectionChanges = m_Selection.load();
    s.commandCount = m_Commands.load();
    s.transactionsRecorded = m_Transactions.load();
    s.searchPasses = m_Search.load();
    s.thumbnailRequests = m_Thumbs.load();
    s.visibleAssets = m_Visible.load();
    s.assetCount = m_Assets.load();
    s.refreshMicros = m_RefreshMicros.load();
    return s;
}

std::string ContentBrowserDiagnostics::FormatSummary() const {
    const auto s = Snapshot();
    std::ostringstream oss;
    oss << "ContentBrowser refresh=" << s.refreshCount
        << " assets=" << s.assetCount
        << " visible=" << s.visibleAssets
        << " sel=" << s.selectionChanges
        << " cmds=" << s.commandCount
        << " txns=" << s.transactionsRecorded
        << " thumbs=" << s.thumbnailRequests
        << " refreshUs=" << s.refreshMicros;
    return oss.str();
}

void ContentBrowserDiagnostics::OnRefresh(std::uint64_t micros, std::uint64_t assets, std::uint64_t visible) noexcept {
    m_Refresh.fetch_add(1);
    m_RefreshMicros.fetch_add(micros);
    m_Assets.store(assets);
    m_Visible.store(visible);
}

void ContentBrowserDiagnostics::OnSelectionChanged(std::uint64_t count) noexcept {
    m_Selection.fetch_add(1);
    (void)count;
}

void ContentBrowserDiagnostics::OnCommand() noexcept { m_Commands.fetch_add(1); }
void ContentBrowserDiagnostics::OnTransaction() noexcept { m_Transactions.fetch_add(1); }
void ContentBrowserDiagnostics::OnSearchPass() noexcept { m_Search.fetch_add(1); }
void ContentBrowserDiagnostics::OnThumbnailRequest() noexcept { m_Thumbs.fetch_add(1); }

} // namespace we::editor::contentbrowser
