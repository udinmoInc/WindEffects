// ==============================================================================
// WindEffects — DirectX12RHI — DirectX12RHI.Build
// Source file for the DirectX12RHI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;

public class DirectX12RHI : ModuleRules
{
    public DirectX12RHI(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;
        BootstrapBinary();
        SetBinaryName("WEDX12RHI.dll");
        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");
        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        Definitions.Add("DX12RHI_EXPORTS");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.LinkerFlags.Add("d3d12.lib");
        PlatformSettings.Windows.LinkerFlags.Add("dxgi.lib");
        PlatformSettings.Windows.LinkerFlags.Add("dxguid.lib");
    }
}
