local lgi = require('lgi')
local Gtk = lgi.require('Gtk')
local IrcClient = lgi.require('IrcClient')
local IrcGui = lgi.IrcGui

local Window = IrcGui:class('Window', Gtk.ApplicationWindow)

function Window:do_constructed()
  self:add(Gtk.Box {
    orientation=Gtk.Orientation.VERTICAL,
    Gtk.Paned {
      vexpand=true,
      IrcClient.Contextview {},
      IrcClient.Chatview {},
    },
    Gtk.Frame {
      Gtk.Entry {}
    }
  })
  self:show_all()
end
