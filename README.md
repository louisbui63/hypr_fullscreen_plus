# hypr_fullscreen_plus
**Partial Deprecation**: some features originally provided by this plugin are part of Hyprland as of [65f66dc](https://github.com/hyprwm/Hyprland/commit/65f66dcf0d38533a383212ca440fdea0163be276), and are replaced by the `allow_pin_fullscreen` option.

This hyprland plugin still provides :
  - A change in the handling of pinned fullscreen windows, such that un-fullscreening the same window twice won't lead to an unpinned, un-fullscreened window
  - Un-fullscreening a formerly pinned window will fullscreen the window that was fullscreen before that, if applicable. This is to simulate fullscreen window dtacking.

## Install
```
hyprpm add https://github.com/louisbui63/hypr_fullscreen_plus

```
