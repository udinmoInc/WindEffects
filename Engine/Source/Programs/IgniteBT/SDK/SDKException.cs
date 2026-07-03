namespace IgniteBT.SDK;

/// <summary>
/// Exception thrown when SDK operations fail.
/// </summary>
public class SDKException : Exception
{
    public string SDKName { get; }
    public List<string> SearchLocations { get; }
    
    public SDKException(string sdkName, string message) : base(message)
    {
        SDKName = sdkName;
        SearchLocations = new List<string>();
    }
    
    public SDKException(string sdkName, string message, Exception innerException) 
        : base(message, innerException)
    {
        SDKName = sdkName;
        SearchLocations = new List<string>();
    }
    
    public SDKException(string sdkName, string message, List<string> searchLocations) 
        : base(message)
    {
        SDKName = sdkName;
        SearchLocations = searchLocations;
    }
}

/// <summary>
/// Exception thrown when SDK validation fails.
/// </summary>
public class SDKValidationException : SDKException
{
    public List<string> ValidationErrors { get; }
    
    public SDKValidationException(string sdkName, List<string> validationErrors) 
        : base(sdkName, $"SDK validation failed for {sdkName}")
    {
        ValidationErrors = validationErrors;
    }
}

/// <summary>
/// Exception thrown when SDK is not found.
/// </summary>
public class SDKNotFoundException : SDKException
{
    public SDKNotFoundException(string sdkName, List<string> searchLocations) 
        : base(sdkName, $"SDK '{sdkName}' not found. Searched in: {string.Join(", ", searchLocations)}", searchLocations)
    {
    }
}
