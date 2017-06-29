config_h = configuration_data()
config_h.set_quoted('PACKAGE_NAME', meson.project_name())
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', meson.project_name())
config_h.set_quoted('G_LOG_DOMAIN', 'Irc')
config_h.set10('ENABLE_NLS', true)
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
config_h.set_quoted('LIBDIR', join_paths(get_option('prefix'), get_option('libdir')))

config_h.set('GLIB_VERSION_MIN_REQUIRED', 'GLIB_VERSION_2_50')
config_h.set('GLIB_VERSION_MAX_ALLOWED', 'GLIB_VERSION_2_50')

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
]

libirc_private_headers = [
  'irc-private.h',
]

libirc_marshal = gnome.genmarshal('irc-marshal',
  sources: 'irc-marshal.list',
  prefix: 'irc_marshal',
)

libirc_enums = gnome.mkenums('irc-enumtypes',
  sources: libirc_public_headers,
  c_template: 'irc-enumtypes.c.template',
  h_template: 'irc-enumtypes.h.template',
)

libirc_gen_headers = [
  libirc_marshal[1],
  libirc_enums[1],
]

libirc_gen_sources = [
  libirc_marshal[0],
  libirc_enums[0],
]

libirc_cflags = [
  '-include', 'irc-config.h',
]

libirc_link_flags = [
  '-Wl,--version-script,' + join_paths(meson.current_source_dir(), 'irc.map'),
]

libgio_dep = dependency('gio-2.0', version: '>= 2.50')

libirc = shared_library('irc',
  libirc_gen_headers + libirc_gen_sources + libirc_sources,
  c_args: libirc_cflags,
  link_args: libirc_link_flags,
  link_depends: 'irc.map',
  dependencies: libgio_dep,
  pic: true,
  install: true,
  install_dir: pkglibdir,
)

libirc_dep = declare_dependency(
  sources: libirc_gen_headers,
  dependencies: libgio_dep,
  link_with: libirc,
  include_directories: include_directories('.'),
)

libirc_gir = gnome.generate_gir(
  libirc,
  sources: libirc_sources + libirc_public_headers,
  nsversion: libirc_api_version,
  namespace: 'Irc',
  symbol_prefix: 'irc',
  identifier_prefix: 'Irc',
  install: true,
  includes: 'Gio-2.0',
  extra_args: '--c-include=irc.h',
  install_dir_typelib: join_paths(pkglibdir, 'girepository-1.0'),
  install_dir_gir: pkgdatadir,
)
