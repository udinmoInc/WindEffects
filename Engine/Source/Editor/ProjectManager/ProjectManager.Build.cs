// ==============================================================================
// WindEffects — ProjectManager — ProjectManager.Build
// Source file for the ProjectManager module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;

public class ProjectManager : ModuleRules
{
    public ProjectManager(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("Projects");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("Text");

        Definitions.Add("PROJECTMANAGER_EXPORTS");
    }
}
