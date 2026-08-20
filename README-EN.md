# desktop_click_outside

A desktop Flutter plugin that passively watches for mouse clicks outside a popup (menu) window. Supports Windows, macOS, and Linux (X11).

It listens for global mouse-button events at the native layer, detects whether the click landed outside the popup, and notifies the Dart side — no manual hit-testing required.

---

[简体中文](./README.md) | English

---

## Platform support

| Platform | Support                                  |
|----------|:----------------------------------------|
| Windows  | ✔️ Fully supported                        |
| Linux    | ✔️ X11 only (Wayland has no global mouse watcher) |
| macOS    | ✔️ Fully supported                        |

## Getting started

### Install

Add the dependency to your `pubspec.yaml`:

```yaml
dependencies:
  desktop_click_outside: ^0.0.1
```

### Usage

```dart
import 'package:desktop_click_outside/desktop_click_outside.dart';

class MyWidget extends StatefulWidget {
  const MyWidget({super.key});

  @override
  State<MyWidget> createState() => _MyWidgetState();
}

class _MyWidgetState extends State<MyWidget> {
  StreamSubscription<void>? _subscription;

  @override
  void initState() {
    super.initState();
    _initWatching();
  }

  Future<void> _initWatching() async {
    // Check whether the current desktop backend supports passive watching.
    final supported = await DesktopClickOutside.instance.isSupported();
    if (!supported) return;

    // Subscribe to outside-click events.
    _subscription = DesktopClickOutside.instance.onClickOutside.listen((_) {
      // Close the popup or do something else.
    });

    // Start watching. The default 300ms grace period prevents the click that
    // opened the popup from immediately closing it.
    await DesktopClickOutside.instance.startWatching();
  }

  @override
  void dispose() {
    _subscription?.cancel();
    DesktopClickOutside.instance.stopWatching();
    super.dispose();
  }
}
```

> See the `example` directory for a complete example.

## API

| Method             | Description                                            | Windows | Linux | macOS |
|--------------------|--------------------------------------------------------|:-------:|:-----:|:-----:|
| `isSupported`      | Whether the current desktop backend supports watching. | ✔️      | ✔️    | ✔️    |
| `startWatching`    | Start watching; accepts a `gracePeriod` (default 300ms). | ✔️      | ✔️    | ✔️    |
| `stopWatching`     | Stop watching; repeated calls are safe.                | ✔️      | ✔️    | ✔️    |
| `onClickOutside`   | Stream of outside-click events (`Stream<void>`).       | ✔️      | ✔️    | ✔️    |

## Implementation notes

- Windows: registers a global mouse Raw Input device via `RegisterRawInputDevices` and compares the process ID of the window under the cursor.
- Linux: uses X11 `XInput2` raw button events (`XI_RawButtonPress`); `isSupported` returns `false` on Wayland.
- macOS: uses global and local event monitors. Clicks on the main window or another app's window are reported; popup windows are ignored.

## License

[MIT](./LICENSE)
