# Contributing to King Panel

Thanks for helping improve King Panel.

## Build

Use Zig 0.13.0 for reproducible portable builds:

```bat
build-portable.cmd
```

To package an installer, use NSIS 3 with an existing `KingPanel.exe` in the `dist` folder:

```bat
build-installer.cmd
```

The two scripts are intentionally independent: the portable builder does not require NSIS, and the installer builder does not require Zig. The project builds with warnings treated as errors.

## Before opening a pull request

1. Confirm `build-portable.cmd` succeeds when application code changes.
2. Confirm `build-installer.cmd` succeeds when release packaging changes.
3. Test tray-menu mouse and keyboard behavior.
4. Test both Windows light and dark themes if menu rendering changed.
5. Test any display-changing behavior with the 15-second revert path.
6. Do not commit generated EXEs or `kingpanel.res`; attach release binaries to GitHub Releases instead.

## Version changes

Keep release versions synchronized in:

- `kingpanel.rc`
- `setup.nsi`

The release workflow rejects a version tag that does not match the installer version.
