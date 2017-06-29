pkg.initFormat();
pkg.initGettext();
pkg.require({
    GLib: '2.0',
	GObject: '2.0',
    Gio: '2.0',
    GdkPixbuf: '2.0',
	Gtk: '3.0',
    Irc: '0.1',
});

const Application = imports.application;

function main(args) {
    let app = new Application.Application();
	return app.run(args);
}
