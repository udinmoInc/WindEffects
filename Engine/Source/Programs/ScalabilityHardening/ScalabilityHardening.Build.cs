// ==============================================================================
// WindEffects — ScalabilityHardening — ScalabilityHardening.Build
// Source file for the ScalabilityHardening module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;

public class ScalabilityHardening : ModuleRules
{
    public ScalabilityHardening(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        SetBinaryName("ScalabilityHardening.exe");
        PublishAtConfigurationRoot();

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Renderer");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Console";
    }
}
