
# ------------- DOCS --------------------

DOC_MODULE = irc
DOC_MAIN_SGML_FILE = irc-docs.xml
DOC_SOURCE_DIR = $(top_srcdir)/src

SCAN_OPTIONS = --rebuild-types #--rebuild-sections
MKDB_OPTIONS = --output-format=xml --name-space=irc
FIXXREF_OPTIONS= \
	--extra-dir=$(datadir)/gtk-doc/html/gobject \
	--extra-dir=$(datadir)/gtk-doc/html/glib \
	--extra-dir=$(datadir)/gtk-doc/html/gio \
	--extra-dir=$(datadir)/gtk-doc/html/gtk3 \
	--extra-dir=$(datadir)/gtk-doc/html/atk \
	--extra-dir=$(datadir)/gtk-doc/html/libsexy3

GTKDOC_CFLAGS = $(IRC_CFLAGS)
GTKDOC_LIBS = $(IRC_LIBS) $(top_srcdir)/libirc.la
HFILE_GLOB = $(top_srcdir)/src/*.h $(top_srcdir)/src/gui/*.h
CFILE_GLOB = $(top_srcdir)/src/*.c $(top_srcdir)/src/gui/*.c

HTML_IMAGES=
content_files=
expand_content_files=

if ENABLE_GTK_DOC
TESTS = $(GTKDOC_CHECK)
endif

include $(top_srcdir)/gtk-doc.make
