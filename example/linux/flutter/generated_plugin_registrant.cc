//
//  Generated file. Do not edit.
//

// clang-format off

#include "generated_plugin_registrant.h"

#include <desktop_click_outside/desktop_click_outside_plugin.h>

void fl_register_plugins(FlPluginRegistry* registry) {
  g_autoptr(FlPluginRegistrar) desktop_click_outside_registrar =
      fl_plugin_registry_get_registrar_for_plugin(registry, "DesktopClickOutsidePlugin");
  desktop_click_outside_plugin_register_with_registrar(desktop_click_outside_registrar);
}
