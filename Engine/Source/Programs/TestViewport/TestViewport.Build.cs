// ==============================================================================
// WindEffects — TestViewport — TestViewport.Build
// Fast, minimal standalone test application for volumetric cloud development.
// Opens a native OS window with only the render viewport.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// ==============================================================================
using IgniteBT.BuildSystem;

public class TestViewport : ModuleRules
{
    public TestViewport(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        SetBinaryName("TestViewport.exe");
        PublishAtConfigurationRoot();

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Renderer");

        PrivateDependencies.Add("VulkanRHI");
        PrivateDependencies.Add("DirectX12RHI");
        PrivateDependencies.Add("NullRHI");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Console";
    }
}
