client_sources = [
  'irc-chatview.c',
  'irc-colorscheme.c',
  'irc-entry.c',
  'irc-entrybuffer.c',
  'irc-text-common.c',
  'irc-textview.c',
  'irc-window.c',
  'irc-userlist.c',
  'contextview/irc-chanstore.c',
  'contextview/irc-contextview.c',
  'contextview/irc-cellrenderer-bubble.c',
  'preferences/irc-preferences-window.c',
  'preferences/irc-network-properties.c',
  'main.c',
]

client_public_headers = [
  'irc-chatview.h',
  'irc-colorscheme.h',
  'irc-entry.h',
  'irc-entrybuffer.h',
  'irc-textview.h',
  'irc-window.h',
  'irc-userlist.h',
  'contextview/irc-chanstore.h',
  'contextview/irc-contextview.h',
  'contextview/irc-cellrenderer-bubble.h',
  'preferences/irc-preferences-window.h',
  'preferences/irc-network-properties.h',
]

client_resources = gnome.compile_resources('irc-resources',
  'irc-resources.gresource.xml',
  source_dir: '../data',
  c_name: 'irc',
)

client_deps = [
  libirc_dep,
  libgtk_dep,
  libpeas_dep,
  libgspell_dep,
  libgtksource_dep,
  dependency('luajit'),
]

client_cflags = [
  '-include', 'irc-config.h'
]

irc_client = executable('irc-client', client_resources + client_sources,
  dependencies: client_deps,
  c_args: client_cflags,
  include_directories: include_directories(['preferences', 'contextview']),
  install: true,
)

gnome.generate_gir(irc_client,
  sources: client_public_headers,
  includes: ['Gtk-3.0', libirc_gir[0]],
  namespace: 'IrcClient',
  identifier_prefix: 'Irc',
  symbol_prefix: 'irc',
  nsversion: '0.1',
  install: true,
)

