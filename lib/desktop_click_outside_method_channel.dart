import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

import 'desktop_click_outside_platform_interface.dart';

class MethodChannelDesktopClickOutside extends DesktopClickOutsidePlatform {
  MethodChannelDesktopClickOutside() {
    methodChannel.setMethodCallHandler(handleMethodCall);
  }

  @visibleForTesting
  final methodChannel = const MethodChannel('desktop_click_outside');

  final StreamController<void> _clickOutsideController =
      StreamController<void>.broadcast();

  @override
  Stream<void> get onClickOutside => _clickOutsideController.stream;

  @visibleForTesting
  Future<void> handleMethodCall(MethodCall call) async {
    switch (call.method) {
      case 'onClickOutside':
        _clickOutsideController.add(null);
        return;
      default:
        throw MissingPluginException('No handler for ${call.method}');
    }
  }

  @override
  Future<bool> isSupported() async {
    return await methodChannel.invokeMethod<bool>('isSupported') ?? false;
  }

  @override
  Future<void> startWatching({required Duration gracePeriod}) {
    return methodChannel.invokeMethod<void>('startWatching', {
      'gracePeriodMs': gracePeriod.inMilliseconds,
    });
  }

  @override
  Future<void> stopWatching() {
    return methodChannel.invokeMethod<void>('stopWatching');
  }
}
