#include "include/desktop_click_outside/desktop_click_outside_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "desktop_click_outside_plugin.h"

void DesktopClickOutsidePluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  desktop_click_outside::DesktopClickOutsidePlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
