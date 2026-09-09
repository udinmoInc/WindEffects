// ==============================================================================
// WindEffects — CrashReporter — CrashReporter.Build
// Source file for the CrashReporter module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;

public class CrashReporter : ModuleRules
{
    public CrashReporter(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        SetBinaryName("WECrashReporter.exe");
        PublishAtConfigurationRoot();

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PrivateDependencies.Add("Engine");
        PrivateDependencies.Add("Renderer");
        PrivateDependencies.Add("RHI");
        PrivateDependencies.Add("KindUI");

        AddOptionalThirdParty("nlohmann_json");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Windows";
    }
}
