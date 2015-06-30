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
#include "irc-server.h"

static void
on_server_connected(IrcServer *serv, gpointer data)
{
  g_print("yes\n");
}

static void
on_server_line(IrcServer *serv, const char *line, gpointer data)
{
  g_print("%s\n", line);
}

static void
test_server (void)
{
	IrcServer *serv;

	GMainLoop *main_loop = g_main_loop_new (NULL, FALSE);

	serv = irc_server_new("chat.freenode.net");
	g_signal_connect (serv, "connected", G_CALLBACK(on_server_connected), NULL);
  	g_signal_connect (serv, "raw-line", G_CALLBACK(on_server_line), NULL);
	irc_server_connect (serv);

	g_main_loop_run (main_loop);
}

int
main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/irc/server", test_server);

	return g_test_run ();
}
