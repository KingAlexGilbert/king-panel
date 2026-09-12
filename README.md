# King Panel

King Panel is an extremely lightweight refresh-rate and resolution modifier for Windows. 
When running at idle, King Panel uses virtually no CPU or GPU resources and only consumes about 1.2MB of RAM.

King Panel supports:
- Refresh rate
- Resolution
- Display Scaling
- Multi-Monitor Support
- HDR Toggle
  
It lives in your task bar, making it extremely easy to use, **the crown controls all**. 
Left click: it brings up a simple menu to change refresh rate.
Right click: it brings up an in-depth menu where you can change your refresh rate, resolution, display scaling, HDR, and settings for multiple monitors.

This was made to avoid the amount of menus Windows 11 makes you go through just to change your monitor settings, especially refresh-rate. 

King Panel was made with AI-assisted coding.

Made by: **King Alex Gilbert**

## Install

1. Download `KingPanel-Setup-1.0.0.exe` from this repository's Releases page.
2. Double-click the exe.
3. Run through the setup options; setup installs to `Program Files\King Panel`.
4. The installer offers optional startup, desktop, and Start menu shortcuts for all users.
5. Setup and uninstall require administrator approval; the app does not request it.
6. King Panel should now show up in the taskbar tray.
A portable `KingPanel.exe` is also available. Exit an existing copy before updating.

Resolution and refresh-rate changes revert after 15 seconds unless you choose "Keep".

## Installation Note

The Windows installer is currently unsigned, so Windows may show an unknown publisher or SmartScreen warning. This is normal for unsigned indie releases.

If you trust this official GitHub release, choose **More info → Run anyway** if SmartScreen appears.

## Build

Install Zig 0.13.0 outside this folder. To build the installer, also install NSIS 3.

```bat
build.cmd app
build.cmd installer
```

The script finds installed tools or accepts full paths through `ZIG_EXE` and
`NSIS_EXE`. It does not download tools. Output files appear in the project root.

## Privacy

This package uses version **1.0.0**, with freshly rebuilt release binaries.
The `dist` folder contains the rebuilt EXEs and SHA-256 checksums for manual
release upload. It is ignored by Git; commit the source and repository files.

The existing Release workflow builds and publishes binaries when you push a
matching version tag, **`v1.0.0`**. See [RELEASING.md](RELEASING.md) for steps.

King Panel is local-first. Your display information and settings are processed locally and are not sent to external servers.

## License

This project is released under the GNU General Public License v3.0.

Distributed modified versions must follow the terms of the GPLv3. See the `LICENSE` file for the complete license terms.

Copyright (C) 2026 King Alex Gilbert

## References

I want to give credit to the GitHub projects that helped inspire King Panel:

- [RefreshRateSwitcher](https://github.com/Yeeyash/refresh-rate-switcher) by [@Yeeyash](https://github.com/Yeeyash)
- [windisplay](https://github.com/zpix1/windisplay) by [@zpix1](https://github.com/zpix1)

Special thanks to their developers for sharing their work and helping inspire King Panel.
