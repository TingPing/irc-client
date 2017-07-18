if get_option('with-docs')

  subdir('xml')

  docpath = join_paths(get_option('prefix'), get_option('datadir'), 'gtk-doc/html')
  glib_prefix = libgio_dep.get_pkgconfig_variable('prefix')
  glib_docpath = join_paths(glib_prefix, 'share/gtk-doc/html')

  gnome.gtkdoc('libirc',
    main_sgml: 'libirc-docs.xml',
    src_dir: meson.source_root() + '/src',
    gobject_typesfile: 'libirc.types',
    dependencies: libirc_dep,
    scan_args: [
      '--rebuild-types',
      '--rebuild-sections',
      '--ignore-headers=' + ' '.join(libirc_private_headers),
    ],
    fixxref_args: [
      '--html-dir=' + docpath,
      '--extra-dir=' + join_paths(glib_docpath, 'glib'),
      '--extra-dir=' + join_paths(glib_docpath, 'gobject'),
      '--extra-dir=' + join_paths(glib_docpath, 'gtk3'),
      '--extra-dir=' + join_paths(glib_docpath, 'gtksourceview-3.0'),
    ],
    install: true
  )

endif
