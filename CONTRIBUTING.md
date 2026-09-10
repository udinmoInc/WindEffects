# Contributing to WindEffects Engine

Thank you for your interest in contributing to WindEffects Engine.

## Table of Contents

- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [Code Style](#code-style)
- [Submitting Changes](#submitting-changes)
- [Pull Request Process](#pull-request-process)
- [Reporting Bugs](#reporting-bugs)
- [Suggesting Features](#suggesting-features)

## Getting Started

### Prerequisites

- Windows 10/11 (64-bit)
- Visual Studio 2022 with C++ workload
- .NET 8.0 SDK
- Vulkan SDK
- Git
- GitHub account

### Fork and Clone

1. Fork the repository on GitHub
2. Clone your fork locally:
   ```bash
   git clone https://github.com/YOUR_USERNAME/WindEffects.git
   cd WindEffects
   ```
3. Add the upstream remote:
   ```bash
   git remote add upstream https://github.com/udinmoInc/WindEffects.git
   ```

## Development Setup

### Building the Engine

Follow the build instructions in the main README.md:

```bash
# Build in Development configuration
we build --config Development

# Run the editor
we run --target Editor --config Development
```

### Keeping Your Fork Updated

```bash
git fetch upstream
git checkout develop
git merge upstream/develop
```

## Code Style

### C++ Guidelines

- Use modern C++23 features where appropriate
- Follow the existing code style in the codebase
- Use meaningful variable and function names
- Add comments for complex logic (explain *why*, not *what*)
- Keep functions focused and concise
- Prefer const correctness
- Use RAII for resource management

### Formatting

- 4 spaces for indentation
- Maximum line length: 120 characters
- Braces on new line for functions/classes
- Braces on same line for control structures

### Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Classes | PascalCase | `EntityManager` |
| Functions | PascalCase | `UpdateEntity` |
| Variables | camelCase | `entityCount` |
| Member variables | m_camelCase | `m_entityCount` |
| Constants | UPPER_SNAKE_CASE | `MAX_ENTITIES` |
| Files | PascalCase matching class | `EntityManager.h` |

## Submitting Changes

### Creating a Branch

```bash
git checkout -b feature/your-feature-name
# or
git checkout -b fix/your-bug-fix
```

### Making Changes

1. Follow the code style guidelines
2. Test your changes thoroughly
3. Build the engine to ensure no compilation errors
4. Run the editor to verify functionality

### Committing

Use clear, descriptive commit messages:

```
type(scope): subject

body

footer
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Maintenance tasks

Example:
```
feat(renderer): add support for ray tracing

Implement hardware-accelerated ray tracing for realistic
reflections and shadows. Added new RayTracingRenderer class
and integrated with existing render graph.

Closes #123
```

## Pull Request Process

### Before Submitting

1. Ensure code follows style guidelines
2. Add tests for new functionality
3. Update documentation if needed
4. Ensure all tests pass
5. Rebase on the latest develop branch

### Submitting a PR

1. Push your branch to your fork:
   ```bash
   git push origin feature/your-feature-name
   ```
2. Open a pull request on GitHub
3. Fill out the PR template
4. Link related issues
5. Request review from maintainers

### PR Review Process

- Maintainers will review your PR
- Address feedback in a timely manner
- Keep discussions focused and constructive
- Once approved, your PR will be merged

## Reporting Bugs

### Before Reporting

1. Check existing issues to avoid duplicates
2. Search the documentation
3. Try to reproduce the issue
4. Gather relevant information

### Bug Report Template

Include:
- Clear title describing the bug
- Steps to reproduce
- Expected behavior
- Actual behavior
- Environment details (OS, hardware, configuration)
- Screenshots or recordings if applicable
- Log files if relevant

## Suggesting Features

### Before Suggesting

1. Check existing feature requests
2. Consider if the feature fits the project scope
3. Think about implementation complexity
4. Be prepared to discuss the proposal

### Feature Request Template

Include:
- Clear title describing the feature
- Detailed description
- Use cases and benefits
- Potential implementation approach
- Alternative solutions considered

## Community Guidelines

- Be respectful and constructive in all interactions
- Welcome newcomers and help them learn
- Focus on what is best for the community
- Show empathy towards other community members

## Getting Help

- Check the documentation
- Search existing issues and discussions
- Ask questions in GitHub Discussions
- Contact the team at support@udinmo.com

## License

By contributing to WindEffects Engine, you agree that your contributions will be licensed under the same license as the project.