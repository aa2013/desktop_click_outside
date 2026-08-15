#include <flutter/encodable_value.h>
#include <flutter/method_call.h>
#include <flutter/method_result_functions.h>
#include <gtest/gtest.h>

#include <memory>
#include <variant>

#include "desktop_click_outside_plugin.h"

namespace desktop_click_outside {
namespace test {

namespace {

using flutter::EncodableMap;
using flutter::EncodableValue;
using MethodCall = flutter::MethodCall<flutter::EncodableValue>;
using flutter::MethodResultFunctions;

}  // namespace

TEST(DesktopClickOutsidePlugin, IsSupportedReturnsFalseWithoutRegistrar) {
  DesktopClickOutsidePlugin plugin;
  bool supported = true;
  plugin.HandleMethodCall(
      MethodCall("isSupported", std::make_unique<EncodableValue>()),
      std::make_unique<MethodResultFunctions<>>(
          [&supported](const EncodableValue* result) {
            supported = std::get<bool>(*result);
          },
          nullptr, nullptr));

  EXPECT_FALSE(supported);
}

TEST(DesktopClickOutsidePlugin, StartAndStopAreSafeWithoutRegistrar) {
  DesktopClickOutsidePlugin plugin;
  int success_count = 0;
  auto success = [&success_count](const EncodableValue* result) {
    success_count++;
  };

  EncodableMap args;
  args[EncodableValue("gracePeriodMs")] = EncodableValue(300);
  plugin.HandleMethodCall(
      MethodCall("startWatching", std::make_unique<EncodableValue>(args)),
      std::make_unique<MethodResultFunctions<>>(success, nullptr, nullptr));
  plugin.HandleMethodCall(
      MethodCall("stopWatching", std::make_unique<EncodableValue>()),
      std::make_unique<MethodResultFunctions<>>(success, nullptr, nullptr));

  EXPECT_EQ(success_count, 2);
}

}  // namespace test
}  // namespace desktop_click_outside
