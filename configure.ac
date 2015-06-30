
AC_PREREQ([2.69])
AC_INIT([IrcClient],[0.0.1],[mailto:tingping@tingping.se],[irc],[])
AM_INIT_AUTOMAKE([1.12 tar-pax dist-xz no-dist-gzip subdir-objects no-define foreign -Wall -Wno-portability])
AC_CONFIG_SRCDIR([src/irc-utils.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE([enable])

AX_IS_RELEASE([git-directory])
AX_CHECK_ENABLE_DEBUG([yes], [G_ENABLE_DEBUG], [G_DISABLE_ASSERT G_DISABLE_CHECKS G_DISABLE_CAST_CHECKS])
AX_REQUIRE_DEFINED([APPSTREAM_XML])
AX_REQUIRE_DEFINED([VAPIGEN_CHECK])
AX_REQUIRE_DEFINED([GOBJECT_INTROSPECTION_CHECK])

AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_INSTALL
AM_PROG_AR
PKG_PROG_PKG_CONFIG([0.28])
LT_PREREQ([2.2.0])
LT_INIT

IT_PROG_INTLTOOL([0.50.1])
GETTEXT_PACKAGE="irc-client"
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Gettext package name])

AC_PATH_PROG(GLIB_COMPILE_RESOURCES, glib-compile-resources)
AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums)
AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)
GOBJECT_INTROSPECTION_CHECK([0.9.3])
GTK_DOC_CHECK([1.4])
VAPIGEN_CHECK([0.18])
DESKTOP_FILE
GLIB_GSETTINGS
APPSTREAM_XML

PKG_CHECK_MODULES([IRC], [
	gio-2.0 >= 2.44
	gtk+-3.0 >= 3.16
	libpeas-gtk-1.0 >= 1.14.0
	libsexy3 >= 1.1.0
dnl	libsecret-1
	libnotify
])

AC_DEFINE([GLIB_VERSION_MIN_REQUIRED], [GLIB_VERSION_2_44], [Dont warn using older APIs])
AC_DEFINE([GLIB_VERSION_MAX_ALLOWED], [GLIB_VERSION_2_44], [Prevents using newer APIs])

AS_IF([test "$CC" = "gcc"], [
	AC_PATH_PROG([AR], [gcc-ar])
	AC_PATH_PROG([NM], [gcc-nm])
	AC_PATH_PROG([RANLIB], [gcc-ranlib])
	AC_SUBST([AR])
	AC_SUBST([NM])
	AC_SUBST([RANLIB])
	AS_IF([test "$enable_debug" = "no"], [
		AX_APPEND_COMPILE_FLAGS([-flto])
	])
])

AX_APPEND_COMPILE_FLAGS([ \
	-std=gnu99 \
	-funsigned-char \
	-fstack-protector-strong \
	-fPIE \
	-fPIC \
	-Wall \
	-Wextra \
	-Wconversion \
	-Winline \
	-Wno-padded \
	-Wno-unused-parameter \
	-Wstrict-prototypes \
	-Wmissing-prototypes \
	-Werror=implicit-function-declaration \
	-Werror=pointer-arith \
	-Werror=init-self \
	-Werror=format-security \
	-Werror=format=2 \
	-Werror=missing-include-dirs \
	-Werror=date-time \
])

AX_APPEND_LINK_FLAGS([ \
	-pie \
	-Wl,-z,relro \
	-Wl,-z,now \
])

AS_IF([test "$enable_debug" = "no"], [
	AX_APPEND_COMPILE_FLAGS([-O2])
])

#AS_IF([test "$enable_debug" = "yes"], [
#	AX_APPEND_COMPILE_FLAGS([ \
#		-fsanitize=undefined \
#		-fsanitize=address \
#	])
#])

AC_CONFIG_FILES([
    Makefile
    po/Makefile.in
    docs/Makefile
])

AC_OUTPUT

echo "
    $PACKAGE $VERSION

    prefix ........: $prefix
    debug .........: $enable_debug
    introspection .: $enable_introspection
    vapi ..........: $enable_vala
    docs ..........: $enable_gtk_doc
"
