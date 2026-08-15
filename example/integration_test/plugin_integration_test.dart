import 'package:desktop_click_outside/desktop_click_outside.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:integration_test/integration_test.dart';

void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  testWidgets('isSupported returns a bool', (WidgetTester tester) async {
    final supported = await DesktopClickOutside.instance.isSupported();
    expect(supported, isA<bool>());
  });
}
