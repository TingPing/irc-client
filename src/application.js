const Lang = imports.lang;
const GLib = imports.gi.GLib;
const Gio = imports.gi.Gio;
const Gtk = imports.gi.Gtk;

const ApplicationWindow = imports.applicationWindow;

const Application = new Lang.Class({
    Name: 'Application',
    Extends: Gtk.Application,

    _init() {
        this.parent({
            application_id: pkg.name,
            flags: Gio.ApplicationFlags.FLAGS_NONE,
        });

        GLib.set_application_name(_("Irc Client"));
    },

    vfunc_startup() {
        this.parent();

        let action = new Gio.SimpleAction({name: 'quit'});
        action.connect('activate', (param) => this.quit());
        this.add_action(action);
    },

    vfunc_activate() {
        let window = this.active_window;
        if (!window)
            window = new ApplicationWindow.ApplicationWindow({application: this});
        window.present();
    }
});
