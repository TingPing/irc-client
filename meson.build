project('irc-client', 'c',
  version: '0.0.1',
  license: 'GPL3+',
  meson_version: '>= 0.37.0',
  default_options: [
    'c_std=gnu99',
    'warning_level=2',
    'buildtype=debugoptimized'
  ]
)

libirc_api_version = '0.1'

cc = meson.get_compiler('c')

test_cflags = [
  '-funsigned-char',
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
  ['-Werror=format-security','-Werror=format=1'],
  '-Werror=date-time',
]
global_cflags = []
foreach cflag : test_cflags
  if cc.has_multi_arguments(cflag)
    global_cflags += cflag
  endif
endforeach
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
  test_ldflags = [
    '-Wl,-z,relro',
    '-Wl,-z,now',
  ]
  global_ldflags = []
  foreach ldflag: test_ldflags
    if cc.has_argument(ldflag) and cc.links('int main (void) { return 0; }', args: ldflag)
      global_ldflags += ldflag
    endif
  endforeach
  add_project_link_arguments(global_ldflags, language: 'c')
endif

i18n = import('i18n')
gnome = import('gnome')

libgio_dep = dependency('gio-2.0', version: '>= 2.52')
libgtk_dep = dependency('gtk+-3.0', version: '>= 3.22')
libpeas_dep = dependency('libpeas-gtk-1.0', version: '>= 1.14.0')
libnotify_dep = dependency('libnotify')
libgspell_dep = dependency('gspell-1', version: '>= 1.3.0')
libgtksource_dep = dependency('gtksourceview-3.0')

subdir('src')
subdir('plugins')
subdir('tests')
subdir('tools')
subdir('po')
subdir('data')
subdir('docs')

meson.add_install_script('meson_post_install.sh')
