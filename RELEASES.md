# LinuxCAD Release Playbook

This repository ships desktop packages for Linux, macOS, and Windows using GitHub Actions.

## One-command local packaging
From [frontend](frontend):

```bash
npm run electron:build:linux
npm run electron:build:mac
npm run electron:build:win
```

Artifacts are written to [frontend/release](frontend/release).

## GitHub release automation
The workflow [release-publish.yml](.github/workflows/release-publish.yml) builds all platforms and attaches files to a GitHub Release.

### Option A: tag push (recommended)
1. Create and push a semantic tag:

```bash
git tag v0.4.0
git push origin v0.4.0
```

2. GitHub Actions builds all OS packages and publishes the release automatically.

### Option B: manual dispatch
1. Open [Release Publish workflow](https://github.com/githubgoog/LinuxCAD/actions/workflows/release-publish.yml).
2. Click Run workflow.
3. Provide a tag value (example: `v0.4.0`).

## Download UX guidance
- Point users to [latest release](https://github.com/githubgoog/LinuxCAD/releases/latest) for stable installers.
- Point testers to [Desktop Build Matrix](https://github.com/githubgoog/LinuxCAD/actions/workflows/desktop-build-matrix.yml) for fresh artifacts.

## Packaging notes
- Rust backend is built per runner OS (`cargo build --release`) in CI.
- Electron bundle includes backend executable in app resources as `backend/server` or `backend/server.exe`.

## Signing and notarization setup
Release workflow supports optional signing via GitHub Secrets. If secrets are absent, builds still complete as unsigned artifacts.

### macOS signing and notarization secrets
- `APPLE_ID`
- `APPLE_APP_SPECIFIC_PASSWORD`
- `APPLE_TEAM_ID`

### Windows signing secrets
- `CSC_LINK`
- `CSC_KEY_PASSWORD`

`CSC_LINK` should point to your signing certificate in a format supported by electron-builder (commonly a base64-encoded `p12` payload).

### Verification behavior
- When secrets exist, CI logs indicate signing/notarization is enabled for that platform.
- When missing, CI logs indicate unsigned fallback and continue publishing artifacts.
