import 'dart:async';

import 'package:desktop_click_outside/desktop_click_outside.dart';
import 'package:desktop_click_outside/desktop_click_outside_method_channel.dart';
import 'package:desktop_click_outside/desktop_click_outside_platform_interface.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';

class MockDesktopClickOutsidePlatform
    with MockPlatformInterfaceMixin
    implements DesktopClickOutsidePlatform {
  final StreamController<void> controller = StreamController<void>.broadcast();
  Duration? lastGracePeriod;
  bool started = false;

  @override
  Stream<void> get onClickOutside => controller.stream;

  @override
  Future<bool> isSupported() => Future.value(true);

  @override
  Future<void> startWatching({required Duration gracePeriod}) async {
    started = true;
    lastGracePeriod = gracePeriod;
  }

  @override
  Future<void> stopWatching() async {
    started = false;
  }
}

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  final initialPlatform = DesktopClickOutsidePlatform.instance;

  test('$MethodChannelDesktopClickOutside is the default instance', () {
    expect(initialPlatform, isInstanceOf<MethodChannelDesktopClickOutside>());
  });

  test('delegates watcher API to platform implementation', () async {
    final fakePlatform = MockDesktopClickOutsidePlatform();
    DesktopClickOutsidePlatform.instance = fakePlatform;

    expect(await DesktopClickOutside.instance.isSupported(), isTrue);

    await DesktopClickOutside.instance.startWatching(
      gracePeriod: const Duration(milliseconds: 450),
    );
    expect(fakePlatform.started, isTrue);
    expect(fakePlatform.lastGracePeriod, const Duration(milliseconds: 450));

    await DesktopClickOutside.instance.stopWatching();
    expect(fakePlatform.started, isFalse);
  });

  test('forwards outside-click stream from platform implementation', () async {
    final fakePlatform = MockDesktopClickOutsidePlatform();
    DesktopClickOutsidePlatform.instance = fakePlatform;

    final future = DesktopClickOutside.instance.onClickOutside.first;
    fakePlatform.controller.add(null);

    await expectLater(future, completes);
  });
}
