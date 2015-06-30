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

- autoconf
- automake
- libtool
- intltool
- gtk-doc
- vala
- python-gobject
- appstream-glib
- desktop-file-utils

### Runtime deps

- gtk3
- glib-networking
- libnotify
- [libsexy3](https://github.com/TingPing/libsexy3)
- libpeas

```sh
./autogen.sh
make -s
sudo make install
```
