# Changelog

All notable changes to King Panel are documented here.

## [Unreleased]

## [0.9.4]

- Removed the menu-window subclass introduced in 0.9.3.
- Restored the original rendering path.
- Adjusted submenu positioning through popup-open events instead.
- Retained the dark menu background and repaint behavior after menu movement.
- Removed the extra small submenu arrow.
- Added upward submenu positioning with screen-boundary fallback behavior.

### Testing note

The submenu-positioning behavior still needs visual confirmation across mouse, keyboard, nested-menu, theme, and screen-edge cases. Rapid menu opening can briefly display the menu before it is moved.
