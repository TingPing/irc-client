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

#include <string.h>
#include "irc-utils.h"

/**
 * irc_strip_attributes:
 * @str: String to strip
 *
 * Strips all known IRC attributes from string.
 *
 * See Also: #IrcAttribute
 * Returns: Newly allocated string
 */
char *
irc_strip_attributes (const char *str)
{
	gsize len = strlen (str);
	char *stripped = g_malloc (len + 1);
	guint8 rcol = 0, bgcol = 0;
	char *dst = stripped;

	while (len-- > 0)
	{
		if (rcol > 0 && (g_ascii_isdigit (*str) || (*str == ',' && g_ascii_isdigit (str[1]) && !bgcol)))
		{
			if (str[1] != ',')
				rcol--;
			if (*str == ',')
			{
				rcol = 2;
				bgcol = 1;
			}
		}
		else
		{
			rcol = bgcol = 0;
			switch (*str)
			{
			case COLOR:
				rcol = 2;
			case HIDDEN:
			case BEEP:
			case RESET:
			case REVERSE:
			case BOLD:
			case UNDERLINE:
			case ITALIC:
				break;
			default:
				*dst++ = *str;
			}
		}
		str++;
	}
	*dst = '\0';

	return stripped;
}

/**
 * irc_isattr:
 * @c: Character
 *
 * See Also: #IrcAttribute
 * Returns: %TRUE if is attribute otherwise %FALSE
 */
gboolean
irc_isattr (guchar c)
{
	switch (c)
	{
		case COLOR:
		case HIDDEN:
		case BEEP:
		case RESET:
		case REVERSE:
		case BOLD:
		case UNDERLINE:
		case ITALIC:
			return TRUE;
		default:
			return FALSE;
	}
}

static const guchar irc_tolower_table[256] = {
	0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa,
	0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14,
	0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
	0x1e, 0x1f,
	' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')',
	'*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	':', ';', '<', '=', '>', '?',
	'@', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
	'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
	't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~',
	'_',
	'`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
	'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
	't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~',
	0x7f,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
	0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9,
	0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9,
	0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
	0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9,
	0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
	0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

/**
 * irc_tolower:
 * @c: Character
 *
 * Returns lower-case version of ascii character according
 * to IRC RFC: '{', '}', and '|' equal '[', ']', and '\'.
 */
inline guchar
irc_tolower (const guchar c)
{
	return irc_tolower_table[c];
}

/**
 * irc_strcasestr:
 * @haystack: String to look in
 * @needle: String to look for
 *
 * Looks for @needle in @haystack ignoring case according
 * to the IRC RFC and returns pointer to the start of the
 * match. If @needle is empty returns pointer to @haystack.
 * If not found returns %NULL.
 *
 * See also: irc_tolower()
 */
char *
irc_strcasestr (const char *haystack, const char *needle)
{
	const gsize needle_len = strlen (needle);
	const gsize haystack_len = strlen (haystack);

	for (gsize i = 0; haystack_len - i >= needle_len; ++i, ++haystack)
	{
		char *haystack_p = (char*)haystack;
		char *needle_p = (char*)needle;
		gsize matched = 0;

		while (*needle_p && irc_tolower(*haystack_p) == irc_tolower(*needle_p))
		{
			++haystack_p;
			++needle_p;
			++matched;
		}
		if (matched == needle_len)
			return (char*)haystack;
	}
	return NULL;
}

/**
 * irc_str_cmp:
 * @s1: First string
 * @s2: Second string
 *
 * Compares two strings for equality ignoring case
 * according to the IRC RFC.
 *
 * See Also: irc_tolower()
 * Returns: 0 when equal
 */
int
irc_str_cmp (const char *s1, const char *s2)
{
    int c1, c2;

	while (*s1 && *s2)
	{
		c1 = (int)irc_tolower (*s1);
		c2 = (int)irc_tolower (*s2);
		if (c1 != c2)
			return (c1 - c2);
		s1++; s2++;
	}

	return (((int)*s1) - ((int)*s2));
}

/**
 * irc_str_equal:
 * @str1: First string
 * @str2: Second string
 *
 * Compares two strings for equality ignoring case
 * according to the IRC RFC.
 *
 * See Also: irc_tolower()
 * Returns: %TRUE when equal otherwise %FALSE
 */
inline gboolean
irc_str_equal (const char *str1, const char *str2)
{
	return irc_str_cmp (str1, str2) == 0;
}

/**
 * irc_str_hash:
 * @str: String to hash
 *
 * Hashes lowercase version of string
 * according to IRC RFC.
 *
 * See Also: irc_tolower()
 */
guint32
irc_str_hash (const char *str)
{
	const char *p = str;
	guint32 h = irc_tolower_table [(guchar)*p];

	if (h)
	{
		for (p += 1; *p != '\0'; ++p)
			h = (h << 5) - h + irc_tolower_table [(guchar)*p];
	}

	return h;
}

/**
 * irc_sasl_encode_plain:
 * @username: Username of user
 * @password: Password of user
 *
 * Encodes @username and @password according to RFC 4616
 *
 * Returns: Newly allocated string
 */
char *
irc_sasl_encode_plain (const char *username, const char *password)
{
	const gsize authlen = (strlen (username) * 2) + 2 + strlen (password);
	g_autofree char *buffer = g_strdup_printf ("%s%c%s%c%s", username, '\0', username, '\0', password);
	return g_base64_encode ((guchar*)buffer, authlen);
}

/**
 * irc_strv_append:
 * @array: (transfer full) (nullable): Array to append
 * @str: String to append
 *
 * If @array is %NULL new array is allocated.
 * If @str is %NULL nothing is appended.
 *
 * Returns: (transfer full): Newly re-allocated array. Free with g_strfreev()
 */
GStrv
irc_strv_append (GStrv array, const char *str)
{
	if (str == NULL)
		return array;

	const gsize len = array ? g_strv_length (array) : 0;
	GStrv new_array = g_realloc_n (array, len + 2, sizeof(char*));
	new_array[len] = g_strdup (str);
	new_array[len + 1] = NULL;
	return new_array;
}