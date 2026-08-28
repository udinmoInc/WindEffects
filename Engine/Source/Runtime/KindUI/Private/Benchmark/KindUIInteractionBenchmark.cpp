#include "KindUI/Benchmark/KindUIInteractionBenchmark.h"

#include "KindUI/Core/EventSystem.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Input/InputEvents.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Layout/OverlayManager.h"
#include "KindUI/Layout/ScrollLayout.h"
#include "KindUI/Layout/Splitter.h"
#include "KindUI/Profiling/UiInputLatencyAudit.h"
#include "KindUI/Profiling/UiPathDiagnostics.h"
#include "KindUI/Theming/DefaultTheme.h"
#include "KindUI/Theming/ThemeManager.h"
#include "KindUI/Widgets/Label.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <sstream>

namespace we::runtime::kindui {
namespace {

using clock = std::chrono::steady_clock;

struct EditorLikeShell {
    std::shared_ptr<OverlayHost> host;
    std::shared_ptr<Column> root;
    std::shared_ptr<Splitter> toolsViewportSplit;
    std::shared_ptr<Splitter> explorerSplit;
    std::shared_ptr<Column> toolsPanel;
    std::shared_ptr<Column> viewportPanel;
    std::shared_ptr<Column> explorerPanel;
    std::shared_ptr<Column> contentPanel;
};

double NowMs() {
    return std::chrono::duration<double, std::milli>(clock::now().time_since_epoch()).count();
}

EditorLikeShell BuildShell() {
    EditorLikeShell shell{};
    shell.host = std::make_shared<OverlayHost>();
    shell.root = std::make_shared<Column>();
    shell.root->SetFlexGrow(1.0f);

    shell.toolsPanel = std::make_shared<Column>();
    shell.toolsPanel->Gap(4.0f);
    for (int i = 0; i < 24; ++i) {
        shell.toolsPanel->AddChild(std::make_shared<PropertyRow>("Tool " + std::to_string(i), "On"));
    }

    shell.viewportPanel = std::make_shared<Column>();
    shell.viewportPanel->SetFlexGrow(1.0f);
    for (int i = 0; i < 8; ++i) {
        shell.viewportPanel->AddChild(std::make_shared<Label>("Viewport chrome " + std::to_string(i)));
    }

    shell.explorerPanel = std::make_shared<Column>();
    for (int i = 0; i < 48; ++i) {
        shell.explorerPanel->AddChild(std::make_shared<Label>("Tree node " + std::to_string(i)));
    }

    shell.contentPanel = std::make_shared<Column>();
    for (int i = 0; i < 32; ++i) {
        shell.contentPanel->AddChild(std::make_shared<PropertyRow>("Asset " + std::to_string(i), "File"));
    }

    shell.explorerSplit = std::make_shared<Splitter>(Orientation::Horizontal, 0.72f);
    shell.explorerSplit->SetFirstChild(shell.viewportPanel);
    shell.explorerSplit->SetSecondChild(shell.explorerPanel);
    shell.explorerSplit->SetFlexGrow(1.0f);
    shell.explorerSplit->SetMinPaneSizes(200.0f, 180.0f);

    shell.toolsViewportSplit = std::make_shared<Splitter>(Orientation::Horizontal, 0.18f);
    shell.toolsViewportSplit->SetFirstChild(shell.toolsPanel);
    shell.toolsViewportSplit->SetSecondChild(shell.explorerSplit);
    shell.toolsViewportSplit->SetFlexGrow(1.0f);
    shell.toolsViewportSplit->SetMinPaneSizes(160.0f, 400.0f);

    shell.root->AddChild(shell.toolsViewportSplit);
    shell.root->AddChild(shell.contentPanel);
    shell.host->SetBaseWidget(shell.root);
    return shell;
}

void LayoutTree(const std::shared_ptr<Widget>& root, float width, float height) {
    UiPathDiagnostics::Get().OnLayoutPass();
    root->Measure(Size{width, height});
    root->Arrange(Rect{0.0f, 0.0f, width, height});
    root->ClearSubtreeLayoutDirty();
}

void PaintTree(const std::shared_ptr<Widget>& root) {
    UiPathDiagnostics::Get().OnPaintPass();
    PaintContext ctx;
    root->Paint(ctx);
    UiPathDiagnostics::Get().SetPaintCommands(static_cast<uint32_t>(ctx.GetCommands().size()));
}

void ProcessInteractionFrame(EditorLikeShell& shell, float width, float height) {
    if (UIRepaintGate::ConsumeNeedsLayout()) {
        LayoutTree(shell.host, width, height);
    }
    if (UIRepaintGate::ConsumeNeedsPaint()) {
        PaintTree(shell.host);
        shell.host->ClearSubtreePaintDirty();
    }
}

void FullUiFrame(EditorLikeShell& shell, float width, float height) {
    LayoutTree(shell.host, width, height);
    PaintTree(shell.host);
    shell.host->ClearSubtreePaintDirty();
}

InteractionScenarioResult RunScenario(
    const char* name,
    const uint32_t steps,
    const std::function<void(EditorLikeShell&, float, float, uint32_t)>& fn) {
    InteractionScenarioResult result{};
    result.name = name;

    Widget::s_GlobalDiagnostics = new Widget::Diagnostics();
    UiPathDiagnostics::SetBenchmarkActive(true);
    UiPathDiagnostics::Get().ResetPeak();

    ThemeManager::Get().Initialize(std::make_shared<DefaultTheme>(), 1.0f);
    auto shell = BuildShell();
    constexpr float width = 1280.0f;
    constexpr float height = 720.0f;
    FullUiFrame(shell, width, height);

    UIRepaintGate::BeginFrame();
    Widget::ResetDiagnostics();

    double totalMs = 0.0;
    double peakMs = 0.0;
    uint32_t peakInvalidate = 0;
    for (uint32_t step = 0; step < steps; ++step) {
        UIRepaintGate::BeginFrame();
        Widget::ResetDiagnostics();
        const double t0 = NowMs();
        fn(shell, width, height, step);
        const double dt = NowMs() - t0;
        totalMs += dt;
        peakMs = std::max(peakMs, dt);
        if (Widget::s_GlobalDiagnostics) {
            peakInvalidate = std::max(peakInvalidate, Widget::s_GlobalDiagnostics->invalidateCount);
        }
    }

    const auto& peak = UiPathDiagnostics::Get().Peak();
    result.peakMs = peakMs;
    result.avgMs = steps > 0 ? totalMs / static_cast<double>(steps) : 0.0;
    result.layoutPasses = peak.layoutPasses;
    result.paintPasses = peak.paintPasses;
    result.invalidateCount = peakInvalidate;
    result.layoutInvalidations = peak.layoutInvalidations;
    result.paintInvalidations = peak.paintInvalidations;
    result.widgetsVisited = peak.widgetsVisited;
    result.paintCommands = peak.paintCommands;

    delete Widget::s_GlobalDiagnostics;
    Widget::s_GlobalDiagnostics = nullptr;
    UiPathDiagnostics::SetBenchmarkActive(false);
    return result;
}

std::shared_ptr<Column> BuildDropdownMenu() {
    auto menu = std::make_shared<Column>();
    menu->Gap(0.0f);
    for (int i = 0; i < 18; ++i) {
        menu->AddChild(MakeSecondaryAction("Menu item " + std::to_string(i)));
    }
    return menu;
}

} // namespace

KindUIInteractionReport RunKindUIInteractionBenchmark(const uint32_t dragSteps) {
    KindUIInteractionReport report{};

    report.idleBaseline = RunScenario("idle", dragSteps, [](EditorLikeShell&, float, float, uint32_t) {
        (void)UIRepaintGate::ConsumeNeedsLayout();
        (void)UIRepaintGate::ConsumeNeedsPaint();
    });
    report.idleBaseline.rootCause = "gate consume only";

    auto splitter = RunScenario("splitter_drag", dragSteps, [dragSteps](EditorLikeShell& shell, float, float, uint32_t step) {
        auto split = shell.toolsViewportSplit;
        const float t = static_cast<float>(step + 1) / static_cast<float>(dragSteps);
        MouseEvent event{};
        event.type = MouseEventType::MouseMove;
        event.position = Point{200.0f + t * 500.0f, 360.0f};
        if (step == 0) {
            MouseEvent down{};
            down.type = MouseEventType::MouseDown;
            down.position = event.position;
            down.button = MouseButton::Left;
            split->OnMouseDown(down);
        }
        split->OnMouseMove(event);
        if (step + 1 == dragSteps) {
            MouseEvent up{};
            up.type = MouseEventType::MouseUp;
            up.position = event.position;
            up.button = MouseButton::Left;
            split->OnMouseUp(up);
        }

        ProcessInteractionFrame(shell, 1280.0f, 720.0f);
    });
    splitter.rootCause = "Splitter drag now uses local Arrange+InvalidatePaint (no root relayout)";
    report.scenarios.push_back(splitter);

    auto windowResize = RunScenario("window_resize", dragSteps, [](EditorLikeShell& shell, float, float, uint32_t step) {
        const float w = 1100.0f + static_cast<float>(step) * 8.0f;
        const float h = 680.0f + static_cast<float>(step) * 4.0f;
        UIRepaintGate::RequestLayout();
        FullUiFrame(shell, w, h);
    });
    windowResize.rootCause = "root Measure/Arrange on every resize";
    report.scenarios.push_back(windowResize);

    auto panelResize = RunScenario("panel_resize", dragSteps, [](EditorLikeShell& shell, float, float, uint32_t step) {
        shell.toolsViewportSplit->SetFixedFirstWidth(160.0f + static_cast<float>(step) * 6.0f);
        UIRepaintGate::RequestLayout();
        FullUiFrame(shell, 1280.0f, 720.0f);
    });
    panelResize.rootCause = "SetFixedFirstWidth + full relayout";
    report.scenarios.push_back(panelResize);

    auto popupOpen = RunScenario("popup_open", std::min(8u, dragSteps), [](EditorLikeShell& shell, float, float, uint32_t) {
        auto menu = BuildDropdownMenu();
        shell.host->ShowPopup(menu, Point{420.0f, 180.0f});
        UIRepaintGate::RequestPaint();
        PaintTree(shell.host);
    });
    popupOpen.rootCause = "OverlayHost::ShowPopup uses AttachOverlayChild (paint-only invalidation)";
    report.scenarios.push_back(popupOpen);

    auto popupClose = RunScenario("dropdown_close", std::min(8u, dragSteps), [](EditorLikeShell& shell, float, float, uint32_t) {
        if (!shell.host->HasOpenPopups()) {
            shell.host->ShowPopup(BuildDropdownMenu(), Point{420.0f, 180.0f});
        }
        shell.host->CloseTopPopup();
        ProcessInteractionFrame(shell, 1280.0f, 720.0f);
    });
    popupClose.rootCause = "DetachOverlayChild avoids layout cascade on close";
    report.scenarios.push_back(popupClose);

    auto dropdown = RunScenario("dropdown", std::min(8u, dragSteps), [](EditorLikeShell& shell, float, float, uint32_t) {
        auto menu = BuildDropdownMenu();
        shell.host->ShowPopup(menu, Point{300.0f, 120.0f});
        FullUiFrame(shell, 1280.0f, 720.0f);
    });
    dropdown.rootCause = "popup cached size retained across host Arrange";
    report.scenarios.push_back(dropdown);

    auto popupScroll = RunScenario("popup_scroll", dragSteps, [](EditorLikeShell& shell, float, float, uint32_t step) {
        static std::shared_ptr<Column> menu;
        if (step == 0) {
            menu = BuildDropdownMenu();
            shell.host->ShowPopup(menu, Point{360.0f, 140.0f});
            FullUiFrame(shell, 1280.0f, 720.0f);
        }
        menu->InvalidatePaint();
        UIRepaintGate::RequestPaint();
        ProcessInteractionFrame(shell, 1280.0f, 720.0f);
    });
    popupScroll.rootCause = "scroll InvalidatePaint only (no relayout)";
    report.scenarios.push_back(popupScroll);

    auto popupMouseMove = RunScenario("popup_mouse_move", dragSteps, [](EditorLikeShell& shell, float, float, uint32_t step) {
        static std::shared_ptr<Column> menu;
        if (step == 0) {
            menu = BuildDropdownMenu();
            shell.host->ShowPopup(menu, Point{360.0f, 140.0f});
            FullUiFrame(shell, 1280.0f, 720.0f);
        }
        MouseEvent move{};
        move.type = MouseEventType::MouseMove;
        move.position = Point{360.0f + static_cast<float>(step) * 4.0f, 150.0f + static_cast<float>(step)};
        menu->OnMouseMove(move);
        ProcessInteractionFrame(shell, 1280.0f, 720.0f);
    });
    popupMouseMove.rootCause = "hover tracking paint-only when item changes";
    report.scenarios.push_back(popupMouseMove);

    auto docking = RunScenario("docking", std::min(6u, dragSteps), [](EditorLikeShell& shell, float, float, uint32_t) {
        auto panel = std::make_shared<Column>();
        panel->AddChild(std::make_shared<Label>("Docked panel"));
        shell.root->AddChild(panel);
        UIRepaintGate::RequestLayout();
        FullUiFrame(shell, 1280.0f, 720.0f);
    });
    docking.rootCause = "InvalidateLayout deduped, no recursive parent propagation";
    report.scenarios.push_back(docking);

    auto undocking = RunScenario("undocking", std::min(6u, dragSteps), [](EditorLikeShell& shell, float, float, uint32_t) {
        auto panel = std::make_shared<Column>();
        panel->AddChild(std::make_shared<Label>("Floated panel"));
        shell.explorerSplit->SetSecondChild(panel);
        UIRepaintGate::RequestLayout();
        FullUiFrame(shell, 1280.0f, 720.0f);
        shell.host->ShowPopup(panel, Point{800.0f, 120.0f});
        ProcessInteractionFrame(shell, 1280.0f, 720.0f);
    });
    undocking.rootCause = "split child swap layout + popup attach paint-only";
    report.scenarios.push_back(undocking);

    // Baseline (pre-Phase-3) simulations for before/after comparison.
    auto splitterBaseline = RunScenario("splitter_drag_baseline", dragSteps, [dragSteps](EditorLikeShell& shell, float, float, uint32_t step) {
        auto split = shell.toolsViewportSplit;
        const float t = static_cast<float>(step + 1) / static_cast<float>(dragSteps);
        MouseEvent event{};
        event.type = MouseEventType::MouseMove;
        event.position = Point{200.0f + t * 500.0f, 360.0f};
        if (step == 0) {
            MouseEvent down{};
            down.type = MouseEventType::MouseDown;
            down.position = event.position;
            down.button = MouseButton::Left;
            split->OnMouseDown(down);
        }
        split->OnMouseMove(event);
        shell.root->InvalidateLayout();
        UIRepaintGate::Request();
        FullUiFrame(shell, 1280.0f, 720.0f);
    });
    splitterBaseline.rootCause = "OLD: InvalidateLayout per drag frame + full root relayout";
    report.scenarios.push_back(splitterBaseline);

    auto popupBaseline = RunScenario("popup_open_baseline", std::min(8u, dragSteps), [](EditorLikeShell& shell, float, float, uint32_t) {
        auto menu = BuildDropdownMenu();
        shell.host->AddChild(menu);
        UIRepaintGate::Request();
        FullUiFrame(shell, 1280.0f, 720.0f);
    });
    popupBaseline.rootCause = "OLD: AddChild popup + recursive InvalidateLayout cascade";
    report.scenarios.push_back(popupBaseline);

    std::ostringstream oss;
    oss << "UIInteractBench steps=" << dragSteps;
    for (const auto& s : report.scenarios) {
        oss << " | " << s.name << ":peak=" << static_cast<uint64_t>(s.peakMs * 1000.0) << "us"
            << ",layout=" << s.layoutPasses << ",paint=" << s.paintPasses
            << ",inv=" << s.invalidateCount;
    }
    report.summary = oss.str();
    return report;
}

UiLatencyScenarioResult RunLatencyScenario(
    const char* name,
    UiInteractionKind kind,
    const std::function<void(EditorLikeShell&, const std::shared_ptr<EventSystem>&)>& fn) {
    UiLatencyScenarioResult result{};
    result.name = name;

    UiInputLatencyAudit::SetBenchmarkActive(true);
    UiInputLatencyAudit::Get().FlushPendingReport();

    ThemeManager::Get().Initialize(std::make_shared<DefaultTheme>(), 1.0f);
    auto shell = BuildShell();
    auto events = std::make_shared<EventSystem>();
    events->SetRootWidget(shell.host);
    FullUiFrame(shell, 1280.0f, 720.0f);

    UIRepaintGate::BeginFrame();
    UiInputLatencyAudit::Get().BeginFrame(1);
    UiInputLatencyAudit::Get().OnOsEvent(kind);
    fn(shell, events);
    if (UIRepaintGate::PeekNeedsLayout()) {
        LayoutTree(shell.host, 1280.0f, 720.0f);
        UiInputLatencyAudit::Get().OnLayout();
    }
    if (UIRepaintGate::PeekNeedsPaint()) {
        PaintTree(shell.host);
        UiInputLatencyAudit::Get().OnUiBuild();
        shell.host->ClearSubtreePaintDirty();
    }
    UiInputLatencyAudit::Get().OnRenderSubmit();
    UiInputLatencyAudit::Get().OnGpuSubmit();
    UiInputLatencyAudit::Get().OnPresentStart();
    UiInputLatencyAudit::Get().OnPresentComplete(nullptr);

    const auto& sample = UiInputLatencyAudit::Get().Last();
    result.inputToHandlerMs = sample.inputToHandlerMs;
    result.handlerToPaintMs = sample.handlerToPaintMs;
    result.paintToPresentMs = sample.paintToPresentMs;
    result.totalVisibleMs = sample.inputToVisibleMs;

    UiInputLatencyAudit::SetBenchmarkActive(false);
    return result;
}

UiLatencyBenchmarkReport RunUiInputLatencyBenchmark(const uint32_t steps) {
    UiLatencyBenchmarkReport report{};
    (void)steps;

    auto hover = RunLatencyScenario("hover", UiInteractionKind::Hover, [](EditorLikeShell& shell, const std::shared_ptr<EventSystem>& es) {
        MouseEvent move{};
        move.type = MouseEventType::MouseMove;
        move.position = Point{420.0f, 180.0f};
        es->ProcessMouseEvent(move);
        (void)shell;
    });
    hover.rootCause = "EventSystem hover + paint-only invalidation";
    report.scenarios.push_back(hover);

    auto click = RunLatencyScenario("click", UiInteractionKind::Click, [](EditorLikeShell& shell, const std::shared_ptr<EventSystem>& es) {
        MouseEvent down{};
        down.type = MouseEventType::MouseDown;
        down.position = Point{300.0f, 120.0f};
        down.button = MouseButton::Left;
        es->ProcessMouseEvent(down);
        (void)shell;
    });
    click.rootCause = "click dispatch + paint";
    report.scenarios.push_back(click);

    auto splitter = RunLatencyScenario("splitter_drag", UiInteractionKind::SplitterDrag, [](EditorLikeShell& shell, const std::shared_ptr<EventSystem>& es) {
        (void)es;
        auto split = shell.toolsViewportSplit;
        MouseEvent down{};
        down.type = MouseEventType::MouseDown;
        down.position = Point{200.0f, 360.0f};
        down.button = MouseButton::Left;
        split->OnMouseDown(down);
        MouseEvent move{};
        move.type = MouseEventType::MouseMove;
        move.position = Point{420.0f, 360.0f};
        split->OnMouseMove(move);
    });
    splitter.rootCause = "local splitter arrange + paint";
    report.scenarios.push_back(splitter);

    auto popup = RunLatencyScenario("popup_open", UiInteractionKind::PopupOpen, [](EditorLikeShell& shell, const std::shared_ptr<EventSystem>&) {
        shell.host->ShowPopup(BuildDropdownMenu(), Point{420.0f, 180.0f});
    });
    popup.rootCause = "AttachOverlayChild paint-only attach";
    report.scenarios.push_back(popup);

    auto scroll = RunLatencyScenario("scroll", UiInteractionKind::Scroll, [](EditorLikeShell& shell, const std::shared_ptr<EventSystem>& es) {
        auto menu = BuildDropdownMenu();
        shell.host->ShowPopup(menu, Point{360.0f, 140.0f});
        MouseEvent wheel{};
        wheel.type = MouseEventType::MouseWheel;
        wheel.position = Point{360.0f, 150.0f};
        wheel.wheelDeltaY = -1.0f;
        es->ProcessMouseEvent(wheel);
    });
    scroll.rootCause = "wheel + paint invalidation";
    report.scenarios.push_back(scroll);

    std::ostringstream oss;
    oss << "UILatencyBench scenarios=" << report.scenarios.size();
    for (const auto& s : report.scenarios) {
        oss << " | " << s.name
            << ":total=" << static_cast<uint64_t>(s.totalVisibleMs * 1000.0) << "us"
            << ",in→handler=" << static_cast<uint64_t>(s.inputToHandlerMs * 1000.0) << "us"
            << ",handler→paint=" << static_cast<uint64_t>(s.handlerToPaintMs * 1000.0) << "us"
            << ",paint→present=" << static_cast<uint64_t>(s.paintToPresentMs * 1000.0) << "us";
    }
    report.summary = oss.str();
    return report;
}

HitTestAuditReport RunHitTestAudit() {
    HitTestAuditReport report{};
    ThemeManager::Get().Initialize(std::make_shared<DefaultTheme>(), 1.0f);

    auto record = [&](const char* name, bool passed, std::string detail) {
        report.cases.push_back(HitTestAuditResult{name, passed, std::move(detail)});
    };

    {
        auto shell = BuildShell();
        FullUiFrame(shell, 1280.0f, 720.0f);
        const auto toolsGeom = shell.toolsPanel->GetGeometry();
        const Point toolPoint{toolsGeom.x + 20.0f, toolsGeom.y + 40.0f};
        const auto hit = EventSystem::HitTest(shell.host, toolPoint);
        const bool passed = hit && hit != shell.toolsPanel && hit != shell.toolsViewportSplit;
        record("sidebar_child", passed, passed ? hit->GetId() : "no deep child");
    }

    {
        auto shell = BuildShell();
        FullUiFrame(shell, 1280.0f, 720.0f);
        const auto barHit = shell.toolsViewportSplit->GetSplitterHitRect();
        const Point splitPoint{
            barHit.x + barHit.width * 0.5f,
            barHit.y + barHit.height * 0.5f
        };
        const auto hit = EventSystem::HitTest(shell.toolsViewportSplit, splitPoint);
        record(
            "splitter_bar",
            hit.get() == shell.toolsViewportSplit.get(),
            hit.get() == shell.toolsViewportSplit.get() ? "splitter" : "other");
    }

    {
        auto scroll = std::make_shared<ScrollLayout>();
        auto content = std::make_shared<Column>();
        for (int i = 0; i < 40; ++i) {
            content->AddChild(std::make_shared<Label>("Row " + std::to_string(i)));
        }
        scroll->SetContent(content);
        scroll->Measure(Size{300.0f, 120.0f});
        scroll->Arrange(Rect{100.0f, 100.0f, 300.0f, 120.0f});
        scroll->SetScrollOffset(200.0f);
        scroll->Arrange(Rect{100.0f, 100.0f, 300.0f, 120.0f});

        const Point visible{150.0f, 150.0f};
        const Point clipped{150.0f, 90.0f};
        const auto visibleHit = EventSystem::HitTest(scroll, visible);
        const auto clippedHit = EventSystem::HitTest(scroll, clipped);
        record(
            "scroll_clip",
            visibleHit != nullptr && clippedHit == nullptr,
            visibleHit ? "visible_ok" : "visible_miss");
    }

    {
        auto shell = BuildShell();
        FullUiFrame(shell, 1280.0f, 720.0f);
        const Point outside{10.0f, 10.0f};
        const auto hit = EventSystem::HitTest(shell.host, outside);
        const bool passed = hit && hit != shell.root && hit != shell.host;
        record("flex_passthrough", passed, passed ? "child_hit" : "container_stole");
    }

    size_t passed = 0;
    for (const auto& c : report.cases) {
        if (c.passed) {
            ++passed;
        }
    }
    std::ostringstream oss;
    oss << "HitTestAudit " << passed << "/" << report.cases.size() << " passed";
    report.summary = oss.str();
    return report;
}

} // namespace we::runtime::kindui
