using Serilog;
using IgniteBT.ModuleDiscovery;

namespace IgniteBT.BuildGraph;

/// <summary>
/// Represents a node in the build dependency graph.
/// </summary>
public class BuildNode
{
    /// <summary>
    /// Module name.
    /// </summary>
    public string Name { get; set; } = string.Empty;

    /// <summary>
    /// Discovered module information.
    /// </summary>
    public DiscoveredModule Module { get; set; } = new();

    /// <summary>
    /// Dependencies of this module.
    /// </summary>
    public List<BuildNode> Dependencies { get; set; } = new();

    /// <summary>
    /// Modules that depend on this module.
    /// </summary>
    public List<BuildNode> Dependents { get; set; } = new();

    /// <summary>
    /// Build order index (for topological sort).
    /// </summary>
    public int BuildOrder { get; set; } = -1;

    /// <summary>
    /// Whether this node has been visited during traversal.
    /// </summary>
    public bool Visited { get; set; }

    /// <summary>
    /// Whether this node is currently in the recursion stack (for cycle detection).
    /// </summary>
    public bool InRecursionStack { get; set; }
}

/// <summary>
/// Build dependency graph that determines build order based on module dependencies.
/// </summary>
public class DependencyGraph
{
    private readonly Dictionary<string, BuildNode> _nodes = new();

    /// <summary>
    /// Gets all nodes in the graph.
    /// </summary>
    public IReadOnlyCollection<BuildNode> Nodes => _nodes.Values.ToList().AsReadOnly();

    /// <summary>
    /// Builds the dependency graph from discovered modules.
    /// </summary>
    public void BuildFromModules(List<DiscoveredModule> modules)
    {
        Log.Information("Building dependency graph from {Count} modules", modules.Count);
        _nodes.Clear();

        // Create nodes for all modules
        foreach (var module in modules)
        {
            var node = new BuildNode
            {
                Name = module.Name,
                Module = module
            };
            _nodes[module.Name] = node;
        }

        // Build dependency edges
        foreach (var module in modules)
        {
            var node = _nodes[module.Name];
            
            foreach (var depName in module.PublicDependencies)
            {
                AddDependency(node, depName);
            }
            
            foreach (var depName in module.PrivateDependencies)
            {
                AddDependency(node, depName);
            }
        }

        Log.Information("Built graph with {Count} nodes", _nodes.Count);
    }

    /// <summary>
    /// Adds a dependency edge from one node to another.
    /// </summary>
    private void AddDependency(BuildNode fromNode, string toModuleName)
    {
        if (_nodes.TryGetValue(toModuleName, out var toNode))
        {
            if (!fromNode.Dependencies.Contains(toNode))
            {
                fromNode.Dependencies.Add(toNode);
                toNode.Dependents.Add(fromNode);
            }
        }
        else
        {
            Log.Warning("Dependency {DepName} not found for module {ModuleName}", toModuleName, fromNode.Name);
        }
    }

    /// <summary>
    /// Performs topological sort to determine build order.
    /// </summary>
    public List<BuildNode> GetBuildOrder()
    {
        Log.Information("Computing build order");
        
        // Reset visited flags
        foreach (var node in _nodes.Values)
        {
            node.Visited = false;
            node.InRecursionStack = false;
            node.BuildOrder = -1;
        }

        var buildOrder = new List<BuildNode>();

        // Perform DFS for topological sort (Kahn's algorithm variant)
        // We want dependencies first, then dependents
        var tempVisited = new HashSet<BuildNode>();
        var permanentVisited = new HashSet<BuildNode>();

        foreach (var node in _nodes.Values.OrderBy(n => n.Name))
        {
            if (!permanentVisited.Contains(node))
            {
                if (!VisitNodeTopological(node, tempVisited, permanentVisited, buildOrder))
                {
                    // Circular dependency detected
                    Log.Error("Circular dependency detected in build graph");
                    return new List<BuildNode>();
                }
            }
        }

        // buildOrder now has dependencies first (no need to reverse)
        Log.Information("Build order: {Order}", string.Join(" -> ", buildOrder.Select(n => n.Name)));
        return buildOrder;
    }

    /// <summary>
    /// Visits a node during DFS traversal for topological sort using temporary/permanent marking.
    /// </summary>
    private bool VisitNodeTopological(BuildNode node, HashSet<BuildNode> tempVisited, 
        HashSet<BuildNode> permanentVisited, List<BuildNode> buildOrder)
    {
        if (permanentVisited.Contains(node))
        {
            return true; // Already processed
        }

        if (tempVisited.Contains(node))
        {
            Log.Error("Circular dependency detected: {Node}", node.Name);
            return false; // Cycle detected
        }

        tempVisited.Add(node);

        // Visit all dependencies first
        foreach (var dep in node.Dependencies)
        {
            if (!VisitNodeTopological(dep, tempVisited, permanentVisited, buildOrder))
            {
                return false;
            }
        }

        tempVisited.Remove(node);
        permanentVisited.Add(node);
        buildOrder.Add(node);

        return true;
    }

    /// <summary>
    /// Gets all nodes in the graph.
    /// </summary>
    public IReadOnlyCollection<BuildNode> GetNodes()
    {
        return _nodes.Values;
    }

    /// <summary>
    /// Gets a node by name.
    /// </summary>
    public BuildNode? GetNode(string name)
    {
        return _nodes.TryGetValue(name, out var node) ? node : null;
    }

    /// <summary>
    /// Validates the graph for circular dependencies.
    /// </summary>
    public bool Validate()
    {
        Log.Information("Validating build graph");

        // Reset visited flags
        foreach (var node in _nodes.Values)
        {
            node.Visited = false;
            node.InRecursionStack = false;
        }

        foreach (var node in _nodes.Values)
        {
            if (!node.Visited)
            {
                if (!CheckCycles(node))
                {
                    return false;
                }
            }
        }

        Log.Information("Build graph validation passed");
        return true;
    }

    /// <summary>
    /// Checks for cycles starting from a node.
    /// </summary>
    private bool CheckCycles(BuildNode node)
    {
        node.Visited = true;
        node.InRecursionStack = true;

        foreach (var dep in node.Dependencies)
        {
            if (!dep.Visited)
            {
                if (!CheckCycles(dep))
                {
                    return false;
                }
            }
            else if (dep.InRecursionStack)
            {
                Log.Error("Circular dependency detected: {Node} -> {Dep}", node.Name, dep.Name);
                return false;
            }
        }

        node.InRecursionStack = false;
        return true;
    }
}
