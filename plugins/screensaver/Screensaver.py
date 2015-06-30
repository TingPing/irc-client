import gi
gi.require_version ('Irc', '0.1')
gi.require_version ('Peas', '1.0')
from gi.repository import GObject, GLib, Gio, Peas, Irc

screensavers = (
	('org.gnome.ScreenSaver', '/org/gnome/ScreenSaver'),
	('org.freedesktop.ScreenSaver', '/org/freedesktop/ScreenSaver'),
)

class Plugin(GObject.Object, Peas.Activatable):

	object = GObject.Property(type=GObject.Object)


	def on_screensaver_active_changed(self, conn, sender, path,
                                   interface, sig, param, data=None):
		active = param.get_child_value(0).get_boolean()
		mgr = Irc.ContextManager.get_default()
		front = mgr.get_front_context()
		if not front:
			return
		if active:
			front.run_command('allserv AWAY :Auto-away')
		else:
			front.run_command('allserv AWAY')

	def on_got_bus(self, obj, result, data=None):
		self.cancel = None
		try:
			self.bus = Gio.bus_get_finish (result)
		except GLib.GError as e:
			print (e)
			return

		for screensaver in screensavers:
			self.subs.append(self.bus.signal_subscribe (None, screensaver[0],
							'ActiveChanged', screensaver[1],
							None, Gio.DBusSignalFlags.NONE,
							self.on_screensaver_active_changed, None))

	def do_activate(self):
		self.subs = []
		self.cancel = Gio.Cancellable()
		Gio.bus_get (Gio.BusType.SESSION, self.cancel, self.on_got_bus)

	def do_deactivate(self):
		if self.cancel:
			self.cancel.cancel()
		for sub in self.subs: # Is this cleanup needed?
			self.bus.signal_unsubscribe (sub)
		self.bus = None

