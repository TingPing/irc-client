project('irc-client', 'c',
  version: '0.0.1',
  license: 'GPL3+',
  meson_version: '>= 0.37.0',
  default_options: [
    'c_std=gnu99',
  ]
)

libirc_api_version = '0.1'

libgio_dep = dependency('gio-2.0', version: '>= 2.50')
libgtk_dep = dependency('gtk+-3.0', version: '>= 3.22')
libpeas_dep = dependency('libpeas-gtk-1.0', version: '>= 1.14.0')
libnotify_dep = dependency('libnotify')
libsexy_dep = dependency('libsexy3', version: '>= 1.1.0')

config_h = configuration_data()
config_h.set_quoted('PACKAGE_NAME', meson.project_name())
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', meson.project_name())
config_h.set10('ENABLE_NLS', true)
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'),
                                 get_option('datadir'), 'locale'))
config_h.set('GLIB_VERSION_MIN_REQUIRED', 'GLIB_VERSION_2_50')
config_h.set('GLIB_VERSION_MAX_ALLOWED', 'GLIB_VERSION_2_50')
if get_option('buildtype') == 'release'
  config_h.set('G_DISABLE_ASSERT', true)
  config_h.set('G_DISABLE_CHECKS', true)
  config_h.set('G_DISABLE_CAST_CHECKS', true)
else
  config_h.set('G_ENABLE_DEBUG', true)
endif
configure_file(
  output: 'config.h',
  configuration: config_h,
)

cc = meson.get_compiler('c')

test_cflags = [
  '-funsigned-char',
  '-fstack-protector-strong',
  '-Wconversion',
  '-Winline',
  '-Wno-padded',
  '-Wno-unused-parameter',
  '-Wno-nonnull-compare',
  '-Wstrict-prototypes',
  '-Wmissing-prototypes',
  '-Werror=implicit-function-declaration',
  '-Werror=pointer-arith',
  '-Werror=init-self',
  '-Werror=format-security',
  #'-Werror=format=2',
  #'-Werror=missing-include-dirs',
  '-Werror=date-time',
]

global_cflags = [
  # For config.h
  '-I' + meson.build_root(),
  '-DHAVE_CONFIG_H',
]
foreach cflag : test_cflags
  if cc.has_argument(cflag)
    global_cflags += cflag
  endif
endforeach
add_project_arguments(global_cflags, language: 'c')

test_ldflags = [
  '-Wl,-z,relro',
  '-Wl,-z,now',
]
global_ldflags = []
foreach ldflag: test_ldflags
  if cc.has_argument(ldflag)
    global_ldflags += ldflag
  endif
endforeach
add_project_link_arguments(global_ldflags, language: 'c')

i18n = import('i18n')
gnome = import('gnome')

subdir('src')
subdir('plugins')
subdir('tests')
subdir('tools')
subdir('po')
subdir('data')
subdir('docs')

meson.add_install_script('meson_post_install.sh')
