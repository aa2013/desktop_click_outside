#include <flutter_linux/flutter_linux.h>

#include "include/desktop_click_outside/desktop_click_outside_plugin.h"

// This file exposes some plugin internals for unit testing. See
// https://github.com/flutter/flutter/issues/88724 for current limitations
// in the unit-testable API.

// Builds the isSupported response without requiring a live method call object.
FlMethodResponse *desktop_click_outside_is_supported_response();
