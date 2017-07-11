config_h = configuration_data()
config_h.set_quoted('PACKAGE_NAME', meson.project_name())
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', meson.project_name())
config_h.set_quoted('G_LOG_DOMAIN', 'Irc')
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
config_h.set_quoted('LIBDIR', join_paths(get_option('prefix'), get_option('libdir')))
config_h.set10('ENABLE_NLS', true)
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'),
                                 get_option('datadir'), 'locale'))
config_h.set('GLIB_VERSION_MIN_REQUIRED', 'GLIB_VERSION_2_52')
config_h.set('GLIB_VERSION_MAX_ALLOWED', 'GLIB_VERSION_2_52')
config_h.set('GDK_VERSION_MIN_REQUIRED', 'GDK_VERSION_3_22')
config_h.set('GDK_VERSION_MAX_ALLOWED', 'GDK_VERSION_3_22')
config_h.set('G_LOG_USE_STRUCTURED', true)
if get_option('buildtype') == 'release'
  config_h.set('G_DISABLE_ASSERT', true)
  config_h.set('G_DISABLE_CHECKS', true)
  config_h.set('G_DISABLE_CAST_CHECKS', true)
elif get_option('buildtype').startswith('debug')
  config_h.set('G_ENABLE_DEBUG', true)
endif
configure_file(
  output: 'irc-config.h',
  configuration: config_h,
)

libirc_sources = [
  'irc-context-action.c',
  'irc-context-manager.c',
  'irc-context.c',
  'irc-command.c',
  'irc-channel.c',
  'irc-message.c',
  'irc-server.c',
  'irc-query.c',
  'irc-user.c',
  'irc-user-list.c',
  'irc-user-list-item.c',
  'irc-utils.c',
  'gui/irc-application.c',
  'gui/irc-chatview.c',
  'gui/irc-colorscheme.c',
  'gui/irc-entry.c',
  'gui/irc-entrybuffer.c',
  'gui/irc-textview.c',
  'gui/irc-window.c',
  'gui/irc-userlist.c',
  'gui/contextview/irc-chanstore.c',
  'gui/contextview/irc-contextview.c',
  'gui/contextview/irc-cellrenderer-bubble.c',
  'gui/preferences/irc-preferences-window.c',
  'gui/preferences/irc-network-properties.c',
]

libirc_public_headers = [
  'irc.h',
  'irc-context-manager.h',
  'irc-context-action.h',
  'irc-context.h',
  'irc-channel.h',
  'irc-message.h',
  'irc-server.h',
  'irc-query.h',
  'irc-user.h',
  'irc-user-list.h',
  'irc-user-list-item.h',
  'irc-utils.h',
  'gui/irc-application.h',
  'gui/irc-chatview.h',
  'gui/irc-entry.h',
  'gui/irc-entrybuffer.h',
  'gui/contextview/irc-contextview.h',
  'gui/irc-textview.h',
  'gui/irc-userlist.h',
  'gui/irc-window.h',
]

libirc_private_headers = [
  'irc-private.h',
  'gui/irc-chatview.h',
  'gui/irc-colorscheme.h',
  'gui/contextview/irc-cellrenderer-bubble.h',
  'gui/contextview/irc-chanstore.h',
  'gui/preferences/irc-preferences-window.h',
  'gui/preferences/irc-network-properties.h',
]

install_headers(libirc_public_headers, subdir: 'irc-client')

libirc_deps = [
  libgio_dep,
  libgtk_dep,
  libpeas_dep,
  libnotify_dep,
  libgspell_dep,
  libgtksource_dep,
]

pkgincludedir = join_paths(get_option('includedir'), 'irc-client')

libirc_marshal = gnome.genmarshal('irc-marshal',
  sources: 'irc-marshal.list',
  prefix: 'irc_marshal',
  install_header: true,
  install_dir: pkgincludedir,
)

libirc_enums = gnome.mkenums('irc-enumtypes',
  sources: libirc_public_headers,
  c_template: 'irc-enumtypes.c.template',
  h_template: 'irc-enumtypes.h.template',
  install_header: true,
  install_dir: pkgincludedir,
)

libirc_resources = gnome.compile_resources('irc-resources',
  '../data/se.tingping.IrcClient.gresource.xml',
  source_dir: '../data',
  c_name: 'irc',
)

libirc_gen_headers = [
  libirc_marshal[1],
  libirc_enums[1],
  libirc_resources[1],
]

libirc_gen_sources = [
  libirc_marshal[0],
  libirc_enums[0],
  libirc_resources[0],
]

libirc_cflags = [
  '-include', join_paths(meson.current_build_dir(), 'irc-config.h')
]

libirc_link_flags = [
  '-Wl,--version-script,' + join_paths(meson.current_source_dir(), 'irc.map'),
]

libirc_includes = [
  '.', 'gui',
  'gui/contextview',
  'gui/preferences',
]

libirc = shared_library('irc',
  libirc_gen_headers + libirc_gen_sources + libirc_sources,
  c_args: libirc_cflags,
  link_args: libirc_link_flags,
  link_depends: 'irc.map',
  dependencies: libirc_deps,
  pic: true,
  include_directories: include_directories(libirc_includes),
  install: true,
)

libirc_dep = declare_dependency(
  sources: libirc_gen_headers,
  dependencies: [libgio_dep, libgtk_dep, libgtksource_dep],
  link_with: libirc,
  include_directories: include_directories(libirc_includes),
)

libirc_gir = gnome.generate_gir(
  libirc,
  sources: libirc_sources + libirc_public_headers,
  nsversion: libirc_api_version,
  namespace: 'Irc',
  symbol_prefix: 'irc',
  identifier_prefix: 'Irc',
  install: true,
  includes: [
    'Gtk-3.0',
    'GtkSource-3.0',
    'Gio-2.0'
  ],
  extra_args: [
    '--c-include=irc.h',
  ],
)

if get_option('with-vapi')

  gnome.generate_vapi('libirc-' + libirc_api_version,
    sources: libirc_gir[0],
    packages: [
      'gio-2.0',
      'gtk+-3.0',
      'gtksourceview-3.0',
    ],
    install: true
  )

endif

executable('irc-client',
  'gui/main.c',
  dependencies: libirc_dep,
  install: true,
)

# We want to find the subdirectory to install our override into:
python3 = find_program('python3')

get_overridedir = '''
import os
import sysconfig

libdir = sysconfig.get_config_var('LIBDIR')
if not libdir:
  libdir = '/usr/lib'

try:
  import gi
  overridedir = gi._overridesdir
except ImportError:
  purelibdir = sysconfig.get_path('purelib')
  overridedir = os.path.join(purelibdir, 'gi', 'overrides')

if overridedir.startswith(libdir): # Should always be True..
  overridedir = overridedir[len(libdir) + 1:]

print(overridedir)
'''

ret = run_command([python3, '-c', get_overridedir])
if ret.returncode() != 0
  error('Failed to determine pygobject overridedir')
else
  pygobject_override_dir = join_paths(get_option('libdir'), ret.stdout().strip())
endif

install_data('Irc.py', install_dir: pygobject_override_dir)
