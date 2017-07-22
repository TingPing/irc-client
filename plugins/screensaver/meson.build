screensaver_plugindir = join_paths(plugin_dir, 'screensaver')

screensaver_sources = [
  'screensaver-plugin.c',
]

screensaver_sources += gnome.gdbus_codegen(
  'screensaver-gnome', 'org.gnome.ScreenSaver.xml',
  namespace: 'ScreensaverGnome',
)

screensaver_sources += gnome.gdbus_codegen(
  'screensaver-fdo', 'org.freedesktop.ScreenSaver.xml',
  namespace: 'ScreensaverFdo',
)

shared_module('screensaver', screensaver_sources,
  c_args: ['-include', 'irc-config.h'],
  dependencies: [
    libirc_dep,
    libpeas_dep,
  ],
  install: true,
  install_dir: screensaver_plugindir
)

install_data('screensaver.plugin', install_dir: screensaver_plugindir)
