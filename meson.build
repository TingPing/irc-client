project('irc-client', 'c',
  version: '0.0.1',
  license: 'GPL3+',
  meson_version: '>= 0.46.0',
  default_options: [
    'c_std=gnu99',
    'warning_level=2',
    'buildtype=debugoptimized'
  ]
)

libirc_api_version = '0.1'

cc = meson.get_compiler('c')

global_cflags = cc.get_supported_arguments ([
  '-funsigned-char',
  '-Wconversion',
  '-Winline',
  '-Wno-padded',
  '-Wno-unused-parameter',
  '-Wno-nonnull-compare',
  '-Wno-cast-function-type', # Silences warning in G_DEFINE_DYNAMIC_TYPE_EXTENDED()
  '-Wstrict-prototypes',
  '-Wmissing-prototypes',
  '-Werror=implicit-function-declaration',
  '-Werror=pointer-arith',
  '-Werror=init-self',
  ['-Werror=format-security','-Werror=format=1'],
  '-Werror=date-time',
  '-Wmaybe-uninitialized',
])
if get_option('buildtype') != 'plain'
  if cc.has_argument('-fstack-protector-strong') and cc.links('''
     int main (void) {
       char buffer[16];
       strcpy(buffer, "foo");
       return 0;
     }
     ''', args: '-fstack-protector-all')
    global_cflags += '-fstack-protector-strong'
  endif
endif
add_project_arguments(global_cflags, language: 'c')

if get_option('buildtype') != 'plain'
  global_ldflags = cc.get_supported_link_arguments ([
    '-Wl,-z,relro',
    '-Wl,-z,now',
  ])
  add_project_link_arguments(global_ldflags, language: 'c')
endif

i18n = import('i18n')
gnome = import('gnome')

libgio_dep = dependency('gio-2.0', version: '>= 2.56')
libnotify_dep = dependency('libnotify')
if not get_option('lib-only')
  libgtk_dep = dependency('gtk+-3.0', version: '>= 3.24')
  libpeas_dep = dependency('libpeas-gtk-1.0', version: '>= 1.14.0')
  libgspell_dep = dependency('gspell-1', version: '>= 1.3.0')
  libgtksource_dep = dependency('gtksourceview-3.0')
endif

subdir('lib')
if not get_option('lib-only')
  subdir('src')
  subdir('plugins')
endif
subdir('tests')
subdir('tools')
subdir('po')
subdir('data')
subdir('docs')

meson.add_install_script('meson_post_install.sh', '@0@'.format(get_option('lib-only')))
