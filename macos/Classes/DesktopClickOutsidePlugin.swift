import Cocoa
import FlutterMacOS

public class DesktopClickOutsidePlugin: NSObject, FlutterPlugin {
  private static let channelName = "desktop_click_outside"
  private static let methodIsSupported = "isSupported"
  private static let methodStartWatching = "startWatching"
  private static let methodStopWatching = "stopWatching"
  private static let methodOnClickOutside = "onClickOutside"
  private static let gracePeriodKey = "gracePeriodMs"
  private static let defaultGracePeriodMs = 300

  private let registrar: FlutterPluginRegistrar
  private let channel: FlutterMethodChannel
  private var globalMonitor: Any?
  private var localMonitor: Any?
  private var watchingSince: Date?
  private var gracePeriod: TimeInterval = TimeInterval(defaultGracePeriodMs) / 1000.0

  public static func register(with registrar: FlutterPluginRegistrar) {
    let channel = FlutterMethodChannel(name: channelName, binaryMessenger: registrar.messenger)
    let instance = DesktopClickOutsidePlugin(registrar: registrar, channel: channel)
    registrar.addMethodCallDelegate(instance, channel: channel)
  }

  init(registrar: FlutterPluginRegistrar, channel: FlutterMethodChannel) {
    self.registrar = registrar
    self.channel = channel
    super.init()
  }

  deinit {
    stopWatching()
  }

  public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
    switch call.method {
    case Self.methodIsSupported:
      result(registrar.view?.window != nil)
    case Self.methodStartWatching:
      let arguments = call.arguments as? [String: Any]
      let gracePeriodMs = arguments?[Self.gracePeriodKey] as? Int ?? Self.defaultGracePeriodMs
      startWatching(gracePeriodMs: max(0, gracePeriodMs))
      result(nil)
    case Self.methodStopWatching:
      stopWatching()
      result(nil)
    default:
      result(FlutterMethodNotImplemented)
    }
  }

  private func startWatching(gracePeriodMs: Int) {
    stopWatching()
    gracePeriod = TimeInterval(gracePeriodMs) / 1000.0
    watchingSince = Date()

    // The global monitor observes clicks delivered to other applications only.
    globalMonitor = NSEvent.addGlobalMonitorForEvents(matching: [.leftMouseDown, .rightMouseDown, .otherMouseDown]) { [weak self] _ in
      self?.notifyAfterGracePeriod()
    }

    // The local monitor covers clicks inside this application, including the main window.
    localMonitor = NSEvent.addLocalMonitorForEvents(matching: [.leftMouseDown, .rightMouseDown, .otherMouseDown]) { [weak self] event in
      self?.handleLocalMouseDown(event)
      return event
    }
  }

  private func stopWatching() {
    if let globalMonitor {
      NSEvent.removeMonitor(globalMonitor)
      self.globalMonitor = nil
    }
    if let localMonitor {
      NSEvent.removeMonitor(localMonitor)
      self.localMonitor = nil
    }
    watchingSince = nil
  }

  private func handleLocalMouseDown(_ event: NSEvent) {
    guard let mainWindow = registrar.view?.window else {
      return
    }
    // Only the main app window is considered outside; popup windows are ignored.
    if event.window === mainWindow {
      notifyAfterGracePeriod()
    }
  }

  private func notifyAfterGracePeriod() {
    guard let watchingSince else {
      return
    }
    if Date().timeIntervalSince(watchingSince) < gracePeriod {
      return
    }
    channel.invokeMethod(Self.methodOnClickOutside, arguments: nil)
  }
}
