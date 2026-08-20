## 0.0.1

* 新增桌面端弹窗外部点击监听能力，支持 Windows、macOS 与 Linux（X11）。
* 提供 `onClickOutside` 事件流，原生层被动监听并上报弹窗外鼠标点击。
* `startWatching` 支持 `gracePeriod` 宽限期参数（默认 300ms），避免打开弹窗的点击立即触发关闭。
* 提供 `isSupported` 平台能力检测，Wayland 下返回 `false`。
---
* Add desktop outside-click watching for Windows, macOS, and Linux (X11).
* Provide an `onClickOutside` event stream reported passively from the native layer.
* Support a `gracePeriod` argument (default 300ms) in `startWatching` so the click that opened the popup cannot immediately close it.
* Add `isSupported` capability detection; returns `false` on Wayland.
