// ==============================================================================
// WindEffects — ToolsPanel — ToolsPanel.Build
// Source file for the ToolsPanel module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;

public class ToolsPanel : ModuleRules
{
    public ToolsPanel(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("Text");
        PublicDependencies.Add("EditorShell");
        PrivateDependencies.Add("Menus");
        PrivateDependencies.Add("ContentBrowser");

        Definitions.Add("TOOLSPANEL_EXPORTS");
    }
}
