from gi.repository import GLib, Gio
from gi.repository import GUPnPIgd as Igd

class IdentdServer:
	''' Implementation of RFC 1413 '''

	def __init__(self):
		self.users = {} # Dict of local ports to usernames
		self.cancel = Gio.Cancellable()
		self.service = Gio.SocketService()
		self.igd = Igd.SimpleIgd()
		try:
			self.port = self.service.add_any_inet_port (None)
		except GLib.GError as e:
			print(e)
			self.service = None
			return

	def start(self):
		if not self.service:
			return

		self.service.connect('incoming', self.on_incoming)
		self.service.start()
		print('Identd: Listening on port {}'.format(self.port))

	def stop(self):
		self.users = {}
		self.cancel.cancel()
		if self.service:
			self.service.stop()
			self.service = None
		if self.igd:
			self.igd.remove_port('TCP', 113)

	def remove_user(self, data=None):
		port, address = data

		if port in self.users:
			del self.users[port]

		self.igd.remove_port_local('TCP', address, port)

	def add_user(self, user, port, address):
		def on_fail(igd, error, protocol, external_port, local_ip, local_port, desc, data=None):
			print('failed') # FIXME: Also error is invalid here..?
		def on_success(igd, protocol, external_ip, replaces_external_ip, external_port, 
						local_ip, local_port, desc, data=None):
			print('success')

		# FIXME: This can crash...
		self.igd.connect('error-mapping-port', on_fail)
		self.igd.connect('mapped-external-port', on_success)
		self.igd.add_port('TCP', 113, address, self.port, 180, 'Identd plugin for IrcClient')
		print('added')

		self.users[port] = user
		GLib.timeout_add_seconds(180, self.remove_user, (port, address))

	def on_write_ready(self, source, result, data=None):
		try:
			source.write_finish(result)
		except GLib.GError as e:
			print(e)

	def on_read_ready(self, source, result, data=None):
		try:
			line, _len = source.read_line_finish_utf8(result)
		except GLib.GError as e:
			print(e)
			return

		split = line.split(', ', 1)
		remote_port, local_port = int(split[0]), int(split[1])
		user = self.users.get(local_port, None)
		if not user:
			response = '{}, {} : ERROR : INVALID-PORT\r\n'.format(local_port, remote_port)
		else:
			response = '{}, {} : USERID : UNIX : {}\r\n'.format(local_port, remote_port, user)
			del self.users[local_port]

		stream = data.get_output_stream()
		stream.write_async (bytes(response, 'UTF-8'), GLib.PRIORITY_DEFAULT, self.cancel, self.on_write_ready)

	def on_incoming(self, service, connection, source, data=None):
		stream = Gio.DataInputStream.new(connection.get_input_stream())
		stream.set_newline_type(Gio.DataStreamNewlineType.CR_LF)
		stream.read_line_async(GLib.PRIORITY_DEFAULT, self.cancel, self.on_read_ready, connection)

server = IdentdServer()
server.start()

def test():
	server.add_user('Username', 4241, '192.168.1.106')
	return False

GLib.timeout_add_seconds(1, test)
GLib.MainLoop().run()
