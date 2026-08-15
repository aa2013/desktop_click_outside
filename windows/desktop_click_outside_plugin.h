#ifndef FLUTTER_PLUGIN_DESKTOP_CLICK_OUTSIDE_PLUGIN_H_
#define FLUTTER_PLUGIN_DESKTOP_CLICK_OUTSIDE_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>

#include <memory>

namespace desktop_click_outside {

class DesktopClickOutsidePlugin : public flutter::Plugin {
 public:
  static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

  DesktopClickOutsidePlugin();

  explicit DesktopClickOutsidePlugin(
      flutter::PluginRegistrarWindows* registrar,
      flutter::MethodChannel<flutter::EncodableValue>* channel);

  virtual ~DesktopClickOutsidePlugin();

  // Disallow copy and assign.
  DesktopClickOutsidePlugin(const DesktopClickOutsidePlugin&) = delete;
  DesktopClickOutsidePlugin& operator=(const DesktopClickOutsidePlugin&) = delete;

  // Called when a method is called on this plugin's channel from Dart.
  void HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

 private:
  bool IsSupported() const;
  bool EnsureMessageWindow();
  bool EnsureRawInputRegistered();
  void StartWatching(int grace_period_ms);
  void StopWatching();
  void UnregisterRawInput();
  void DestroyMessageWindow();
  void NotifyClickOutside();
  LRESULT HandleMessageWindowProc(
      HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
  static LRESULT CALLBACK MessageWindowProc(
      HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

  flutter::PluginRegistrarWindows* registrar_ = nullptr;
  flutter::MethodChannel<flutter::EncodableValue>* channel_ = nullptr;
  HWND main_window_ = nullptr;
  HWND message_window_ = nullptr;
  bool raw_input_registered_ = false;
  bool watching_click_ = false;
  ULONGLONG watching_since_ = 0;
  ULONGLONG grace_period_ms_ = 300;
};

}  // namespace desktop_click_outside

#endif  // FLUTTER_PLUGIN_DESKTOP_CLICK_OUTSIDE_PLUGIN_H_
