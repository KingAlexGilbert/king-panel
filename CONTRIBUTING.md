# Contributing to King Panel

Thanks for helping improve King Panel.

## Build

Use Zig 0.13.0 for reproducible portable builds:

```bat
build-portable.cmd
```

To package an installer, use NSIS 3.08 or newer with an existing `KingPanel.exe` in the `dist` folder:

```bat
build-installer.cmd
```

For ARM64, use the same source and tools with these two double-clickable build files:

```bat
build-portable-arm64.cmd
build-installer-arm64.cmd
```

Keep all four build files together in the repository root. The original files build x64; the two `-arm64.cmd` files build ARM64. Both architectures share `kingpanel.c`, resources, and `setup.nsi`. ARM64 outputs have an `-arm64` suffix and stay in `dist` alongside the x64 outputs. ARM64 can be cross-compiled on an x64 Windows PC; runtime testing requires Windows on ARM.

Portable compilation and installer packaging are intentionally independent: the portable builder does not require NSIS, and the installer builder does not require Zig. The project builds with warnings treated as errors.

## Before opening a pull request

1. Confirm `build-portable.cmd` and `build-portable-arm64.cmd` succeed when application code changes.
2. Confirm `build-installer.cmd` and `build-installer-arm64.cmd` succeed when release packaging changes.
3. Test tray-menu mouse and keyboard behavior.
4. Test both Windows light and dark themes if menu rendering changed.
5. Test display changes and the 15-second revert path on each supported architecture. Build success alone does not verify ARM64 display-driver behavior.
6. Do not commit generated EXEs or resource files; attach release binaries to GitHub Releases instead.

## Version changes

Keep release versions synchronized in:

- `kingpanel.rc`
- `setup.nsi`

The release workflow rejects a version tag that does not match the installer version.
