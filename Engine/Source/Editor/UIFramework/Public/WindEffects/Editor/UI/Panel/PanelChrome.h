#pragma once

#include "WindEffects/Editor/UI/Export.h"
#include "KindUI/Panel/PanelChrome.h"
#include "WindEffects/Editor/UI/Panel/PanelBodyLayout.h"

namespace we::editor::panels {

namespace PanelChrome = ::we::runtime::kindui::panels::PanelChrome;
using DockTabDescriptor = ::we::runtime::kindui::panels::PanelChrome::DockTabDescriptor;
using DockTabLayout = ::we::runtime::kindui::panels::PanelChrome::DockTabLayout;
using DockTabStripState = ::we::runtime::kindui::panels::PanelChrome::DockTabStripState;
using DockTabStripLayout = ::we::runtime::kindui::panels::PanelChrome::DockTabStripLayout;
using DockPanelGeometry = ::we::runtime::kindui::panels::PanelChrome::DockPanelGeometry;
using FloatingHeaderAction = ::we::runtime::kindui::panels::PanelChrome::FloatingHeaderAction;
using PanelBodyRegion = ::we::runtime::kindui::panels::PanelBodyRegion;

} // namespace we::editor::panels
