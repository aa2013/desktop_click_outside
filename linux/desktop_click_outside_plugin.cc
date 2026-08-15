#include "include/desktop_click_outside/desktop_click_outside_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <unistd.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>

#include <algorithm>
#include <cstring>

#include "desktop_click_outside_plugin_private.h"

#define DESKTOP_CLICK_OUTSIDE_PLUGIN(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), desktop_click_outside_plugin_get_type(), \
                              DesktopClickOutsidePlugin))

namespace {

constexpr char kChannelName[] = "desktop_click_outside";
constexpr char kMethodIsSupported[] = "isSupported";
constexpr char kMethodStartWatching[] = "startWatching";
constexpr char kMethodStopWatching[] = "stopWatching";
constexpr char kMethodOnClickOutside[] = "onClickOutside";
constexpr char kGracePeriodMsKey[] = "gracePeriodMs";
constexpr gint64 kDefaultGracePeriodMs = 300;

}  // namespace

struct _DesktopClickOutsidePlugin {
  GObject parent_instance;
  FlPluginRegistrar* registrar;
  FlMethodChannel* channel;
  GdkWindow* root_window;
  Window main_window;
  gint xinput_opcode;
  gint64 grace_period_ms;
  gint64 watching_since_ms;
  gboolean filter_installed;
  gboolean watching;
};

G_DEFINE_TYPE(DesktopClickOutsidePlugin, desktop_click_outside_plugin,
              g_object_get_type())

static gboolean is_x11_supported() {
  GdkDisplay* display = gdk_display_get_default();
  return display != nullptr && GDK_IS_X11_DISPLAY(display);
}

FlMethodResponse* desktop_click_outside_is_supported_response() {
  g_autoptr(FlValue) result = fl_value_new_bool(is_x11_supported());
  return FL_METHOD_RESPONSE(fl_method_success_response_new(result));
}

static gint64 now_ms() {
  return g_get_monotonic_time() / 1000;
}

static gint64 grace_period_from_args(FlValue* args) {
  if (args == nullptr || fl_value_get_type(args) != FL_VALUE_TYPE_MAP) {
    return kDefaultGracePeriodMs;
  }
  FlValue* value = fl_value_lookup_string(args, kGracePeriodMsKey);
  if (value == nullptr || fl_value_get_type(value) != FL_VALUE_TYPE_INT) {
    return kDefaultGracePeriodMs;
  }
  return std::max<gint64>(0, fl_value_get_int(value));
}

static Window get_main_window(DesktopClickOutsidePlugin* self) {
  if (self->registrar == nullptr) {
    return 0;
  }
  FlView* view = fl_plugin_registrar_get_view(self->registrar);
  if (view == nullptr) {
    return 0;
  }
  GtkWidget* top_level = gtk_widget_get_toplevel(GTK_WIDGET(view));
  if (!GTK_IS_WINDOW(top_level)) {
    return 0;
  }
  GdkWindow* window = gtk_widget_get_window(top_level);
  if (window == nullptr || !GDK_IS_X11_WINDOW(window)) {
    return 0;
  }
  return GDK_WINDOW_XID(window);
}

static guint get_window_pid(Display* display, Window window) {
  if (display == nullptr || window == 0) {
    return 0;
  }
  Atom pid_atom = XInternAtom(display, "_NET_WM_PID", True);
  if (pid_atom == None) {
    return 0;
  }

  Atom type = None;
  int format = 0;
  unsigned long nitems = 0;
  unsigned long bytes_after = 0;
  unsigned char* prop = nullptr;
  guint pid = 0;
  if (XGetWindowProperty(display, window, pid_atom, 0, 1, False, XA_CARDINAL,
                         &type, &format, &nitems, &bytes_after, &prop) ==
          Success &&
      prop != nullptr && nitems > 0) {
    pid = *reinterpret_cast<guint*>(prop);
    XFree(prop);
  }
  return pid;
}

static Window find_client_window_with_pid(Display* display, Window window,
                                          guint* pid, int depth = 0) {
  if (display == nullptr || window == 0 || depth > 6) {
    return 0;
  }

  guint current_pid = get_window_pid(display, window);
  if (current_pid != 0) {
    *pid = current_pid;
    return window;
  }

  Window root = 0;
  Window parent = 0;
  Window* children = nullptr;
  unsigned int child_count = 0;
  if (!XQueryTree(display, window, &root, &parent, &children, &child_count)) {
    return 0;
  }

  Window result = 0;
  for (unsigned int i = 0; i < child_count; ++i) {
    result = find_client_window_with_pid(display, children[i], pid, depth + 1);
    if (result != 0) {
      break;
    }
  }
  if (children != nullptr) {
    XFree(children);
  }
  return result;
}

static Window query_window_under_pointer(Display* display) {
  Window root = DefaultRootWindow(display);
  Window child = 0;
  Window unused_root = 0;
  int root_x = 0;
  int root_y = 0;
  int win_x = 0;
  int win_y = 0;
  unsigned int mask = 0;
  if (!XQueryPointer(display, root, &unused_root, &child, &root_x, &root_y,
                     &win_x, &win_y, &mask)) {
    return 0;
  }
  return child;
}

static void notify_click_outside(DesktopClickOutsidePlugin* self) {
  if (self->channel == nullptr) {
    return;
  }
  fl_method_channel_invoke_method(self->channel, kMethodOnClickOutside, nullptr,
                                  nullptr, nullptr, nullptr);
}

static void handle_raw_button_press(DesktopClickOutsidePlugin* self,
                                    Display* display) {
  if (!self->watching ||
      now_ms() - self->watching_since_ms < self->grace_period_ms) {
    return;
  }

  GdkDisplay* gdk_display = gdk_display_get_default();
  gdk_x11_display_error_trap_push(gdk_display);

  gboolean notify = FALSE;
  Window pointer_window = query_window_under_pointer(display);
  if (pointer_window == 0) {
    notify = TRUE;
  } else {
    guint pid = 0;
    Window client_window =
        find_client_window_with_pid(display, pointer_window, &pid);
    if (pid == static_cast<guint>(getpid())) {
      // The main window is outside the popup; other same-process windows are
      // treated as popup windows and ignored.
      notify = client_window == self->main_window;
    } else {
      notify = TRUE;
    }
  }

  gint trap_code = gdk_x11_display_error_trap_pop(gdk_display);
  if (trap_code != Success) {
    g_warning("desktop_click_outside: pointer window query failed (x_error=%d)",
              trap_code);
  }

  if (notify) {
    notify_click_outside(self);
  }
}

static GdkFilterReturn on_x11_root_event(GdkXEvent* xevent, GdkEvent* event,
                                         gpointer data) {
  XEvent* x_event = static_cast<XEvent*>(xevent);
  DesktopClickOutsidePlugin* self = DESKTOP_CLICK_OUTSIDE_PLUGIN(data);
  if (x_event->type != GenericEvent ||
      x_event->xcookie.extension != self->xinput_opcode) {
    return GDK_FILTER_CONTINUE;
  }

  Display* display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
  if (XGetEventData(display, &x_event->xcookie)) {
    if (x_event->xcookie.evtype == XI_RawButtonPress) {
      handle_raw_button_press(self, display);
    }
    XFreeEventData(display, &x_event->xcookie);
  }
  return GDK_FILTER_CONTINUE;
}

static gboolean ensure_xinput_listener(DesktopClickOutsidePlugin* self) {
  if (self->filter_installed) {
    return TRUE;
  }
  if (!is_x11_supported()) {
    return FALSE;
  }

  GdkDisplay* gdk_display = gdk_display_get_default();
  Display* display = GDK_DISPLAY_XDISPLAY(gdk_display);
  int event = 0;
  int error = 0;
  if (!XQueryExtension(display, "XInputExtension", &self->xinput_opcode, &event,
                       &error)) {
    return FALSE;
  }

  self->main_window = get_main_window(self);
  if (self->main_window == 0) {
    return FALSE;
  }

  self->root_window = gdk_get_default_root_window();
  if (self->root_window == nullptr) {
    return FALSE;
  }

  unsigned char mask[XIMaskLen(XI_RawButtonPress)] = {0};
  XISetMask(mask, XI_RawButtonPress);
  XIEventMask event_mask;
  event_mask.deviceid = XIAllMasterDevices;
  event_mask.mask_len = sizeof(mask);
  event_mask.mask = mask;

  gdk_x11_display_error_trap_push(gdk_display);
  Status select_status = XISelectEvents(display, DefaultRootWindow(display), &event_mask, 1);
  XFlush(display);
  gint trap_code = gdk_x11_display_error_trap_pop(gdk_display);
  if (select_status != Success || trap_code != Success) {
    g_warning("desktop_click_outside: XISelectEvents failed (status=%d, x_error=%d)",
              select_status, trap_code);
    return FALSE;
  }

  gdk_window_add_filter(self->root_window, on_x11_root_event, self);
  self->filter_installed = TRUE;
  return TRUE;
}

static void stop_xinput_listener(DesktopClickOutsidePlugin* self) {
  self->watching = FALSE;
  if (self->filter_installed && self->root_window != nullptr) {
    gdk_window_remove_filter(self->root_window, on_x11_root_event, self);
  }
  self->filter_installed = FALSE;
  self->root_window = nullptr;
}

static void start_watching(DesktopClickOutsidePlugin* self, FlValue* args) {
  self->grace_period_ms = grace_period_from_args(args);
  self->watching_since_ms = now_ms();
  self->watching = ensure_xinput_listener(self);
}

static void desktop_click_outside_plugin_handle_method_call(
    DesktopClickOutsidePlugin* self, FlMethodCall* method_call) {
  g_autoptr(FlMethodResponse) response = nullptr;
  const gchar* method = fl_method_call_get_name(method_call);

  if (strcmp(method, kMethodIsSupported) == 0) {
    response = desktop_click_outside_is_supported_response();
  } else if (strcmp(method, kMethodStartWatching) == 0) {
    start_watching(self, fl_method_call_get_args(method_call));
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
  } else if (strcmp(method, kMethodStopWatching) == 0) {
    stop_xinput_listener(self);
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
  } else {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }

  fl_method_call_respond(method_call, response, nullptr);
}

static void desktop_click_outside_plugin_dispose(GObject* object) {
  DesktopClickOutsidePlugin* self = DESKTOP_CLICK_OUTSIDE_PLUGIN(object);
  stop_xinput_listener(self);
  if (self->channel != nullptr) {
    g_object_unref(self->channel);
    self->channel = nullptr;
  }
  if (self->registrar != nullptr) {
    g_object_unref(self->registrar);
    self->registrar = nullptr;
  }
  G_OBJECT_CLASS(desktop_click_outside_plugin_parent_class)->dispose(object);
}

static void desktop_click_outside_plugin_class_init(
    DesktopClickOutsidePluginClass* klass) {
  G_OBJECT_CLASS(klass)->dispose = desktop_click_outside_plugin_dispose;
}

static void desktop_click_outside_plugin_init(DesktopClickOutsidePlugin* self) {
  self->grace_period_ms = kDefaultGracePeriodMs;
}

static void method_call_cb(FlMethodChannel* channel, FlMethodCall* method_call,
                           gpointer user_data) {
  DesktopClickOutsidePlugin* plugin = DESKTOP_CLICK_OUTSIDE_PLUGIN(user_data);
  desktop_click_outside_plugin_handle_method_call(plugin, method_call);
}

void desktop_click_outside_plugin_register_with_registrar(
    FlPluginRegistrar* registrar) {
  DesktopClickOutsidePlugin* plugin = DESKTOP_CLICK_OUTSIDE_PLUGIN(
      g_object_new(desktop_click_outside_plugin_get_type(), nullptr));
  plugin->registrar = FL_PLUGIN_REGISTRAR(g_object_ref(registrar));

  g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
  g_autoptr(FlMethodChannel) channel =
      fl_method_channel_new(fl_plugin_registrar_get_messenger(registrar),
                            kChannelName, FL_METHOD_CODEC(codec));
  plugin->channel = FL_METHOD_CHANNEL(g_object_ref(channel));
  fl_method_channel_set_method_call_handler(channel, method_call_cb,
                                            g_object_ref(plugin),
                                            g_object_unref);

  g_object_unref(plugin);
}
