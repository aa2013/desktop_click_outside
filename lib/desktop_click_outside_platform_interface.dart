import 'package:plugin_platform_interface/plugin_platform_interface.dart';

import 'desktop_click_outside_method_channel.dart';

abstract class DesktopClickOutsidePlatform extends PlatformInterface {
  DesktopClickOutsidePlatform() : super(token: _token);

  static final Object _token = Object();

  static DesktopClickOutsidePlatform _instance = MethodChannelDesktopClickOutside();

  static DesktopClickOutsidePlatform get instance => _instance;

  static set instance(DesktopClickOutsidePlatform instance) {
    PlatformInterface.verifyToken(instance, _token);
    _instance = instance;
  }

  /// Emits outside-click notifications from the platform implementation.
  Stream<void> get onClickOutside {
    throw UnimplementedError('onClickOutside has not been implemented.');
  }

  /// Returns whether this platform backend can passively observe outside clicks.
  Future<bool> isSupported() {
    throw UnimplementedError('isSupported() has not been implemented.');
  }

  /// Enables the native watcher with a native-side startup grace period.
  Future<void> startWatching({required Duration gracePeriod}) {
    throw UnimplementedError('startWatching() has not been implemented.');
  }

  /// Disables the native watcher. Repeated calls must be safe.
  Future<void> stopWatching() {
    throw UnimplementedError('stopWatching() has not been implemented.');
  }
}
