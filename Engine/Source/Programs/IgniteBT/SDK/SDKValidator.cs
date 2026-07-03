using Serilog;

namespace IgniteBT.SDK;

/// <summary>
/// Validates SDK installations.
/// </summary>
public class SDKValidator
{
    /// <summary>
    /// Validates an SDK installation.
    /// </summary>
    public async Task<ValidationResult> ValidateAsync(ISDKProvider provider, SDKInfo info)
    {
        var result = new ValidationResult();
        
        Log.Debug("Validating SDK: {SDKName}", info.Name);
        
        // Check if root path exists
        if (!Directory.Exists(info.RootPath))
        {
            result.ValidationErrors.Add($"Root path does not exist: {info.RootPath}");
            return result;
        }
        
        // Validate include paths
        foreach (var includePath in info.IncludePaths)
        {
            if (!Directory.Exists(includePath))
            {
                result.ValidationErrors.Add($"Include path does not exist: {includePath}");
            }
        }
        
        // Validate library paths
        foreach (var libraryPath in info.LibraryPaths)
        {
            if (!Directory.Exists(libraryPath))
            {
                result.ValidationErrors.Add($"Library path does not exist: {libraryPath}");
            }
        }
        
        // Validate binary paths
        foreach (var binaryPath in info.BinaryPaths)
        {
            if (!Directory.Exists(binaryPath))
            {
                result.Warnings.Add($"Binary path does not exist: {binaryPath}");
            }
        }
        
        // Validate tool paths
        foreach (var toolPath in info.ToolPaths)
        {
            if (!File.Exists(toolPath))
            {
                result.Warnings.Add($"Tool does not exist: {toolPath}");
            }
        }
        
        // Validate version
        if (string.IsNullOrEmpty(info.Version) || info.Version == "Unknown")
        {
            result.Warnings.Add("Could not determine SDK version");
        }
        
        // Validate platform compatibility
        if (!provider.SupportedPlatforms.Contains(info.Platform))
        {
            result.ValidationErrors.Add($"Platform {info.Platform} is not supported by this SDK");
        }
        
        // Validate architecture compatibility
        if (!provider.SupportedArchitectures.Contains(info.Architecture))
        {
            result.ValidationErrors.Add($"Architecture {info.Architecture} is not supported by this SDK");
        }
        
        // Run provider-specific validation
        try
        {
            var validationResult = await provider.ValidateAsync(info.RootPath);
            if (!validationResult.Success)
            {
                result.ValidationErrors.Add(validationResult.Error ?? "Provider validation failed");
            }
            result.Warnings.AddRange(validationResult.Warnings);
        }
        catch (Exception ex)
        {
            result.Warnings.Add($"Provider validation threw exception: {ex.Message}");
        }
        
        result.Success = result.ValidationErrors.Count == 0;
        
        if (result.Success)
        {
            Log.Debug("SDK {SDKName} validation passed", info.Name);
        }
        else
        {
            Log.Warning("SDK {SDKName} validation failed with {Count} errors", 
                info.Name, result.ValidationErrors.Count);
        }
        
        return result;
    }
}

/// <summary>
/// Result of SDK validation.
/// </summary>
public class ValidationResult
{
    public bool Success { get; set; }
    public List<string> ValidationErrors { get; set; } = new();
    public List<string> Warnings { get; set; } = new();
}
