import sys
import gi
gi.require_version ('Irc', '0.1')
gi.require_version ('Peas', '1.0')
from gi.repository import GObject, Peas, Irc

class ContextStdout:
	def __init__(self, ctx):
		self.ctx = ctx

	def write(self, string):
		if string:
			self.ctx.emit('print', string.rstrip('\n'), 0)

class ConsoleContext(GObject.Object, Irc.Context):
	parent = GObject.Property(type=Irc.Context, default=None)
	name = GObject.Property(type=str, default=">>Python<<")
	active = GObject.Property(type=bool, default=True)

	def __init__(self):
		GObject.Object.__init__(self)
		self.stdout = ContextStdout(self)
		sys.stdout = self.stdout
		sys.stderr = self.stdout
		self.shared_locals = {}
		self.connect('command', self.on_command)

	def do_get_name(self):
		return ">>Python<<"

	def do_get_parent(self):
		return None

	def on_command(self, obj, words, words_eol, data=None):
		exec(words_eol[1], globals(), self.shared_locals)
		return True

class Plugin(GObject.Object, Peas.Activatable):

	object = GObject.Property(type=GObject.Object)

	def do_activate(self):
		self.mgr = Irc.ContextManager.get_default()
		self.console = ConsoleContext ()
		self.mgr.add (self.console)

	def do_deactivate(self):
		self.mgr.remove (self.console)
		self.console = None
