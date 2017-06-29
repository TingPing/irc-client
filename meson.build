project('se.tingping.IrcClient', 'c',
  version: '0.0.1',
  license: 'GPL3+',
  meson_version: '>= 0.37.0',
  default_options: [
    'c_std=gnu99',
  ]
)

libirc_api_version = '0.1'
pkgdatadir = join_paths(get_option('datadir'), meson.project_name())
pkglibdir = join_paths(get_option('libdir'), meson.project_name())

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

global_cflags = []
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

subdir('data')
subdir('docs')
subdir('lib')
# subdir('plugins')
subdir('po')
subdir('src')
subdir('tests')
subdir('tools')

meson.add_install_script('meson_post_install.sh')
