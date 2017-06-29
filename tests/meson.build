test_env = [
  'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
  'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir()),
  'G_DEBUG=gc-friendly',
  'GSETTINGS_BACKEND=memory',
  'MALLOC_CHECK_=2',
]

test_dependencies = [
  libirc_dep
]

test_irc_message = executable('test-irc-message', 'test-irc-message.c',
  dependencies: test_dependencies
)
test('Test IrcMessage', test_irc_message,
  env: test_env
)

if false
test_irc_server = executable('test-irc-server', 'test-irc-server.c',
  dependencies: test_dependencies
)
test('Test IrcServer', test_irc_server,
  env: test_env
)
endif

test_irc_utils = executable('test-irc-utils', 'test-irc-utils.c',
  dependencies: test_dependencies
)
test('Test Irc utils', test_irc_utils,
  env: test_env
)
