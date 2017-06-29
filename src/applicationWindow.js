const Lang = imports.lang;
const GLib = imports.gi.GLib;
const Gio = imports.gi.Gio;
const Gtk = imports.gi.Gtk;


const ApplicationWindow = new Lang.Class({
    Name: 'ApplicationWindow',
    Extends: Gtk.ApplicationWindow,
    Template: 'resource:///se/tingping/IrcClient/ui/ApplicationWindow.ui',

    _init(params) {
        this.parent(params);
    },
});
