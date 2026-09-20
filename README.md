<p align="center">
  <img src="docs/images/crown.ico" width="128" alt="King Panel app icon">
</p>

<h1 align="center">King Panel</h1>

King Panel is an extremely lightweight refresh-rate and resolution modifier for Windows and Windows ARM. 
When running at idle, King Panel uses virtually no CPU or GPU resources and only consumes about 1.2MB of RAM.

King Panel supports:
- Refresh rate
- Resolution
- Display Scaling
- Multi-Monitor Support
- HDR Toggle
  
It lives in your system tray, making it extremely easy to use, **the crown controls all**. 
Left click: it brings up a simple menu to change refresh rate.
Right click: it brings up an in-depth menu where you can change your refresh rate, resolution, display scaling, HDR, and settings for multiple monitors.

This was made to avoid the amount of menus Windows 11 makes you go through just to change your monitor settings, especially refresh-rate. 

The project was developed with AI-assisted coding and packaging help, then manually tested, debugged, and polished to give it that human touch.

Made by: **King Alex Gilbert**

## Screenshots

### King Panel in System Tray

![System Tray](screenshots/system-tray.png)

### Menu: Right Click

![Menu: Right Click](screenshots/king-panel-menu.png)

### Menu: Left Click

![Menu: Left Click](screenshots/left-click.png)

## Install

Choose the build for your Windows PC:

| PC architecture | Portable app | Installer |
| --- | --- | --- |
| x64 (Intel / AMD) | `KingPanel.exe` | `KingPanel-Setup-1.0.1.exe` |
| ARM64 (Windows on ARM, such as Snapdragon) | `KingPanel-arm64.exe` | `KingPanel-Setup-1.0.1-arm64.exe` |

The ARM64 app runs natively on Windows on ARM. Both versions use the same application source and features.

### Recommended
1. Download the matching installer from the [latest GitHub release](https://github.com/KingAlexGilbert/king-panel/releases/latest).
2. Double-click the exe.
3. Run through the setup options; setup installs to `Program Files\King Panel`.
4. The installer offers optional startup, desktop, and Start menu shortcuts for all users.
5. Run King Panel from your chosen shortcut or file location.
6. King Panel should now show up in the system tray.

### Portable

Download `KingPanel.exe` for x64 or `KingPanel-arm64.exe` for ARM64. Exit an existing copy before updating.

1. Double-click the exe.
2. King Panel should now show up in the system tray.

## Installation Note

Resolution and refresh-rate changes revert after 15 seconds unless you choose "Keep".

The Windows installer is currently unsigned, so Windows may show an unknown publisher or SmartScreen warning. This is normal for unsigned indie releases.

If you trust this official GitHub release, choose **More info → Run anyway** if SmartScreen appears.

## Build

Keep all four build files in the repository root, alongside the shared `kingpanel.c` and `setup.nsi`:

| Double-click this file | Builds | Output in `dist` |
| --- | --- | --- |
| `build-portable.cmd` | x64 portable app | `KingPanel.exe` |
| `build-installer.cmd` | x64 installer | `KingPanel-Setup-1.0.1.exe` |
| `build-portable-arm64.cmd` | ARM64 portable app | `KingPanel-arm64.exe` |
| `build-installer-arm64.cmd` | ARM64 installer | `KingPanel-Setup-1.0.1-arm64.exe` |

Each architecture uses the same application source, resources, and installer definition. Generated EXEs stay in `dist` and are uploaded to GitHub Releases. The existing `assets`, `docs`, and `screenshots` folders keep their current locations.

The ARM64 files select the architecture automatically and call their corresponding existing builder, keeping the build logic in one place. Portable compilation and installer packaging remain separate, so you do not need both build tools installed on the same PC.

**Portable app**

1. Install Zig 0.13.0
2. Put Zig 0.13.0 into King Panel's repo.
3. Double click build-portable.cmd
4. King Panel.exe will be compiled in the `dist` folder

**Installer**

1. Install NSIS 3.08
2. Have a portable King Panel exe in the `dist` folder
3. Double click build-installer.cmd
4. King Panel-Setup.exe will be compiled in the `dist` folder

### Native ARM64

**Portable app**

1. Install Zig 0.13.0
2. Put Zig 0.13.0 into King Panel's repo.
3. Double click build-portable-arm64.cmd
4. King Panel.exe will be compiled in the `dist` folder

**Installer**

1. Install NSIS 3.08
2. Have a portable King Panel exe in the `dist` folder
3. Double click build-installer-arm64.cmd
4. King Panel-Setup.exe will be compiled in the `dist` folder

**Note**

You can cross-compile the ARM64 app on an x64 Windows PC; an ARM PC is only needed to test its behavior.

## Privacy

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
