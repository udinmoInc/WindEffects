// ==============================================================================
// WindEffects — VulkanRHI — VulkanRHI.Build
// Source file for the VulkanRHI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;
using System.IO;

public class VulkanRHI : ModuleRules
{
    public VulkanRHI(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        BootstrapBinary();
        SetBinaryName("WEVulkanRHI.dll");

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");

        var thirdPartyRoot = Path.Combine(context.EngineDirectory, "ThirdParty");
        PrivateIncludePaths.Add(Path.Combine(thirdPartyRoot, "volk"));
        PrivateIncludePaths.Add(Path.Combine(thirdPartyRoot, "Vulkan-Headers", "include"));

        OptionalSDK("VulkanSDK");
        DefineIf(HasSDK("VulkanSDK") || true, "WE_HAS_VULKAN=1");

        AddOptionalThirdParty("glm");
        DefineIf(HasThirdParty("glm"), "WE_HAS_GLM=1");
        DefineIf(!HasThirdParty("glm"), "WE_HAS_GLM=0");

        Definitions.Add("VULKANRHI_EXPORTS");

        PlatformSettings.Windows ??= new WindowsSettings();
    }
}
