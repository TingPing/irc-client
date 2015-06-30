
ACLOCAL_AMFLAGS = -I m4
DISTCHECK_CONFIGURE_FLAGS = --enable-introspection --enable-gtk-doc --enable-vala
SUBDIRS = po docs

# ------------ CLIENT --------------------

pkglib_LTLIBRARIES = libirc.la
libirc_la_SOURCES = \
	src/irc-context-action.c \
	src/irc-context-manager.c \
	src/irc-context.c \
	src/irc-command.c \
	src/irc-channel.c \
	src/irc-message.c \
	src/irc-server.c \
	src/irc-query.c \
	src/irc-user.c \
	src/irc-utils.c \
	src/gui/irc-application.c \
	src/gui/irc-window.c \
	src/gui/irc-userlist.c \
	src/gui/textview/irc-textview.c \
	src/gui/contextview/irc-chanstore.c \
	src/gui/contextview/irc-contextview.c \
	src/gui/textview/irc-chatview.c \
	src/gui/textview/irc-colorscheme.c \
	src/gui/entry/irc-entry.c \
	src/gui/entry/irc-entrybuffer.c \
	src/gui/contextview/irc-cellrenderer-bubble.c \
	src/gui/preferences/irc-preferences-window.c \
	src/gui/preferences/irc-network-properties.c
pkginclude_HEADERS = \
	src/irc.h \
	src/irc-context-manager.h \
	src/irc-context-action.h \
	src/irc-context.h \
	src/irc-channel.h \
	src/irc-message.h \
	src/irc-server.h \
	src/irc-query.h \
	src/irc-user.h \
	src/irc-utils.h \
	src/gui/irc-application.h \
	src/gui/textview/irc-chatview.h \
	src/gui/contextview/irc-contextview.h \
	src/gui/textview/irc-textview.h \
	src/gui/entry/irc-entrybuffer.h \
	src/gui/entry/irc-entry.h \
	src/gui/irc-userlist.h \
	src/gui/irc-window.h

noinst_HEADERS = \
	src/irc-private.h \
	src/gui/contextview/irc-cellrenderer-bubble.h \
	src/gui/contextview/irc-chanstore.h \
	src/gui/textview/irc-chatview.h \
	src/gui/textview/irc-colorscheme.h \
	src/gui/preferences/irc-preferences-window.h \
	src/gui/preferences/irc-network-properties.h

nodist_libirc_la_SOURCES = \
	src/irc-enumtypes.c \
	src/irc-marshal.c \
	src/gui/irc-resources.c
nodist_pkginclude_HEADERS = \
	src/irc-enumtypes.h \
	src/irc-marshal.h \
	src/gui/irc-resources.h

libirc_la_LIBADD = $(IRC_LIBS)
libirc_la_LDFLAGS = $(IRC_LDFLAGS) \
	-avoid-version \
	-export-symbols-regex '^irc_.*'
libirc_la_CFLAGS = $(IRC_CFLAGS) \
	-DG_LOG_DOMAIN=\"Irc\" -DLOCALEDIR=\"$(localedir)\" -DLIBDIR=\"$(libdir)\" \
	-Isrc -Isrc/gui -Isrc/gui/preferences -Isrc/gui/textview -Isrc/gui/entry \
	-Isrc/gui/contextview

bin_PROGRAMS = irc-client
irc_client_CFLAGS = $(libirc_la_CFLAGS)
irc_client_LDFLAGS = $(libirc_la_LDFLAGS)
irc_client_LDADD = $(libirc_la_LIBADD) libirc.la
irc_client_SOURCES = src/gui/main.c

src/irc-marshal.h: src/irc-marshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=irc_marshal --header $< > $@

src/irc-marshal.c: src/irc-marshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=irc_marshal --body $< > $@

src/irc-enumtypes.h: src/irc-enumtypes.h.template $(pkginclude_HEADERS)
	$(AM_V_GEN) $(GLIB_MKENUMS) --template $< $(filter-out %.template,$^) > $@

src/irc-enumtypes.c: src/irc-enumtypes.c.template $(pkginclude_HEADERS)
	$(AM_V_GEN) $(GLIB_MKENUMS) --template $< $(filter-out %.template,$^) > $@

src/gui/irc-resources.h: data/se.tingping.IrcClient.gresource.xml src/gui/irc-resources.c
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=data --generate-header --c-name irc $<

src/gui/irc-resources.c: data/se.tingping.IrcClient.gresource.xml $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=data --generate-dependencies data/se.tingping.IrcClient.gresource.xml)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=data --generate-source --c-name irc $<

desktop_in_files = data/se.tingping.IrcClient.desktop.in
desktop_FILES = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
@DESKTOP_FILE_RULES@

appdata_in_files = data/se.tingping.IrcClient.appdata.xml.in
appdata_XML = $(appdata_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@
@APPSTREAM_XML_RULES@

gsettings_SCHEMAS = data/se.tingping.IrcClient.gschema.xml
@GSETTINGS_RULES@

EXTRA_DIST = $(desktop_in_files) $(appdata_in_files) \
	data/se.tingping.IrcClient.gschema.xml.in \
	src/irc-enumtypes.c.template src/irc-enumtypes.h.template \
	src/irc-marshal.list \
	README.md autogen.sh .editorconfig
BUILT_SOURCES = $(nodist_libirc_la_SOURCES) $(nodist_pkginclude_HEADERS)
CLEANFILES = $(BUILT_SOURCES)

run: irc-client
	$(TESTS_ENVIRONMENT) $(LIBTOOL) --mode=execute gdb ./$< -ex r

# ----------- PyGi Override ----------

# FIXME: This should be handled in configure.ac
overridedir = `python3 -c "import gi; print(gi._overridesdir)"`
override_DATA = src/Irc.py

# ----------- PLUGINS ----------------

plugindir = $(pkglibdir)
consoleplugindir = $(plugindir)/python_console
consoleplugin_DATA = plugins/python_console/PythonConsole.py plugins/python_console/python_console.plugin

screensaverplugindir = $(plugindir)/screensaver
screensaverplugin_DATA = plugins/screensaver/Screensaver.py plugins/screensaver/screensaver.plugin

# ------------ INTROSPECTION -------------

include $(INTROSPECTION_MAKEFILE)
if HAVE_INTROSPECTION
Irc-0.1.gir: libirc.la
Irc_0_1_gir_CFLAGS = $(libirc_la_CFLAGS)
Irc_0_1_gir_INCLUDES = Gtk-3.0 Gio-2.0 Sexy-3.0
Irc_0_1_gir_LIBS = libirc.la
Irc_0_1_gir_FILES = $(libirc_la_SOURCES) $(pkginclude_HEADERS)
Irc_0_1_gir_SCANNERFLAGS = --warn-all --c-include="irc.h"
INTROSPECTION_GIRS = Irc-0.1.gir

girdir = $(INTROSPECTION_GIRDIR)
gir_DATA = $(INTROSPECTION_GIRS)

typelibsdir = $(INTROSPECTION_TYPELIBDIR)
typelibs_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelibs_DATA)
endif

# ------------- VAPI -------------------

include $(VAPIGEN_MAKEFILE)
if ENABLE_VAPIGEN
libirc.vapi: Irc-0.1.gir libirc.deps
libirc_vapi_DEPS = gtk+-3.0 gio-2.0 libsexy3
libirc_vapi_FILES = Irc-0.1.gir
VAPIGEN_VAPIS = libirc.vapi

vapidir = $(VAPIGEN_VAPIDIR)
vapi_DATA = $(VAPIGEN_VAPIS) $(VAPIGEN_VAPIS:.vapi=.deps)

libirc.deps: Makefile.am
	$(AM_V_GEN) for pkg in $(libirc_vapi_DEPS); do \
		echo $$pkg >> $@; \
	done

CLEANFILES += $(vapi_DATA)
BUILT_SOURCES += libirc.deps
endif

# ------------- TOOLS --------------------

noinst_PROGRAMS = str-hash
str_hash_CFLAGS = $(libirc_la_CFLAGS)
str_hash_LDFLAGS = $(IRC_LDFLAGS)
str_hash_LDADD = $(IRC_LIBS) libirc.la
str_hash_SOURCES = tools/str-hash.c

# ---------------- TESTS -----------------

TESTS_ENVIRONMENT= \
	GOBJECT_DEBUG=instance-count \
	G_TEST_SRCDIR="$(abs_srcdir)" \
	G_TEST_BUILDDIR="$(abs_builddir)" \
	G_DEBUG=gc-friendly \
	MALLOC_CHECK_=2 \
	MALLOC_PERTURB_=$$(($${RANDOM:-256} % 256)) \
	LSAN_OPTIONS="suppressions=m4/leak-supp.txt" \
	G_MESSAGES_DEBUG=Irc \
	IRC_PLUGIN_DIR=plugins

LOG_COMPILER = m4/tap-test

test_libs = $(irc_client_LDADD)
test_cflags = $(irc_client_CFLAGS)

TESTS = tests/test-irc-message
tests_test_irc_message_CFLAGS = $(test_cflags)
tests_test_irc_message_LDADD = $(test_libs)

TESTS += tests/test-irc-utils
tests_test_irc_utils_CFLAGS = $(test_cflags)
tests_test_irc_utils_LDADD = $(test_libs)

noinst_PROGRAMS += tests/test-irc-textview
tests_test_irc_textview_SOURCES = tests/test-irc-textview.c
tests_test_irc_textview_CFLAGS = $(test_cflags)
tests_test_irc_textview_LDADD = $(test_libs)

#TESTS += tests/test-irc-server
#tests_test_irc_server_CFLAGS = $(test_cflags)
#tests_test_irc_server_LDADD = $(test_libs)

check_PROGRAMS = $(TESTS)

