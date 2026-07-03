using IgniteBT.Launcher;

namespace IgniteBT.Launcher;

/// <summary>
/// Backward-compatible entry point that delegates to the bootstrap launcher.
/// </summary>
public static class IgniteBTLauncher
{
    public static int Forward(string[] args, string? startDirectory = null) =>
        BootstrapLauncher.Forward(args, startDirectory);

    public static EngineDescriptorData ResolveMarker(string? startDirectory = null) =>
        BootstrapLauncher.ResolveDescriptor(startDirectory);
}
