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


#include <glib.h>
#include "irc-utils.h"

static void
test_cmp (void)
{
	g_assert_true (irc_str_equal ("AbCdeF{}|", "aBcDEf[]\\"));
}

static void
test_strip (void)
{
	const char *orig = "\002\017\035\037\003,03\026\007\010\00304,04testing\00399";
	g_autofree char *stripped = irc_strip_attributes (orig);
	g_assert_cmpstr (stripped, ==, "testing");

  	g_autofree char *stripped2 = irc_strip_attributes ("\003,,3\003,3,3\0033,,3\0033333");
	g_assert_cmpstr (stripped2, ==, ",,3,3,,333");
}

static void
test_hash (void)
{
	guint h1, h2;

	h1 = irc_str_hash ("AbC{}\\");
	h2 = irc_str_hash ("aBc[]|");

	g_assert_cmpuint (h1, ==, h2);
}

static void
test_strstr (void)
{
	const char *haystack = "afjoaifjo[]|testing";
	char *match;

	match = irc_strcasestr (haystack, "jfpaf");
	g_assert_null (match);

	match = irc_strcasestr (haystack, "afjoaifjo[]|testing2");
	g_assert_null (match);

	match = irc_strcasestr (haystack, "O{}\\t");
	g_assert_nonnull (match);
	g_assert_true (match == haystack + 8);

	match = irc_strcasestr (haystack, "");
	g_assert_nonnull (match);
	g_assert_true (match == haystack);

  	match = irc_strcasestr (haystack, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
	g_assert_null (match);

  	match = irc_strcasestr (haystack, "afjoaifjo[]|testing");
	g_assert_nonnull (match);
	g_assert_true (haystack == match);
}

static void
test_strv (void)
{
	const char *test_str = "abc/def/ghi";
	GStrv split = g_strsplit (test_str, "/", 0);

	g_assert_cmpuint (g_strv_length (split), ==, 3);

	GStrv new = irc_strv_append (split, "jkl");

	g_assert_cmpuint (g_strv_length (new), ==, 4);
	g_assert_cmpstr (new[3], ==, "jkl");
	g_assert_null (new[4]);

	g_clear_pointer (&new, g_strfreev);

	new = irc_strv_append (new, "test");

  	g_assert_cmpuint (g_strv_length (new), ==, 1);
	g_strfreev (new);
}

int
main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/irc/utils/strip", test_strip);
	g_test_add_func ("/irc/utils/cmp", test_cmp);
	g_test_add_func ("/irc/utils/strstr", test_strstr);
	g_test_add_func ("/irc/utils/hash", test_hash);
	g_test_add_func ("/irc/utils/strv", test_strv);

	return g_test_run ();
}
