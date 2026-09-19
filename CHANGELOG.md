# Changelog

All notable changes to King Panel are documented here.

## [1.0.1] - Menu Fixes
- Split Windows builds into independent `build-portable.cmd` and `build-installer.cmd` scripts so portable builds require only Zig and installer packaging requires only NSIS plus an existing `KingPanel.exe`.
- Store both generated binaries in `dist/` so build outputs stay out of the repository root.

- Fixed King Panel's menus from hiding behind the taskbar.
- Active monitors are numbered sequentially in King Panel instead of exposing sparse Windows GDI display IDs.
- Scaled the keep/revert display confirmation window correctly on high-DPI and 4K displays.
- Switched King Panel to Per-Monitor V2 DPI awareness so hover highlights and click hitboxes stay aligned after changing display scaling.
- Fixed display options sometimes requiring a second click after a previous change by no longer discarding selections when delayed Windows display/settings notifications arrive. Open menus are only closed for an actual DPI transition that would invalidate their geometry.

## [1.0.0] - Initial release

King Panel is an extremely lightweight refresh-rate and resolution modifier for Windows.
When running at idle, King Panel uses virtually no CPU or GPU resources and only consumes about 1.2MB of RAM.

King Panel supports:

- Refresh rate
- Resolution
- Display Scaling
- Multi-Monitor Support
- HDR Toggle

It lives in your system tray, making it extremely easy to use, **the crown controls all.**

Left click: it brings up a simple menu to change refresh rate.
Right click: it brings up an in-depth menu where you can change your refresh rate, resolution, display scaling, HDR, and settings for multiple monitors.
