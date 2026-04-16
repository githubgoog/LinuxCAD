# LinuxCAD Deployment (Native Desktop)

LinuxCAD deploys as a native Rust desktop binary.

## Build Release Artifacts

```bash
./build-linuxcad.sh
```

## Primary Artifact
- `frontend-rust/target/release/frontend-rust`

## Deployment Model
- Ship the native desktop binary and required runtime libraries for target OS.
- CAD operations run locally through in-process Rust engine integration.

## Verification
Run locally after build:

```bash
./run --release
```
