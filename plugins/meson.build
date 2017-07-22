plugin_dir = join_paths(get_option('libdir'), 'irc-client')

install_subdir('python_console', install_dir: plugin_dir)

subdir('screensaver')

if get_option('with-identd')
  subdir('identd')
endif

