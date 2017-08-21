client_sources = [
  'irc-application.c',
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
]

client_cflags = [
  '-include', 'irc-config.h'
]

executable('irc-client', client_resources + client_sources,
  dependencies: client_deps,
  c_args: client_cflags,
  include_directories: include_directories(['preferences', 'contextview']),
  install: true,
)
