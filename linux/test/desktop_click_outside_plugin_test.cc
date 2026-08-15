#include <flutter_linux/flutter_linux.h>
#include <gtest/gtest.h>

#include "desktop_click_outside_plugin_private.h"
#include "include/desktop_click_outside/desktop_click_outside_plugin.h"

namespace desktop_click_outside {
namespace test {

TEST(DesktopClickOutsidePlugin, IsSupportedReturnsBool) {
  g_autoptr(FlMethodResponse) response =
      desktop_click_outside_is_supported_response();
  ASSERT_NE(response, nullptr);
  ASSERT_TRUE(FL_IS_METHOD_SUCCESS_RESPONSE(response));
  FlValue* result =
      fl_method_success_response_get_result(FL_METHOD_SUCCESS_RESPONSE(response));
  ASSERT_EQ(fl_value_get_type(result), FL_VALUE_TYPE_BOOL);
}

}  // namespace test
}  // namespace desktop_click_outside
