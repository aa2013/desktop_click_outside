import 'desktop_click_outside_platform_interface.dart';

class DesktopClickOutside {
  DesktopClickOutside._();

  static final DesktopClickOutside instance = DesktopClickOutside._();

  /// Emits whenever the platform detects a mouse click outside the popup window.
  Stream<void> get onClickOutside => DesktopClickOutsidePlatform.instance.onClickOutside;

  /// Returns whether the current desktop backend supports passive outside-click watching.
  Future<bool> isSupported() {
    return DesktopClickOutsidePlatform.instance.isSupported();
  }

  /// Starts passive outside-click watching.
  ///
  /// [gracePeriod] is applied by the native implementation so the click that
  /// opened a popup cannot immediately close it again.
  Future<void> startWatching({
    Duration gracePeriod = const Duration(milliseconds: 300),
  }) {
    return DesktopClickOutsidePlatform.instance.startWatching(
      gracePeriod: gracePeriod,
    );
  }

  /// Stops watching. Implementations must treat repeated calls as no-ops.
  Future<void> stopWatching() {
    return DesktopClickOutsidePlatform.instance.stopWatching();
  }
}
