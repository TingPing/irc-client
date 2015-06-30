# Irc.py
#
# Copyright (C) 2015 Patrick Griffis <tingping@tingping.se>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

from ..module import get_introspection_module
from ..overrides import override

Irc = get_introspection_module('Irc')
__all__ = []

class Message(Irc.Message):
	def __str__(self):
		return str({
			'time': self.timestamp,
			'sender': self.sender,
			'command': self.command,
			'numeric': self.numeric,
			'words': self.words,
			'words_eol': self.words_eol,
		})

Message = override (Message)
__all__.append('Message')
