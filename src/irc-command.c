/* irc-command.c
 *
 * Copyright (C) 2015 Patrick Griffis <tingping@tingping.se>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib/gi18n.h>
#include "irc.h"
#include "irc-private.h"

static IrcServer *
get_contexts_server (IrcContext *ctx)
{
	if (!IRC_IS_CHANNEL(ctx) && !IRC_IS_QUERY(ctx))
		return NULL;

	return IRC_SERVER(irc_context_get_parent (ctx));
}

static gboolean
command_say (IrcContext *ctx, const GStrv words, const GStrv words_eol)
{
	IrcServer *serv = get_contexts_server (ctx);
	if (!serv)
		return FALSE;

	irc_server_write_linef (serv, "PRIVMSG %s :%s", irc_context_get_name (ctx), words_eol[1]);

	g_autofree char *formatted = g_strdup_printf ("\00304%s\00314 %s", "TingPing", words_eol[1]); // TODO: Share event formatting
	irc_context_print (ctx, formatted);
	return TRUE;
}

static gboolean
command_me (IrcContext *ctx, const GStrv words, const GStrv words_eol)
{
	IrcServer *serv = get_contexts_server (ctx);
	if (!serv)
		return FALSE;

	irc_server_write_linef (serv, "PRIVMSG %s :\001ACTION %s\001", irc_context_get_name (ctx), words_eol[1]);

	g_autofree char *formatted = g_strdup_printf ("* \002\00304%s\002\00314 %s", "TingPing", words_eol[1]);
	irc_context_print (ctx, formatted);
	return TRUE;
}

static gboolean
command_part (IrcContext *ctx, const GStrv words, const GStrv words_eol)
{
	if (!IRC_IS_CHANNEL(ctx))
		return FALSE;

	irc_channel_part (IRC_CHANNEL(ctx));
	return TRUE;
}

static void
allserv_foreach (GNode *node, gpointer data)
{
	if (IRC_IS_SERVER(node->data))
		irc_context_run_command (node->data, data);
}

static gboolean
command_allserv (IrcContext *ctx, const GStrv words, const GStrv words_eol)
{
	IrcContextManager *mgr = irc_context_manager_get_default();
	irc_context_manager_foreach_parent (mgr, allserv_foreach, words_eol[1]);
	return TRUE;
}

struct command
{
	uint hash; // From irc_str_hash()
	gboolean (*callback)(IrcContext*, const GStrv, const GStrv);
	const char *help;
};

static const struct command commands[] = {
	{ 0x1bbebu, command_say, N_("say <message> | Sends message to current channel") }, // say
	{ 0xd98u, command_me, N_("me <message> | Sends an action to current channel") }, // me
	{ 0x3463f3u, command_part, N_("part [<channel>] | Leaves the channel") }, // part
	{ 0xc9af9137u, command_allserv, N_("allserv <command> | Runs command on all connected servers") }, // allserv
};

gboolean
handle_command (IrcContext *ctx, const GStrv words, const GStrv words_eol)
{
	uint hash = irc_str_hash (words[0]);
	for (gsize i = 0; i < G_N_ELEMENTS(commands); ++i)
	{
		if (hash == commands[i].hash)
		{
			if (!commands[i].callback(ctx, words, words_eol))
				irc_context_print (ctx, _(commands[i].help));
			return TRUE;
		}
	}

	// Send unknown commands directly to server for now
	IrcServer *serv;
	if (IRC_IS_SERVER(ctx))
		serv = IRC_SERVER(ctx);
	else
	{
		IrcContext *parent = irc_context_get_parent (ctx);
		if (IRC_IS_SERVER(parent))
			serv = IRC_SERVER(parent);
		else
		{
			g_info ("Unknown command in non-server context");
			return FALSE;
		}
	}

	g_info ("sending unknown command %s to server", words_eol[0]);
	irc_server_write_line (serv, words_eol[0]);
	return TRUE;
}
