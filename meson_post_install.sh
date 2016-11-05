#!/bin/sh

if [ -z $DESTDIR ]; then

	echo 'Compiling GSchema'
	glib-compile-schemas "$MESON_INSTALL_PREFIX/share/glib-2.0/schemas"
	echo 'Updating icon cache'
	gtk-update-icon-cache -qtf "$MESON_INSTALL_PREFIX/share/icons/hicolor"
	echo 'Updating desktop database'
	update-desktop-database -q "$MESON_INSTALL_PREFIX/share/applications"

fi
