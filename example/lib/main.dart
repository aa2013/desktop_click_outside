import 'dart:async';

import 'package:desktop_click_outside/desktop_click_outside.dart';
import 'package:flutter/material.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  bool? _supported;
  int _outsideClickCount = 0;
  StreamSubscription<void>? _subscription;

  @override
  void initState() {
    super.initState();
    _initPlugin();
  }

  Future<void> _initPlugin() async {
    final supported = await DesktopClickOutside.instance.isSupported();
    _subscription = DesktopClickOutside.instance.onClickOutside.listen((_) {
      setState(() {
        _outsideClickCount++;
      });
    });
    if (!mounted) return;
    setState(() {
      _supported = supported;
    });
  }

  @override
  void dispose() {
    _subscription?.cancel();
    DesktopClickOutside.instance.stopWatching();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final supported = _supported;
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(title: const Text('Desktop click outside example')),
        body: Center(
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Text('Supported: ${supported ?? 'checking'}'),
              Text('Outside clicks: $_outsideClickCount'),
              const SizedBox(height: 16),
              FilledButton(
                onPressed: supported == true
                    ? () => DesktopClickOutside.instance.startWatching()
                    : null,
                child: const Text('Start watching'),
              ),
              TextButton(
                onPressed: () => DesktopClickOutside.instance.stopWatching(),
                child: const Text('Stop watching'),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
