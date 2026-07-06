namespace IgniteBT.Workspace.SDK;

/// <summary>
/// Result type for SDK operations.
/// </summary>
public class SDKResult<T>
{
    public bool Success { get; set; }
    public T? Value { get; set; }
    public string? Error { get; set; }
    public List<string> Warnings { get; set; } = new();
    public List<string> SearchLocations { get; set; } = new();
    
    public static SDKResult<T> Ok(T value) => new() { Success = true, Value = value };
    public static SDKResult<T> Fail(string error) => new() { Success = false, Error = error };
    
    public SDKResult<T> WithWarning(string warning)
    {
        Warnings.Add(warning);
        return this;
    }
    
    public SDKResult<T> WithSearchLocation(string location)
    {
        SearchLocations.Add(location);
        return this;
    }
}

/// <summary>
/// Non-generic result type for SDK operations.
/// </summary>
public class SDKResult
{
    public bool Success { get; set; }
    public string? Error { get; set; }
    public List<string> Warnings { get; set; } = new();
    public List<string> SearchLocations { get; set; } = new();
    
    public static SDKResult Ok() => new() { Success = true };
    public static SDKResult Fail(string error) => new() { Success = false, Error = error };
    
    public SDKResult WithWarning(string warning)
    {
        Warnings.Add(warning);
        return this;
    }
    
    public SDKResult WithSearchLocation(string location)
    {
        SearchLocations.Add(location);
        return this;
    }
}
