/*
 * Copyright 2015 Patrick Griffis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#pragma once

#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

/**
 * RETURNS_NON_NULL:
 */
#if __has_attribute(returns_nonnull)
#define RETURNS_NON_NULL __attribute__((returns_nonnull))
#else
#define RETURNS_NON_NULL
#endif

/**
 * NON_NULL:
 * @...: Arguement numbers that should be not %NULL
 */
#if __has_attribute(nonnull)
#define NON_NULL(...) __attribute__((nonnull(__VA_ARGS__)))
#else
#define NON_NULL
#endif

/**
 * WARN_UNUSED_RESULT:
 */
#if __has_attribute(warn_unused_result)
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define WARN_UNUSED_RESULT
#endif

#include <glib-object.h>

/**
 * IrcAttribute:
 * @BOLD: Toggles bold
 * @ITALIC: Toggles italic
 * @COLOR: Starts a color
 * @UNDERLINE: Toggles underline
 * @RESET: Toggles all previous attributes
 * @HIDDEN: Toggles invisible
 * @BEEP: Creates a beep
 * @REVERSE: Reverses normal colors
 * @CTCP: Surrounds CTCP messages
 *
 * Characters for IRC attributes.
 */
typedef enum {
	BOLD = '\002',
	ITALIC = '\035',
	COLOR = '\003',
	UNDERLINE = '\037',
	RESET = '\017',
	HIDDEN = '\010',
	BEEP = '\007',
	REVERSE = '\026',
	CTCP = '\001',
} IrcAttribute;

gboolean irc_isattr (guchar c) G_GNUC_CONST;
guint32 irc_str_hash (const char *str) G_GNUC_PURE NON_NULL();
char *irc_strip_attributes (const char *str) G_GNUC_PURE NON_NULL();
int irc_str_cmp (const char *s1, const char *s2) G_GNUC_PURE NON_NULL();
gboolean irc_str_equal (const char *str1, const char *str2) G_GNUC_PURE NON_NULL();
guchar irc_tolower (const guchar c) G_GNUC_CONST;
char *irc_strcasestr (const char *haystack, const char *needle) G_GNUC_PURE NON_NULL();
char *irc_sasl_encode_plain (const char *username, const char *password) NON_NULL();
GStrv irc_strv_append (GStrv array, const char *str) G_GNUC_PURE;
char *irc_convert_invalid_text (const char *text, gssize len, GIConv converter, const char *fallback_char) G_GNUC_PURE NON_NULL();

