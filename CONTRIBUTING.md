# Contributing to NanoMatch

Thank you for your interest in contributing to NanoMatch. This document provides guidelines and instructions for contributing to this project.

## Prerequisites

Before contributing, ensure you have the following installed:

- **C++20-compatible compiler**: GCC 11+, Clang 14+, or MSVC 2022+
- **CMake**: 3.20 or higher
- **Google Test**: installed on the system and found with `find_package(GTest)` (Debian/Ubuntu: `libgtest-dev`; macOS: `brew install googletest`)
- **clang-format**: 18, the version CI checks with (other versions format a few lines differently)
- **clang-tidy**: 14+ (for static analysis, optional but recommended)
- **Doxygen**: 1.9+ (for documentation generation, optional)

## Building the Project

```bash
# Clone the repository
git clone https://github.com/raouf-abedini/NanoMatch.git
cd NanoMatch

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# For debug builds with sanitizers
cmake -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
```

## Running Tests

```bash
# Run all tests
cd build
ctest --output-on-failure

# Run tests directly
./build/tests/nanomatch_tests
```

## Running Benchmarks

```bash
./build/bench/nanomatch_bench
```

## Code Style

This project uses `clang-format` to enforce a consistent code style. The configuration is defined in `.clang-format` at the project root.

### Formatting Your Code

```bash
# Format a single file
clang-format -i src/book/order_book.cpp

# Format all source files
find include src tests bench -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i
```

### Style Summary

- 4-space indentation (no tabs)
- 100-character column limit
- `Left` pointer alignment (`int* ptr`, not `int *ptr`)
- Braces on the next line after function definitions
- Sorted `#include` directives
- `snake_case` for variables and functions
- `PascalCase` for types and classes
- `UPPER_SNAKE_CASE` for constants and macros
- Prefix member variables with `m_`

### Static Analysis

```bash
# Run clang-tidy on a file
clang-tidy src/engine/matching_engine.cpp -- -std=c++20 -Iinclude

# Run clang-tidy on all source files
find src include -name '*.cpp' -o -name '*.hpp' | xargs clang-tidy -- -std=c++20 -Iinclude
```

## Branching Model

- **`main`**: Stable, production-ready code. All CI checks must pass before merging.
- **Feature branches**: Branch from `main` using the naming convention below.

### Branch Naming

| Type        | Pattern                        | Example                          |
|-------------|--------------------------------|----------------------------------|
| Feature     | `feature/<short-description>`  | `feature/add-iceberg-orders`     |
| Bug fix     | `fix/<short-description>`      | `fix/price-level-crash`          |
| Performance | `perf/<short-description>`     | `perf/reduce-allocation-overhead`|
| Docs        | `docs/<short-description>`     | `docs/update-api-reference`      |
| Refactor    | `refactor/<short-description>` | `refactor/order-book-cleanup`    |

## Pull Request Process

1. **Create a branch** from `main` following the naming conventions above.
2. **Make focused changes**. Each PR should address a single concern.
3. **Write or update tests** for any new or changed functionality.
4. **Ensure all tests pass** locally before pushing.
5. **Run clang-format** on all modified files.
6. **Open a pull request** against `main` using the PR template.
7. **Respond to review feedback** promptly.
8. **Squash commits** into logical units before merging (or request squash-merge).

### PR Checklist

Before submitting your PR, verify:

- [ ] Code compiles without warnings (`-Wall -Wextra -Wpedantic`)
- [ ] All existing tests pass
- [ ] New tests are added for new functionality
- [ ] Code is formatted with `clang-format`
- [ ] No performance regressions (run benchmarks if applicable)
- [ ] Documentation is updated if public API changes

## Commit Message Conventions

Follow the [Conventional Commits](https://www.conventionalcommits.org/) specification:

```
<type>(<scope>): <short summary>

<optional body>

<optional footer>
```

### Types

| Type       | Description                                    |
|------------|------------------------------------------------|
| `feat`     | A new feature                                  |
| `fix`      | A bug fix                                      |
| `perf`     | A performance improvement                      |
| `refactor` | Code change that neither fixes a bug nor adds a feature |
| `test`     | Adding or updating tests                       |
| `docs`     | Documentation only changes                     |
| `build`    | Changes to the build system or dependencies    |
| `ci`       | Changes to CI configuration files and scripts  |
| `chore`    | Other changes that don't modify src or test    |

### Examples

```
feat(engine): add fill-or-kill order support

Implement FOK order type that either fills the entire order
immediately or cancels it entirely.

Closes #42
```

```
perf(book): reduce cache misses in price level traversal

Replace std::map with a sorted vector for price levels,
improving L1 cache utilization during order matching.

Benchmark results: 15% latency reduction at p99.
```

## Testing Requirements

- All new features must include unit tests.
- All bug fixes must include a regression test that demonstrates the fix.
- Tests should be placed in the `tests/` directory, following the existing structure.
- Use Google Test (`gtest`) for all test cases.
- Aim for deterministic tests; avoid dependencies on timing or system state.
- Performance-sensitive changes should include benchmark results in the PR description.

### Writing Tests

```cpp
#include <gtest/gtest.h>
#include "nanomatch/engine/matching_engine.hpp"

TEST(MatchingEngineTest, LimitBuyMatchesSellAtSamePrice) {
    // Arrange
    MatchingEngine engine;
    // ...
    // Act
    // ...
    // Assert
    EXPECT_EQ(result.status, FillStatus::Filled);
}
```

## Reporting Issues

If you find a bug or have a feature request, please open an issue using the appropriate template in the GitHub issue tracker. For security vulnerabilities, see [SECURITY.md](SECURITY.md).

## License

By contributing to NanoMatch, you agree that your contributions will be licensed under the MIT License, as specified in the [LICENSE](LICENSE) file.
