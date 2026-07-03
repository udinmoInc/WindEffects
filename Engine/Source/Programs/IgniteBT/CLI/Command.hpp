#pragma once

#include "Core/Types.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace IgniteBT {

// Command context
struct CommandContext {
    std::string command;
    std::vector<std::string> arguments;
    std::unordered_map<std::string, std::string> flags;
    
    bool HasFlag(const std::string& flag) const;
    std::string GetFlagValue(const std::string& flag, const std::string& defaultValue = "") const;
    std::vector<std::string> GetArguments() const { return arguments; }
};

// Command interface
class ICommand {
public:
    virtual ~ICommand() = default;
    
    virtual std::string GetName() const = 0;
    virtual std::string GetDescription() const = 0;
    virtual std::string GetUsage() const = 0;
    
    virtual Result<void> Execute(const CommandContext& context) = 0;
};

// Command registry
class CommandRegistry {
public:
    static CommandRegistry& Get();
    
    void RegisterCommand(std::shared_ptr<ICommand> command);
    void UnregisterCommand(const std::string& name);
    
    std::shared_ptr<ICommand> GetCommand(const std::string& name) const;
    std::vector<std::shared_ptr<ICommand>> GetAllCommands() const;
    
    Result<void> ExecuteCommand(const CommandContext& context);
    
private:
    CommandRegistry() = default;
    
    std::unordered_map<std::string, std::shared_ptr<ICommand>> m_commands;
};

// Command parser
class CommandParser {
public:
    static CommandContext Parse(const std::string& commandLine);
    static CommandContext Parse(int argc, char* argv[]);
    
private:
    static bool IsFlag(const std::string& arg);
    static std::pair<std::string, std::string> ParseFlag(const std::string& arg);
};

// Built-in commands
class HelpCommand : public ICommand {
public:
    std::string GetName() const override { return "help"; }
    std::string GetDescription() const override { return "Show help information"; }
    std::string GetUsage() const override { return "help [command]"; }
    
    Result<void> Execute(const CommandContext& context) override;
};

class VersionCommand : public ICommand {
public:
    std::string GetName() const override { return "version"; }
    std::string GetDescription() const override { return "Show version information"; }
    std::string GetUsage() const override { return "version"; }
    
    Result<void> Execute(const CommandContext& context) override;
};

class CleanCommand : public ICommand {
public:
    std::string GetName() const override { return "clean"; }
    std::string GetDescription() const override { return "Clean build artifacts"; }
    std::string GetUsage() const override { return "clean [--target=<target>] [--all]"; }
    
    Result<void> Execute(const CommandContext& context) override;
};

class RebuildCommand : public ICommand {
public:
    std::string GetName() const override { return "rebuild"; }
    std::string GetDescription() const override { return "Rebuild targets"; }
    std::string GetUsage() const override { return "rebuild <target>"; }
    
    Result<void> Execute(const CommandContext& context) override;
};

class DoctorCommand : public ICommand {
public:
    std::string GetName() const override { return "doctor"; }
    std::string GetDescription() const override { return "Diagnose build environment"; }
    std::string GetUsage() const override { return "doctor"; }
    
    Result<void> Execute(const CommandContext& context) override;
};

class BenchmarkCommand : public ICommand {
public:
    std::string GetName() const override { return "benchmark"; }
    std::string GetDescription() const override { return "Benchmark build performance"; }
    std::string GetUsage() const override { return "benchmark [--iterations=<n>]"; }
    
    Result<void> Execute(const CommandContext& context) override;
};

class PluginsCommand : public ICommand {
public:
    std::string GetName() const override { return "plugins"; }
    std::string GetDescription() const override { return "List available plugins"; }
    std::string GetUsage() const override { return "plugins [--list] [--info=<plugin>]"; }
    
    Result<void> Execute(const CommandContext& context) override;
};

// Build command (placeholder - will be implemented with BuildGraph)
class BuildCommand : public ICommand {
public:
    std::string GetName() const override { return "build"; }
    std::string GetDescription() const override { return "Build targets"; }
    std::string GetUsage() const override { return "build <target> [--config=<config>] [--platform=<platform>]"; }
    
    Result<void> Execute(const CommandContext& context) override;
};

// Generate command (placeholder - will be implemented with ProjectGenerator)
class GenerateCommand : public ICommand {
public:
    std::string GetName() const override { return "generate"; }
    std::string GetDescription() const override { "Generate project files"; }
    std::string GetUsage() const override { return "generate <format> [--output=<path>]"; }
    
    Result<void> Execute(const CommandContext& context) override;
};

// Package command (placeholder - will be implemented with PackageManager)
class PackageCommand : public ICommand {
public:
    std::string GetName() const override { return "package"; }
    std::string GetDescription() const override { "Create distributable package"; }
    std::string GetUsage() const override { return "package <target> [--output=<path>]"; }
    
    Result<void> Execute(const CommandContext& context) override;
};

// Test command (placeholder)
class TestCommand : public ICommand {
public:
    std::string GetName() const override { return "test"; }
    std::string GetDescription() const override { "Run tests"; }
    std::string GetUsage() const override { return "test [--filter=<pattern>]"; }
    
    Result<void> Execute(const CommandContext& context) override;
};

// Graph command (placeholder - will be implemented with BuildGraph)
class GraphCommand : public ICommand {
public:
    std::string GetName() const override { return "graph"; }
    std::string GetDescription() const override { "Show dependency graph"; }
    std::string GetUsage() const override { return "graph <target> [--output=<path>]"; }
    
    Result<void> Execute(const CommandContext& context) override;
};

} // namespace IgniteBT
