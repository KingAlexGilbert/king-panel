# Contributing to King Panel

Thanks for helping improve King Panel.

## Build

Use Zig 0.13.0 for reproducible local builds:

```bat
build.cmd app
```

For installer changes, also install NSIS 3 and run:

```bat
build.cmd installer
```

The project builds with warnings treated as errors.

## Before opening a pull request

1. Confirm `build.cmd app` succeeds.
2. If you changed `setup.nsi`, confirm `build.cmd installer` succeeds.
3. Test tray-menu mouse and keyboard behavior.
4. Test both Windows light and dark themes if menu rendering changed.
5. Test any display-changing behavior with the 15-second revert path.
6. Do not commit generated EXEs or `kingpanel.res`; attach release binaries to GitHub Releases instead.

## Version changes

Keep release versions synchronized in:

- `kingpanel.rc`
- `setup.nsi`

The release workflow rejects a version tag that does not match the installer version.
