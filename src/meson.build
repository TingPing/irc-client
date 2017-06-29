conf = configuration_data()
conf.set('name', meson.project_name())
conf.set('version', meson.project_version())
conf.set('prefix', get_option('prefix'))
conf.set('libdir', join_paths(get_option('prefix'), get_option('libdir')))

if get_option('datadir') != 'share'
  error('datadir must be "share"')
endif
if get_option('localedir') != 'share/locale'
  error('localedir must be "share/locale"')
endif

configure_file(
  input: meson.project_name() + '.js.in',
  output: meson.project_name(),
  configuration: conf,
  install: true,
  install_dir: get_option('bindir')
)

irc_src_resources = gnome.compile_resources(meson.project_name() + '.src',
  'se.tingping.IrcClient.src.gresource.xml',
  gresource_bundle: true,
  install: true,
  install_dir: pkgdatadir,
)

irc_client = join_paths(meson.current_build_dir(), meson.project_name())
run_target('run',
  command: [irc_client],
  depends: [
    irc_data_resources,
    irc_src_resources,
    libirc_gir[1],
    irc_settings
  ]
)
