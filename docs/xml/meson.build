ent_conf = configuration_data()
ent_conf.set('PACKAGE', 'IrcClient')
ent_conf.set('PACKAGE_BUGREPORT', 'https://github.com/TingPing/irc-client/issues')
ent_conf.set('PACKAGE_NAME', 'irc-client')
ent_conf.set('PACKAGE_STRING', 'irc-client')
ent_conf.set('PACKAGE_TARNAME', 'irc-client-' + meson.project_version())
ent_conf.set('PACKAGE_URL', 'https://github.com/TingPing/irc-client')
ent_conf.set('PACKAGE_VERSION', meson.project_version())
ent_conf.set('PACKAGE_API_VERSION', libirc_api_version)

configure_file(
  input: 'gtkdocentities.ent.in',
  output: 'gtkdocentities.ent',
  configuration: ent_conf
)
