# LinuxCAD Interoperability Test Matrix (Release-Blocking)

Date: 2026-04-14

## Policy

A feature is unacceptable if it breaks when used after or alongside another feature.
All matrix failures are release blockers.

## Critical Sequence Suite

1. Primitive -> Extrude -> Boolean -> Fillet -> Chamfer -> Save/Load
2. Primitive -> Mirror -> Array -> Boolean -> Save/Load
3. Sketch -> Extrude -> Revolve -> Sweep -> Save/Load
4. Primitive -> Hole -> Radial Array -> Chamfer
5. Loft between two generated parts -> Mirror -> Fillet
6. Boolean result -> Shell -> Fillet
7. Boolean result -> Cut -> Hole
8. Revolve result -> Sweep -> Fillet
9. Sweep result -> Boolean union with primitive
10. Mate A-B -> move A -> solve -> save/load -> solve
11. Mate chain A-B-C -> apply array on B -> constraints remain valid
12. Section view on/off during edit -> no geometry/state corruption
13. Import project -> merge features -> apply boolean and fillet
14. Undo/redo through multi-op chain (>=20 steps)
15. Duplicate selected features -> apply mates -> save/load
16. Template load -> parameter edits -> operation chaining

## Pairwise Coverage Targets

Major feature groups:

- Primitive creation
- Sketch
- Extrude
- Boolean
- Mirror
- Linear array
- Radial array
- Shell
- Chamfer
- Fillet
- Cut
- Hole
- Revolve
- Sweep
- Loft
- Mate constraints
- Import/export
- Save/load
- Undo/redo

Coverage rules:

1. Every feature group must be tested after every other feature group at least once.
2. Every sequence must include a save/load checkpoint for persistence validation.
3. Any known unstable pair gets a dedicated regression test case.

## Pass/Fail Criteria

A test case passes only if all of the following are true:

1. No crash/hang.
2. Resulting geometry remains valid and selectable.
3. Constraint solve status is valid (no orphan/cycle failures unless expected by test).
4. Save/load reproduces same logical model state.
5. Undo/redo returns to exact prior state for tracked params/features.

Failure types:

- `crash`: process termination or unhandled exception
- `invalid-geometry`: non-renderable or invalid mesh result
- `constraint-regression`: mates/constraints fail after operation
- `state-regression`: wrong selection/history or incorrect mode transitions
- `persistence-regression`: mismatch after save/load roundtrip

## Automation Layers

1. Unit tests (Rust): geometry ops, serialization, solver edge cases.
2. Integration tests (Rust + API): operation chains and solver behavior.
3. E2E tests (desktop): full workflows with file roundtrips.

## Required CI Gates

1. Unit + integration suite must be green on Linux before merge.
2. Critical sequence E2E suite must be green on release branch.
3. Manual smoke of top 5 workflows before tagging.

## Minimal Smoke Flows

1. New project -> create 3 primitives -> boolean union -> fillet -> save/load.
2. Sketch -> extrude -> hole -> mirror -> array -> save/load.
3. Two-part assembly -> mate -> move/adjust -> save/load.
4. Import project -> apply new operation -> export.
5. Undo/redo stress path over mixed operations.
