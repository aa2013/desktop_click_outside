#include "desktop_click_outside_plugin.h"

#include <windows.h>

#include <cstdio>
#include <flutter/encodable_value.h>
#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <algorithm>
#include <memory>
#include <vector>

namespace desktop_click_outside {

namespace {

constexpr char kChannelName[] = "desktop_click_outside";
constexpr char kMethodIsSupported[] = "isSupported";
constexpr char kMethodStartWatching[] = "startWatching";
constexpr char kMethodStopWatching[] = "stopWatching";
constexpr char kMethodOnClickOutside[] = "onClickOutside";
constexpr char kGracePeriodMsKey[] = "gracePeriodMs";
constexpr int kDefaultGracePeriodMs = 300;
constexpr wchar_t kMessageWindowClassName[] =
    L"DesktopClickOutsideRawInputWindow";

int GetGracePeriodMs(const flutter::EncodableValue* arguments) {
  if (!arguments || !std::holds_alternative<flutter::EncodableMap>(*arguments)) {
    return kDefaultGracePeriodMs;
  }
  const auto& map = std::get<flutter::EncodableMap>(*arguments);
  const auto value = map.find(flutter::EncodableValue(kGracePeriodMsKey));
  if (value == map.end()) {
    return kDefaultGracePeriodMs;
  }
  if (std::holds_alternative<int>(value->second)) {
    const int grace_period_ms = std::get<int>(value->second);
    return grace_period_ms < 0 ? 0 : grace_period_ms;
  }
  if (std::holds_alternative<int64_t>(value->second)) {
    const int64_t grace_period_ms = std::get<int64_t>(value->second);
    return static_cast<int>(grace_period_ms < 0 ? 0 : grace_period_ms);
  }
  return kDefaultGracePeriodMs;
}

void LogLastError(const char* message) {
  char buffer[192];
  sprintf_s(buffer, sizeof(buffer), "[desktop_click_outside] %s, err=%lu",
            message, ::GetLastError());
  ::OutputDebugStringA(buffer);
}

}  // namespace

// static
void DesktopClickOutsidePlugin::RegisterWithRegistrar(
    flutter::PluginRegistrarWindows* registrar) {
  auto channel =
      std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
          registrar->messenger(), kChannelName,
          &flutter::StandardMethodCodec::GetInstance());

  auto plugin =
      std::make_unique<DesktopClickOutsidePlugin>(registrar, channel.get());

  channel->SetMethodCallHandler(
      [plugin_pointer = plugin.get()](const auto& call, auto result) {
        plugin_pointer->HandleMethodCall(call, std::move(result));
      });

  registrar->AddPlugin(std::move(plugin));
  channel.release();
}

DesktopClickOutsidePlugin::DesktopClickOutsidePlugin() = default;

DesktopClickOutsidePlugin::DesktopClickOutsidePlugin(
    flutter::PluginRegistrarWindows* registrar,
    flutter::MethodChannel<flutter::EncodableValue>* channel)
    : registrar_(registrar), channel_(channel) {
  if (registrar_ && registrar_->GetView()) {
    main_window_ = ::GetAncestor(registrar_->GetView()->GetNativeWindow(), GA_ROOT);
    EnsureMessageWindow();
  }
}

DesktopClickOutsidePlugin::~DesktopClickOutsidePlugin() {
  StopWatching();
  UnregisterRawInput();
  DestroyMessageWindow();
  delete channel_;
  channel_ = nullptr;
}

bool DesktopClickOutsidePlugin::IsSupported() const {
  return main_window_ != nullptr && channel_ != nullptr &&
         message_window_ != nullptr;
}

bool DesktopClickOutsidePlugin::EnsureMessageWindow() {
  if (message_window_) {
    return true;
  }

  HINSTANCE instance = ::GetModuleHandle(nullptr);
  WNDCLASSW window_class{};
  window_class.lpfnWndProc = DesktopClickOutsidePlugin::MessageWindowProc;
  window_class.hInstance = instance;
  window_class.lpszClassName = kMessageWindowClassName;

  if (!::RegisterClassW(&window_class) &&
      ::GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
    LogLastError("RegisterClassW failed");
    return false;
  }

  // Raw Input is delivered to this hidden window instead of Flutter's top-level
  // window proc delegate. This keeps the plugin independent from the host
  // runner's message-dispatch ordering.
  message_window_ = ::CreateWindowExW(
      0, kMessageWindowClassName, L"", 0, 0, 0, 0, 0, nullptr, nullptr,
      instance, this);
  if (!message_window_) {
    LogLastError("CreateWindowExW failed");
    return false;
  }
  return true;
}

bool DesktopClickOutsidePlugin::EnsureRawInputRegistered() {
  if (raw_input_registered_) {
    return true;
  }
  if (!main_window_ || !EnsureMessageWindow()) {
    return false;
  }

  RAWINPUTDEVICE rid{};
  rid.usUsagePage = 0x01;
  rid.usUsage = 0x02;
  rid.dwFlags = RIDEV_INPUTSINK;
  rid.hwndTarget = message_window_;
  if (!::RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
    LogLastError("RegisterRawInputDevices failed");
    return false;
  }
  raw_input_registered_ = true;
  return true;
}

void DesktopClickOutsidePlugin::StartWatching(int grace_period_ms) {
  if (!EnsureRawInputRegistered()) {
    return;
  }
  grace_period_ms_ = static_cast<ULONGLONG>(
      grace_period_ms < 0 ? 0 : grace_period_ms);
  watching_since_ = ::GetTickCount64();
  watching_click_ = true;
}

void DesktopClickOutsidePlugin::StopWatching() {
  watching_click_ = false;
}

void DesktopClickOutsidePlugin::UnregisterRawInput() {
  if (!raw_input_registered_) {
    return;
  }
  RAWINPUTDEVICE rid{};
  rid.usUsagePage = 0x01;
  rid.usUsage = 0x02;
  rid.dwFlags = RIDEV_REMOVE;
  rid.hwndTarget = nullptr;
  ::RegisterRawInputDevices(&rid, 1, sizeof(rid));
  raw_input_registered_ = false;
}

void DesktopClickOutsidePlugin::DestroyMessageWindow() {
  if (!message_window_) {
    return;
  }
  ::DestroyWindow(message_window_);
  message_window_ = nullptr;
}

void DesktopClickOutsidePlugin::NotifyClickOutside() {
  if (!channel_) {
    return;
  }
  channel_->InvokeMethod(kMethodOnClickOutside,
                         std::make_unique<flutter::EncodableValue>());
}

LRESULT DesktopClickOutsidePlugin::HandleMessageWindowProc(
    HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
  if (message != WM_INPUT) {
    return ::DefWindowProc(hwnd, message, wparam, lparam);
  }

  if (watching_click_ && ::GetTickCount64() - watching_since_ >= grace_period_ms_) {
    UINT size = 0;
    if (::GetRawInputData(reinterpret_cast<HRAWINPUT>(lparam), RID_INPUT, nullptr,
                          &size, sizeof(RAWINPUTHEADER)) != static_cast<UINT>(-1) &&
        size > 0) {
      std::vector<BYTE> buffer(size);
      if (::GetRawInputData(reinterpret_cast<HRAWINPUT>(lparam), RID_INPUT,
                            buffer.data(), &size, sizeof(RAWINPUTHEADER)) !=
          static_cast<UINT>(-1)) {
        RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buffer.data());
        if (raw->header.dwType == RIM_TYPEMOUSE) {
          const USHORT down =
              raw->data.mouse.usButtonFlags &
              (RI_MOUSE_LEFT_BUTTON_DOWN | RI_MOUSE_RIGHT_BUTTON_DOWN |
               RI_MOUSE_MIDDLE_BUTTON_DOWN | RI_MOUSE_BUTTON_4_DOWN |
               RI_MOUSE_BUTTON_5_DOWN);
          if (down) {
            POINT point;
            ::GetCursorPos(&point);
            HWND root = ::GetAncestor(::WindowFromPoint(point), GA_ROOT);
            DWORD pid = 0;
            if (root) {
              ::GetWindowThreadProcessId(root, &pid);
            }
            if (pid != ::GetCurrentProcessId() || root == main_window_) {
              NotifyClickOutside();
            }
          }
        }
      }
    }
  }

  return ::DefWindowProc(hwnd, message, wparam, lparam);
}

LRESULT CALLBACK DesktopClickOutsidePlugin::MessageWindowProc(
    HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
  if (message == WM_NCCREATE) {
    const auto* create_struct = reinterpret_cast<CREATESTRUCT*>(lparam);
    auto* plugin = reinterpret_cast<DesktopClickOutsidePlugin*>(
        create_struct->lpCreateParams);
    ::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(plugin));
  }

  auto* plugin = reinterpret_cast<DesktopClickOutsidePlugin*>(
      ::GetWindowLongPtr(hwnd, GWLP_USERDATA));
  if (message == WM_NCDESTROY) {
    ::SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
  }
  if (!plugin) {
    return ::DefWindowProc(hwnd, message, wparam, lparam);
  }
  return plugin->HandleMessageWindowProc(hwnd, message, wparam, lparam);
}

void DesktopClickOutsidePlugin::HandleMethodCall(
    const flutter::MethodCall<flutter::EncodableValue>& method_call,
    std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result) {
  const auto& method = method_call.method_name();
  if (method == kMethodIsSupported) {
    result->Success(flutter::EncodableValue(IsSupported()));
  } else if (method == kMethodStartWatching) {
    StartWatching(GetGracePeriodMs(method_call.arguments()));
    result->Success();
  } else if (method == kMethodStopWatching) {
    StopWatching();
    result->Success();
  } else {
    result->NotImplemented();
  }
}

}  // namespace desktop_click_outside
