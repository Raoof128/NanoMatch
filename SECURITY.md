# Security Policy

## Reporting a Vulnerability

If you discover a security vulnerability in NanoMatch, please report it responsibly.

**Do not open a public GitHub issue for security vulnerabilities.**

### How to Report

Email: **raouf.abedini@protonmail.com**

Include in your report:
- Description of the vulnerability
- Steps to reproduce
- Potential impact
- Suggested fix (if any)

### Response Timeline

| Stage | Timeline |
|-------|----------|
| Acknowledgement | Within 48 hours |
| Initial assessment | Within 7 days |
| Fix development | Within 30 days |
| Public disclosure | After fix is released |

### Scope

This policy covers:
- The NanoMatch matching engine library (`include/`, `src/`)
- Build system and CI/CD configuration
- Test infrastructure

Out of scope:
- Third-party dependencies (report to their maintainers)
- Benchmark results or performance characteristics

### Responsible Disclosure

We ask that you:
- Give us reasonable time to address the issue before public disclosure
- Do not exploit the vulnerability beyond what is necessary to demonstrate it
- Do not access or modify data belonging to others

We commit to:
- Acknowledging your report promptly
- Keeping you informed of our progress
- Crediting you in the fix announcement (unless you prefer otherwise)

## Supported Versions

| Version | Supported |
|---------|-----------|
| 1.x     | Yes       |
