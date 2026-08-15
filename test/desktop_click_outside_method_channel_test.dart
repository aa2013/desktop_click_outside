import 'package:desktop_click_outside/desktop_click_outside_method_channel.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';

void main() {
  TestWidgetsFlutterBinding.ensureInitialized();

  late MethodChannelDesktopClickOutside platform;
  late List<MethodCall> log;

  setUp(() {
    platform = MethodChannelDesktopClickOutside();
    log = <MethodCall>[];

    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger.setMockMethodCallHandler(platform.methodChannel, (methodCall) async {
      log.add(methodCall);
      switch (methodCall.method) {
        case 'isSupported':
          return true;
        case 'startWatching':
        case 'stopWatching':
          return null;
        default:
          throw MissingPluginException();
      }
    });
  });

  tearDown(() {
    TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger.setMockMethodCallHandler(platform.methodChannel, null);
  });

  test('isSupported delegates to method channel', () async {
    expect(await platform.isSupported(), isTrue);
    expect(log, <Matcher>[isMethodCall('isSupported', arguments: null)]);
  });

  test('startWatching sends native grace period in milliseconds', () async {
    await platform.startWatching(
      gracePeriod: const Duration(milliseconds: 450),
    );

    expect(
      log,
      <Matcher>[
        isMethodCall(
          'startWatching',
          arguments: {'gracePeriodMs': 450},
        ),
      ],
    );
  });

  test('stopWatching delegates to method channel', () async {
    await platform.stopWatching();

    expect(log, <Matcher>[isMethodCall('stopWatching', arguments: null)]);
  });

  test('native onClickOutside call emits stream event', () async {
    final future = platform.onClickOutside.first;

    await TestDefaultBinaryMessengerBinding.instance.defaultBinaryMessenger.handlePlatformMessage(
      platform.methodChannel.name,
      platform.methodChannel.codec.encodeMethodCall(
        const MethodCall('onClickOutside'),
      ),
      (_) {},
    );

    await expectLater(future, completes);
  });
}
