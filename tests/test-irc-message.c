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
#include "irc-message.h"

static void
test_message (void)
{
	IrcMessage *msg;

	msg = irc_message_new (":nick!~ident@hostname QUIT :Read error: Connection reset by peer");

	g_assert_nonnull (msg);
	g_assert_nonnull (msg->params);
	g_assert_cmpstr (msg->sender, ==, "nick!~ident@hostname");
	g_assert_cmpstr (msg->command, ==, "QUIT");
	g_assert_cmpint (msg->numeric, ==, 0);
	g_assert_cmpstr (irc_message_get_param(msg, 0), ==, "Read error: Connection reset by peer");
	g_assert_cmpstr (irc_message_get_param(msg, 1), ==, "");
	g_assert_cmpstr (irc_message_get_word_eol (msg, 2), ==, "Connection reset by peer");
	g_assert_cmpstr (irc_message_get_word_eol (msg, 100), ==, "");
	g_assert_cmpstr (irc_message_get_word_eol (msg, 5), ==, "peer");
	g_assert_cmpstr (irc_message_get_word_eol (msg, 6), ==, "");
	g_assert_cmpstr (irc_message_get_word_eol (msg, 0), ==, ":Read error: Connection reset by peer");

	irc_message_free (msg);

	msg = irc_message_new (":card.freenode.net 318 Nick Nick :End of /WHOIS list.");

	g_assert_nonnull (msg);
	g_assert_cmpstr (msg->sender, ==, "card.freenode.net");
	g_assert_null (msg->command);
	g_assert_nonnull (msg->params);
	g_assert_cmpint (msg->numeric, ==, 318);
	g_assert_cmpstr (irc_message_get_param(msg, 0), ==, "Nick");
	g_assert_cmpstr (irc_message_get_param(msg, 1), ==, "Nick");
	g_assert_cmpstr (irc_message_get_param(msg, 2), ==, "End of /WHOIS list.");
	g_assert_cmpstr (irc_message_get_param(msg, 3), ==, "");


	irc_message_free (msg);

  	msg = irc_message_new ("PING :card.freenode.net");

	g_assert_nonnull (msg);
	g_assert_null (msg->sender);
	g_assert_cmpstr (msg->command, ==, "PING");
	g_assert_cmpint (msg->numeric, ==, 0);
	g_assert_cmpstr (irc_message_get_param(msg, 0), ==, "card.freenode.net");
	g_assert_nonnull (msg->params);

  	irc_message_free (msg);

  	msg = irc_message_new (":nick!ident@host AWAY");

	g_assert_nonnull (msg);
	g_assert_nonnull (msg->sender);
	g_assert_cmpstr (msg->command, ==, "AWAY");
	g_assert_nonnull (msg->params);
	g_assert_cmpstr (irc_message_get_param(msg, 0), ==, "");

  	irc_message_free (msg);

	msg = irc_message_new ("@time=2015-06-16T19:02:58.651Z;te-st1=\\:\\s\\\\\\ttest;thing;something=; PING");

	g_assert_cmpstr (irc_message_get_tag_value (msg, "te-st1"), ==, "; \\ttest");
	g_assert_cmpint (msg->timestamp, ==, 1434481378);
	g_assert_null (irc_message_get_tag_value (msg, "junk"));
	g_assert_null (irc_message_get_tag_value (msg, "something"));
	g_assert_true (irc_message_has_tag (msg, "thing"));
	g_assert_false (irc_message_has_tag (msg, "junk"));

	irc_message_free (msg);
}

static void
test_message_performance (void)
{
	g_test_timer_start ();

	for (gsize i = 0; i < 1000000; ++i)
	{
		IrcMessage *msg = irc_message_new ("@time=2015-06-16T19:02:58.651Z :TingPing!~tingping@fedora/pdpc.professional.tingping PRIVMSG ##tingpingtest :test");
		g_test_queue_destroy ((GDestroyNotify)irc_message_free, msg);
	}

	double elapsed = g_test_timer_elapsed ();
	g_debug ("Took %f seconds", elapsed);
}

int
main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/irc/message", test_message);
	g_test_add_func ("/irc/message_performance", test_message_performance);

	return g_test_run ();
}
