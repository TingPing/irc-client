if not get_option('lib-only')
  i18n.merge_file(
    input: 'se.tingping.IrcClient.desktop.in',
    output: 'se.tingping.IrcClient.desktop',
    type: 'desktop',
    po_dir: '../po',
    install: true,
    install_dir: join_paths(get_option('datadir'), 'applications')
  )

  i18n.merge_file(
    input: 'se.tingping.IrcClient.appdata.xml.in',
    output: 'se.tingping.IrcClient.appdata.xml',
    po_dir: '../po',
    install: true,
    install_dir: join_paths(get_option('datadir'), 'appdata')
  )
endif

install_data('se.tingping.IrcClient.gschema.xml',
  install_dir: join_paths(get_option('datadir'), 'glib-2.0/schemas')
)
