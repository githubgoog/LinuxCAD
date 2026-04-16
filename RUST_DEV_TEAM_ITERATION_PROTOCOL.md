# Rust Dev Team Iteration Protocol

Date: 2026-04-14

## Purpose

Define a multi-agent development loop for LinuxCAD migration work where parallel coding lanes converge through compile and interoperability gates.

## Team Lanes

1. Types and contracts lane
2. File I/O parity lane
3. Backend API bridge lane
4. UI and state lane
5. QA interoperability lane

## Circular Prototype Loop

1. Plan slice
- Define one migration slice with clear acceptance checks.

2. Parallel implementation
- Run coding lanes in parallel on non-conflicting files.

3. Integration
- Merge code and normalize API boundaries.

4. Compile gate
- Run full workspace compile checks.

5. Runtime gate
- Execute prototype flow (bootstrap, backend probe, read/write, invariant checks).

6. Interop gate
- Run matrix tests for feature compatibility.

7. Iterate
- Fix failures and repeat from step 2 until all gates pass.

## Mandatory Gates

A slice is complete only if all are true:

1. Compile passes
2. Runtime prototype loop passes
3. Interoperability matrix passes for touched feature groups
4. No data compatibility regression for .lcad read/write

## Current Iteration Baseline

Implemented:

1. Rust frontend workspace scaffold
2. Expanded CAD types crate
3. Package-based .lcad file I/O read/write
4. Typed backend health client and geometry operation scaffold
5. App shell circular prototype loop with invariant validation

Validation:

1. cargo check passes
2. app_shell loop runs two iterations and validates roundtrip invariants
