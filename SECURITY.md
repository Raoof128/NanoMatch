# Security Policy

NanoMatch is a study project maintained by one person in spare time. It is a single-threaded library and command-line demo with no network surface.

## Reporting a Vulnerability

Please do not open a public issue for a security problem. Use GitHub's private vulnerability reporting instead:

**[Report a vulnerability](https://github.com/Raoof128/NanoMatch/security/advisories/new)** (the "Report a vulnerability" button on the repository's Security tab)

Useful things to include: what the problem is, an input or sequence of calls that reproduces it, and a suggested fix if you have one.

Reports are read and answered as time allows; there is no guaranteed response time. Reporters are credited in the changelog unless they prefer otherwise.

## Scope

In scope: memory-safety errors, crashes or hangs from malformed input (the CSV reader, command-line arguments), and undefined behaviour in the library (`include/`, `src/`).

Out of scope: benchmark results, and third-party dependencies (report those upstream).

## Supported Versions

Only the current `main` branch is maintained.
