# IrcClient

This project is still a work in progress; It doesn't even have a
name yet. Nothing is guaranteed to be complete or correct.

The goals of the project are to be a simple and easy to use
client that also supports modern irc features and is extensible,
somewhat similar to that of [he]xchat with a more modern
interface and codebase.

The todo list is mostly on the UI side and plugin api; The core
irc library is mostly functional though does not handle or expose
everything yet. (Also yes win32 support will come eventually)

## Building

### Build-only deps

- meson
- gtk-doc (optional for developers)
- vala (optional for bindings)
- python-gobject (optional for bindings)

### Runtime deps

- gtk3
- gtksourceview3
- libnotify
- gspell
- libpeas
- glib-networking (optional for TLS)
- gupnp-igd (optional for identd)


```sh
meson build && cd build
ninja
sudo ninja install
```
