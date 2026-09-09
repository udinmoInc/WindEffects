// ==============================================================================
// WindEffects — MainFrame — MainFrame.Build
// Source file for the MainFrame module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;

public class MainFrame : ModuleRules
{
    public MainFrame(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("Text");
        PublicDependencies.Add("Toolbar");
        PublicDependencies.Add("Menus");
        PrivateDependencies.Add("EditorShell");

        Definitions.Add("MAINFRAME_EXPORTS");
    }
}
