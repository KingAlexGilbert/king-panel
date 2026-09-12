# Changelog

All notable changes to King Panel are documented here.

## [Unreleased]

## [1.0.0] - Initial release

- Added display-query bounds, safe label formatting and scaling arithmetic checks.
- Reduced memory per saved menu choice and removed duplicated display-query code.
- Added stack protection and an explicit normal-user application manifest.
- Hardened installer path checks and reduced installer icon overhead.
- Added DPI-scaled submenu arrows and duplicate-arrow suppression.
- Preserved dark/light menus and upward submenu positioning.

Known limitation: native submenus can briefly appear in their initial position
before moving upward. Per-monitor scaling uses undocumented Windows packets.
The unsuccessful later positioning experiment is not included.

## [0.9.4]

- Removed the menu-window subclass introduced in 0.9.3.
- Restored the original rendering path.
- Adjusted submenu positioning through popup-open events instead.
- Retained the dark menu background and repaint behavior after menu movement.
- Removed the extra small submenu arrow.
- Added upward submenu positioning with screen-boundary fallback behavior.

### Testing note

The submenu-positioning behavior still needs visual confirmation across mouse, keyboard, nested-menu, theme, and screen-edge cases. Rapid menu opening can briefly display the menu before it is moved.
