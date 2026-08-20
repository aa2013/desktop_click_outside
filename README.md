# desktop_click_outside

桌面端 Flutter 插件，用于被动监听鼠标在弹窗（popup/menu）外部点击的事件，支持 Windows、macOS 与 Linux（X11）。

通过原生层监听全局鼠标按下事件，判断点击位置是否落在弹窗之外，并主动上报给 Dart 侧，无需在 Dart 层手动做命中测试。

---

简体中文 | [English](./README-EN.md)

---

## 平台支持

| 平台    | 支持                                   |
|-------|:-------------------------------------|
| Windows | ✔️ 完全支持                              |
| Linux   | ✔️ 仅支持 X11（Wayland 不提供全局鼠标监听）        |
| macOS   | ✔️ 完全支持                              |

## 快速开始

### 安装

将此依赖添加到你的 `pubspec.yaml`：

```yaml
dependencies:
  desktop_click_outside: ^0.0.1
```

### 用法

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
    // 先检测当前桌面后端是否支持被动监听
    final supported = await DesktopClickOutside.instance.isSupported();
    if (!supported) return;

    // 订阅弹窗外点击事件
    _subscription = DesktopClickOutside.instance.onClickOutside.listen((_) {
      // 关闭弹窗或做其它处理
    });

    // 开始监听，默认 300ms 宽限期可避免“打开弹窗的那一次点击”立即触发关闭
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

> 完整示例见插件仓库中的 `example` 目录。

## API

| 方法          | 描述                                          | Windows | Linux | macOS |
|-------------|---------------------------------------------|:-------:|:-----:|:-----:|
| `isSupported` | 当前桌面后端是否支持被动监听                              | ✔️      | ✔️    | ✔️    |
| `startWatching` | 开始监听，可传入 `gracePeriod` 宽限期（默认 300ms）       | ✔️      | ✔️    | ✔️    |
| `stopWatching`  | 停止监听，重复调用是安全的                                | ✔️      | ✔️    | ✔️    |
| `onClickOutside` | 弹窗外点击事件流（`Stream<void>`）                     | ✔️      | ✔️    | ✔️    |

## 实现说明

- Windows：通过 `RegisterRawInputDevices` 注册全局鼠标 Raw Input，比较点击位置所在窗口的进程 ID。
- Linux：依赖 X11 的 `XInput2` Raw Button 事件（`XI_RawButtonPress`），Wayland 下 `isSupported` 返回 `false`。
- macOS：使用全局与本地事件监视器，点击主窗口或其它应用窗口时触发上报，弹窗窗口被忽略。

## License

[MIT](./LICENSE)
