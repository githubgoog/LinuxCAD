# LinuxCAD

[![Latest release](https://img.shields.io/github/v/release/githubgoog/LinuxCAD?display_name=tag&sort=semver)](https://github.com/githubgoog/LinuxCAD/releases/latest)
[![Desktop Build Matrix](https://github.com/githubgoog/LinuxCAD/actions/workflows/desktop-build-matrix.yml/badge.svg)](https://github.com/githubgoog/LinuxCAD/actions/workflows/desktop-build-matrix.yml)
[![Release Publish](https://github.com/githubgoog/LinuxCAD/actions/workflows/release-publish.yml/badge.svg)](https://github.com/githubgoog/LinuxCAD/actions/workflows/release-publish.yml)

LinuxCAD is a desktop CAD application powered by Electron (frontend) and Rust (geometry/backend services).

## Easy Downloads

### Stable builds
1. Open [Releases](https://github.com/githubgoog/LinuxCAD/releases/latest).
2. Download the installer/package for your OS:
- Linux: `.AppImage` or `.deb`
- macOS: `.dmg`
- Windows: `.exe` (NSIS installer)

### Nightly CI artifacts
1. Open the [Desktop Build Matrix workflow](https://github.com/githubgoog/LinuxCAD/actions/workflows/desktop-build-matrix.yml).
2. Open the latest successful run.
3. Download the `linuxcad-Linux`, `linuxcad-macOS`, or `linuxcad-Windows` artifact.

## Cross-platform support status
- Linux: AppImage and deb package builds in CI.
- macOS: dmg and zip builds in CI.
- Windows: nsis installer and zip builds in CI.

Signed macOS/Windows installers are supported through GitHub Secrets in the release workflow; see [RELEASES.md](RELEASES.md) for setup.

## Developer commands
Run from [frontend](frontend):

```bash
npm run electron:build:linux
npm run electron:build:mac
npm run electron:build:win
```

A full release for all desktop platforms is automated by GitHub Actions in [release-publish.yml](.github/workflows/release-publish.yml).
