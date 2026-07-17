#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "KindUI/Core/Widget.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Widgets/Label.h"
#include "Renderer/EditorRenderDebugStub.h"
#include <functional>
#include <memory>
#include <string>

namespace we::runtime::kindui {

/// Foundation stub: full investigation UI is disabled under the current renderer.
class UIFRAMEWORK_API RenderInvestigationModal : public Widget {
public:
    RenderInvestigationModal();
    ~RenderInvestigationModal() override = default;

    void Construct();

    Size Measure(const Size& availableSize) override;
    void Arrange(const Rect& allottedRect) override;
    void Paint(PaintContext& context) override;
    void OnMouseWheel(const MouseEvent& event) override;
    void OnKeyDown(const KeyEvent& event) override;
    void OnMouseDown(const MouseEvent& event) override;

    void UpdateFromSnapshot(const we::runtime::renderer::RenderDebuggerFrameSnapshot& snapshot);
    void SetOnClose(std::function<void()> callback) { m_OnClose = std::move(callback); }

private:
    void BuildLayout();
    void RebuildPassList();
    void RebuildTargetList();
    void RefreshResourcePanel();
    void RefreshPixelPanel();
    void RefreshShaderPanel();
    void RefreshFailureBanner();
    void RefreshGraphPanel();
    void RefreshWarnings();
    void RefreshTexturePreview();
    void SelectPass(int index);
    void SelectTarget(int index);
    void StepPass(int delta);
    void CycleViewMode(int delta);
    bool PassMatchesSearch(const std::string& name) const;
    bool TargetMatchesSearch(const std::string& name) const;
    const we::runtime::renderer::GpuPassValidation* GetSelectedPass() const;
    const we::runtime::renderer::GpuRenderTargetInfo* GetSelectedTarget() const;

    we::runtime::renderer::RenderDebuggerFrameSnapshot m_Snapshot{};
    std::function<void()> m_OnClose;
    std::shared_ptr<Column> m_Root;
    std::shared_ptr<Label> m_TitleLabel;
};

class UIFRAMEWORK_API RenderInvestigationModalHost {
public:
    static void Toggle();
    static void Show();
    static void Hide();
    static bool IsVisible();
    static void UpdateFromSnapshot(const we::runtime::renderer::RenderDebuggerFrameSnapshot& snapshot);
};

} // namespace we::runtime::kindui
