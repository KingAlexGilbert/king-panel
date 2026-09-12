# King Panel

An extremely lightweight refresh-rate and resolution modifier for Windows.

King Panel lives in the notification area. Left-click the crown for refresh rates
at your current resolution; right-click for monitor resolutions, refresh rates,
scaling, supported primary-display HDR, and Exit. It supports multiple monitors
and follows the Windows app theme.

## Install

Download `KingPanel-Setup-0.9.7.exe` from this repository's Releases page.
Setup installs to `Program Files\King Panel` and offers optional startup,
desktop, and Start menu shortcuts. These options apply to all users.
Setup and uninstall require administrator approval; the app does not request it.
A portable `KingPanel.exe` is also available. Exit an existing copy before updating.

Resolution and refresh-rate changes revert after 15 seconds unless you choose Keep.
Executables are unsigned. Submenus open upward, but can briefly appear in Windows'
initial position before moving. Scaling support depends on the Windows display driver.

## Build

Install Zig 0.13.0 outside this folder. To build the installer, also install NSIS 3.

```bat
build.cmd app
build.cmd installer
```

The script finds installed tools or accepts full paths through `ZIG_EXE` and
`NSIS_EXE`. It does not download tools. Output files appear in the project root.

## First release

This package uses version **0.9.7**, matching the supplied release binaries.
The `dist` folder contains the supplied EXEs and SHA-256 checksums for manual
release upload. It is ignored by Git; commit the source and repository files.

The existing Release workflow builds and publishes binaries when you push a
matching version tag, **`v0.9.7`**. See [RELEASING.md](RELEASING.md) for steps.

## License

See [LICENSE](LICENSE).
