// ==============================================================================
// WindEffects — Platform — Platform.Build
// Source file for the Platform module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;

public class Platform : ModuleRules
{
    public Platform(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        BootstrapBinary();
        SetBinaryName("WindeffectsPlatform.dll");

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");

        Definitions.Add("PLATFORM_EXPORTS");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.LinkerFlags.Add("user32.lib");
        PlatformSettings.Windows.LinkerFlags.Add("gdi32.lib");
        PlatformSettings.Windows.LinkerFlags.Add("shell32.lib");
        PlatformSettings.Windows.LinkerFlags.Add("ole32.lib");
        PlatformSettings.Windows.LinkerFlags.Add("oleaut32.lib");
        PlatformSettings.Windows.LinkerFlags.Add("uuid.lib");
        PlatformSettings.Windows.LinkerFlags.Add("advapi32.lib");
        PlatformSettings.Windows.LinkerFlags.Add("comdlg32.lib");
        PlatformSettings.Windows.LinkerFlags.Add("dwmapi.lib");
        PlatformSettings.Windows.LinkerFlags.Add("shlwapi.lib");
        PlatformSettings.Windows.LinkerFlags.Add("winmm.lib");
        PlatformSettings.Windows.LinkerFlags.Add("xinput.lib");
    }
}
