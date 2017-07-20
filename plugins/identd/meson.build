identd_plugindir = join_paths(plugin_dir, 'identd')
shared_module('identd', [
    'identd-plugin.c',
    'identd-service.c'
  ],
  c_args: ['-include', 'irc-config.h'],
  dependencies: [
    libirc_dep,
    libpeas_dep,
    dependency('gupnp-igd-1.0')
  ],
  install: true,
  install_dir: identd_plugindir
)

install_data('identd.plugin', install_dir: identd_plugindir)
