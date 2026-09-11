// ==============================================================================
// WindEffects — Core — Core.Build
// Source file for the Core module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;

public class Core : ModuleRules
{
    public Core(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        BootstrapBinary();
        SetBinaryName("WindeffectsCore.dll");

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        Definitions.Add("CORE_EXPORTS");
        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.LinkerFlags.Add("delayimp.lib");
        PlatformSettings.Windows.LinkerFlags.Add("dbghelp.lib");

        // nlohmann/json is optional for Core - needed for product metadata and crash reporting
        AddOptionalThirdParty("nlohmann_json");
        DefineIf(HasThirdParty("nlohmann_json"), "WE_HAS_NLOHMANN_JSON=1");
        DefineIf(!HasThirdParty("nlohmann_json"), "WE_HAS_NLOHMANN_JSON=0");

        // glm is Private-only (via Math/GlmInterop.h). Public APIs use Core/Math/Types.h.
        AddOptionalThirdParty("glm");
        DefineIf(HasThirdParty("glm"), "WE_HAS_GLM=1");
        DefineIf(!HasThirdParty("glm"), "WE_HAS_GLM=0");
    }
}
