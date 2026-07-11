#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "Core/Widget.h"
#include "Layout/Box.h"
#include "Layout/ScrollLayout.h"
#include "Layout/Splitter.h"
#include "Widgets/Button.h"
#include "Widgets/Label.h"
#include "Widgets/TextBox.h"
#include "Renderer/EditorRenderDebugStub.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace WindEffects::Editor::UI {

enum class InvestigationTextureViewMode {
    RGB,
    Alpha,
    Red,
    Green,
    Blue,
    HdrHeatmap,
    Luminance,
    FalseColor,
    Histogram
};

class UIFRAMEWORK_API RenderInvestigationModal : public Widget {
public:
    RenderInvestigationModal();
    ~RenderInvestigationModal() override = default;

    /// Must be called after the widget is owned by std::shared_ptr.
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
    uint64_t m_LastFrameNumber = UINT64_MAX;
    size_t m_LastPassCount = 0;
    int m_LastRefreshPassIndex = -1;
    int m_LastRefreshTargetIndex = -1;
    InvestigationTextureViewMode m_LastRefreshViewMode = InvestigationTextureViewMode::RGB;
    int m_SelectedPassIndex = 0;
    int m_SelectedTargetIndex = 0;
    int m_StepPassIndex = -1;
    InvestigationTextureViewMode m_ViewMode = InvestigationTextureViewMode::RGB;
    std::string m_SearchQuery;

    std::function<void()> m_OnClose;

    std::shared_ptr<VerticalBox> m_Root;
    std::shared_ptr<HorizontalBox> m_Header;
    std::shared_ptr<Label> m_TitleLabel;
    std::shared_ptr<Label> m_FrameLabel;
    std::shared_ptr<TextBox> m_SearchBox;
    std::shared_ptr<Button> m_CloseButton;
    std::shared_ptr<Button> m_CaptureButton;
    std::shared_ptr<Button> m_CompareButton;
    std::shared_ptr<Button> m_StepBackButton;
    std::shared_ptr<Button> m_StepFwdButton;
    std::shared_ptr<Label> m_StepLabel;

    std::shared_ptr<Splitter> m_MainSplitter;
    std::shared_ptr<Splitter> m_CenterRightSplitter;
    std::shared_ptr<Splitter> m_MainBottomSplitter;

    std::shared_ptr<ScrollLayout> m_PassScroll;
    std::shared_ptr<VerticalBox> m_PassList;
    std::vector<std::shared_ptr<Button>> m_PassButtons;

    std::shared_ptr<VerticalBox> m_CenterColumn;
    std::shared_ptr<ScrollLayout> m_TargetScroll;
    std::shared_ptr<HorizontalBox> m_TargetRow;
    std::vector<std::shared_ptr<Button>> m_TargetButtons;
    std::shared_ptr<Widget> m_TextureCanvas;

    std::shared_ptr<ScrollLayout> m_ResourceScroll;
    std::shared_ptr<Label> m_ResourceLabel;

    std::shared_ptr<ScrollLayout> m_BottomScroll;
    std::shared_ptr<Label> m_FailureLabel;
    std::shared_ptr<Label> m_PixelLabel;
    std::shared_ptr<Label> m_ShaderLabel;
    std::shared_ptr<Label> m_GraphLabel;
    std::shared_ptr<Label> m_WarningsLabel;
    std::shared_ptr<VerticalBox> m_BottomColumn;

    std::shared_ptr<HorizontalBox> m_ViewModeRow;
    std::vector<std::shared_ptr<Button>> m_ViewModeButtons;
};

class UIFRAMEWORK_API RenderInvestigationModalHost {
public:
    static void Toggle();
    static void Show();
    static void Hide();
    static bool IsVisible();
    static void UpdateFromSnapshot(const we::runtime::renderer::RenderDebuggerFrameSnapshot& snapshot);
};

} // namespace WindEffects::Editor::UI
