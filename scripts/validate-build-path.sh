#!/usr/bin/env bash
# conda-forge cross compilers pass --sysroot=... paths via activation scripts
# without quoting. If the conda env path contains spaces, clang splits arguments
# and CMake fails ("no such file or directory: 'CAD/.pixi/.../sysroot'").
#
# Symlinks cannot fix this reliably: kernels resolve cwd to the real path under
# /proc for child processes such as ninja/clang tasks. The remedy is moving or
# recloning without spaces.

set -euo pipefail

root="${PIXI_PROJECT_ROOT:-}"
if [[ -z "${root}" ]]; then
    root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
fi

if [[ "${root}" != *" "* ]]; then
    exit 0
fi

echo >&2 "Cannot build LinuxCAD: path contains spaces:"
printf >&2 '  %q\n\n' "${root}"

cat >&2 <<'STOP'

Conda/Pixi's compiler wrappers pass --sysroot paths that split on directory
segments that include a space ('.../Linux' 'CAD/.pixi/...'), so Clang cannot run.

Fix:

  Rename or move the checkout so no directory name contains a space:

      mv ~/path/to/repo-with-a-space/name ~/LinuxCAD
      cd ~/LinuxCAD
      pixi install
      pixi run linuxcad-release

STOP

exit 1
