#include "WindEffects/Editor/UI/Widgets/RenderInvestigationModal.h"
#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"

namespace we::editor::panels {

namespace {

std::shared_ptr<RenderInvestigationModal> g_Modal;

} // namespace

RenderInvestigationModal::RenderInvestigationModal() = default;

void RenderInvestigationModal::Construct() {
    m_Root = std::make_shared<Column>();
    m_TitleLabel = std::make_shared<Label>("Render Investigation");
    m_Root->AddChild(m_TitleLabel);
    AddChild(m_Root);
}

Size RenderInvestigationModal::Measure(const Size& availableSize) {
    m_DesiredSize = availableSize;
    return m_DesiredSize;
}

void RenderInvestigationModal::Arrange(const Rect& allottedRect) {
    m_Geometry = allottedRect;
    if (m_Root) {
        m_Root->Measure(Size{ allottedRect.width, allottedRect.height });
        m_Root->Arrange(allottedRect);
    }
}

void RenderInvestigationModal::Paint(PaintContext& context) {
    if (!m_Visible) {
        return;
    }
    if (m_Root) {
        m_Root->Paint(context);
    }
}

void RenderInvestigationModal::OnMouseWheel(const MouseEvent& /*event*/) {}
void RenderInvestigationModal::OnKeyDown(const KeyEvent& /*event*/) {}
void RenderInvestigationModal::OnMouseDown(const MouseEvent& /*event*/) {}

void RenderInvestigationModal::UpdateFromSnapshot(const we::runtime::renderer::RenderDebuggerFrameSnapshot& snapshot) {
    m_Snapshot = snapshot;
}

void RenderInvestigationModal::BuildLayout() {}
void RenderInvestigationModal::RebuildPassList() {}
void RenderInvestigationModal::RebuildTargetList() {}
void RenderInvestigationModal::RefreshResourcePanel() {}
void RenderInvestigationModal::RefreshPixelPanel() {}
void RenderInvestigationModal::RefreshShaderPanel() {}
void RenderInvestigationModal::RefreshFailureBanner() {}
void RenderInvestigationModal::RefreshGraphPanel() {}
void RenderInvestigationModal::RefreshWarnings() {}
void RenderInvestigationModal::RefreshTexturePreview() {}
void RenderInvestigationModal::SelectPass(int /*index*/) {}
void RenderInvestigationModal::SelectTarget(int /*index*/) {}
void RenderInvestigationModal::StepPass(int /*delta*/) {}
void RenderInvestigationModal::CycleViewMode(int /*delta*/) {}
bool RenderInvestigationModal::PassMatchesSearch(const std::string& /*name*/) const { return true; }
bool RenderInvestigationModal::TargetMatchesSearch(const std::string& /*name*/) const { return true; }
const we::runtime::renderer::GpuPassValidation* RenderInvestigationModal::GetSelectedPass() const { return nullptr; }
const we::runtime::renderer::GpuRenderTargetInfo* RenderInvestigationModal::GetSelectedTarget() const { return nullptr; }

void RenderInvestigationModalHost::Show() {
    if (!g_Modal) {
        g_Modal = std::make_shared<RenderInvestigationModal>();
        g_Modal->Construct();
    }
    if (auto* overlayMgr = we::programs::editor::GetEditorPopupHost()) {
        overlayMgr->ShowFullscreenPopup(g_Modal);
    }
}

void RenderInvestigationModalHost::Hide() {
    if (auto* overlayMgr = we::programs::editor::GetEditorPopupHost()) {
        overlayMgr->CloseAllPopups();
    }
}

void RenderInvestigationModalHost::Toggle() {
    if (IsVisible()) {
        Hide();
    } else {
        Show();
    }
}

bool RenderInvestigationModalHost::IsVisible() {
    return g_Modal && g_Modal->IsVisible();
}

void RenderInvestigationModalHost::UpdateFromSnapshot(const we::runtime::renderer::RenderDebuggerFrameSnapshot& snapshot) {
    if (g_Modal) {
        g_Modal->UpdateFromSnapshot(snapshot);
    }
}

} // namespace we::editor::panels
