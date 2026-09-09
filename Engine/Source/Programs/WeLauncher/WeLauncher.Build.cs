// ==============================================================================
// WindEffects — WeLauncher — WeLauncher.Build
// Source file for the WeLauncher module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;

public class WeLauncher : ModuleRules
{
    public WeLauncher(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        SetBinaryName("WeLauncher.exe");
        PublishAtConfigurationRoot();

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("Text");
        PublicDependencies.Add("Icons");
        PublicDependencies.Add("Projects");
        PrivateDependencies.Add("Engine");
        PrivateDependencies.Add("RHI");
        PrivateDependencies.Add("Renderer");
        PrivateDependencies.Add("ECS");

        PrivateDependencies.Add("VulkanRHI");
        PrivateDependencies.Add("NullRHI");

        AddOptionalThirdParty("nlohmann_json");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Windows";
    }
}
