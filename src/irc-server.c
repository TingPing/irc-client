/* irc-server.c
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
#include <stdlib.h>
#include <string.h>
#include <libnotify/notify.h>
#include "irc-context-action.h"
#include "irc-context-manager.h"
#include "irc-context.h"
#include "irc-user.h"
#include "irc-channel.h"
#include "irc-server.h"
#include "irc-message.h"
#include "irc-query.h"
#include "irc-utils.h"
#include "irc-enumtypes.h"
#include "irc-marshal.h"

struct _IrcServerClass
{
	GObjectClass parent_class;
	gboolean (*inbound_line)(IrcServer *self, const char *line);
};

typedef struct
{
  	GIConv in_decoder;
	GIConv out_encoder;
	char *host;
	char *network_name;
	GSocketClient *socket;
	GSocketConnection *conn;
	GHashTable *usertable;
	GHashTable *chantable;
	GHashTable *querytable;
	IrcUser *me;
  	GCancellable *connect_cancel;
	GCancellable *read_cancel;
	char *nick_prefixes;
	char *nick_modes;
	char *chan_types;
  	char *chan_modes;
	char *encoding;
	GQueue *sendq;

	// CAP negotiation...
	gboolean sent_capend;
	gboolean waiting_on_cap;
	gboolean waiting_on_sasl;

	guint has_sendq;
	guint caps;
  	guint16 port;
} IrcServerPrivate;

enum {
	CONNECTED,
	INBOUND,
	N_SIGNALS
};

static guint obj_signals[N_SIGNALS];

static void irc_server_iface_init (IrcContextInterface *iface);

G_DEFINE_TYPE_WITH_CODE (IrcServer, irc_server, G_TYPE_OBJECT,
						G_IMPLEMENT_INTERFACE (IRC_TYPE_CONTEXT, irc_server_iface_init)
						G_ADD_PRIVATE (IrcServer))

static void
on_user_unref (gpointer data, GObject *obj, gboolean is_last_ref)
{
	IrcUser *user = IRC_USER(obj);
	IrcServerPrivate *priv = irc_server_get_instance_private (IRC_SERVER(data));

	if (is_last_ref)
	{
		g_hash_table_remove (priv->usertable, user->nick);
		g_object_unref (user);
	}
}

static void
usertable_insert (IrcServer *self, IrcUser *user)
{
  	IrcServerPrivate *priv = irc_server_get_instance_private (self);

	g_object_add_toggle_ref (G_OBJECT(user), on_user_unref, self);

	if (!g_hash_table_replace (priv->usertable, user->nick, user))
		g_assert_not_reached ();
}

static inline IrcUser *
usertable_lookup (IrcServer *self, const char *nick)
{
	IrcServerPrivate *priv = irc_server_get_instance_private (self);

	IrcUser *user = g_hash_table_lookup (priv->usertable, nick);
	if (user != NULL)
		g_object_ref (user);

	return user;
}


static char *
nick_from_host (const char *host)
{
	g_assert (host != NULL);

	char *p = strchr (host, '!');
	if (p == NULL)
		return g_strdup (host);
	else
		return g_strndup (host, (guintptr)p - (guintptr)host);
}

static void
inbound_ctcp (IrcServer *self, IrcMessage *msg)
{
 	IrcServerPrivate *priv = irc_server_get_instance_private (self);

	if (irc_str_equal(irc_message_get_param(msg, 0), priv->me->nick))
	{
		if (g_str_equal (irc_message_get_param(msg, 1), "\001VERSION\001"))
		{
			g_autofree char *nick = nick_from_host (msg->sender);
			irc_server_write_linef (self, "NOTICE %s :\001VERSION TingPingChat\001", nick);
		}
		else
		{
			g_warning ("Unknown CTCP to me");
		}
	}
	else
	{
		g_warning ("Channel CTCP, ignored");
	}
}

static void
notification_activated_action (NotifyNotification *notification, char *action, gpointer data)
{
	GApplication *app = g_application_get_default ();
	g_action_group_activate_action (G_ACTION_GROUP (app), "focus-context", g_variant_new_string (data));
	g_application_activate (app);
}

static void
show_notification (IrcContext *ctx, const char *title, const char *body)
{
	IrcContextManager *mgr = irc_context_manager_get_default ();
	if (ctx == irc_context_manager_get_front_context (mgr))
		return; // TODO: Check window focus

#if 0
	g_autoptr (GNotification) notification = g_notification_new (title);
	g_notification_set_body (notification, body);
	g_notification_set_sound_name (notification, "message-new-instant");
	g_notification_set_priority (notification, G_NOTIFICATION_PRIORITY_NORMAL);
  	const char *ctx_id = irc_context_get_id (ctx);
	g_notification_set_default_action_and_target (notification, "app.focus-context", "s", ctx_id);
	g_application_send_notification (g_application_get_default (), NULL, notification);
#else
	if (G_UNLIKELY(!notify_is_initted ()))
	{
		if (!notify_init (g_get_application_name ()))
			return;
	}
	NotifyNotification *notification = notify_notification_new (title, body, NULL);
	notify_notification_set_hint (notification, "desktop-entry", g_variant_new_string ("se.tingping.IrcClient"));
	notify_notification_set_hint (notification, "sound-name", g_variant_new_string ("message-new-instant"));
	notify_notification_add_action (notification, "default", _("Focus"), notification_activated_action,
									g_strdup(irc_context_get_id (ctx)), g_free);
	notify_notification_show (notification, NULL);
#endif
}

static void
inbound_privmsg (IrcServer *self, IrcMessage *msg)
{
	IrcServerPrivate *priv = irc_server_get_instance_private (self);
	IrcContext *dest_ctx;
	gboolean is_action = FALSE, is_highlight = FALSE, is_chan = TRUE, is_you = FALSE;

	// TODO: Make this function less ugly

	const char *first_word = irc_message_get_param(msg, 1);
	if (g_str_has_prefix (first_word, "\001") &&
		g_str_has_suffix (first_word, "\001"))
	{
		if (!g_str_has_prefix (first_word, "\001ACTION"))
		{
			inbound_ctcp (self, msg);
			return;
		}
		is_action = TRUE;
	}

	g_autofree char *ctx_nick = nick_from_host (msg->sender);
	if (irc_str_equal (ctx_nick, priv->me->nick))
	{
		is_you = TRUE;
	}
	if (strchr (priv->chan_types, irc_message_get_param(msg, 0)[0]) == NULL)
	{
		if (is_you)
		{
			g_free (ctx_nick);
			ctx_nick = g_strdup (irc_message_get_param(msg, 0));
		}
		IrcContextManager *mgr = irc_context_manager_get_default ();
		g_autoptr(IrcUser) user = usertable_lookup (self, ctx_nick);
		if (user == NULL)
		{
			if (is_you)
				user = irc_user_new (ctx_nick);
			else
				user = irc_user_new (msg->sender);
			usertable_insert (self, user);
		}
		dest_ctx = g_hash_table_lookup (priv->querytable, ctx_nick);
		if (dest_ctx == NULL)
		{
			g_debug ("Found nothing for %s, making new", ctx_nick);
			IrcQuery *query = irc_query_new (IRC_CONTEXT(self), user);
			if (!g_hash_table_replace (priv->querytable, user->nick, query))
				g_assert_not_reached ();
			dest_ctx = IRC_CONTEXT(query);
			irc_context_manager_add (mgr, dest_ctx);
			if (priv->caps & IRC_SERVER_SUPPORT_MONITOR)
			{
				irc_server_write_linef (self, "MONITOR + %s", ctx_nick);
			}
		}
		is_chan = FALSE;
	}
	else
	{
		IrcChannel *chan = g_hash_table_lookup (priv->chantable, irc_message_get_param(msg, 0));
		if (chan == NULL)
		{
			g_warning ("Recieved PRIVMSG for unknown channel");
			return;
		}
		dest_ctx = IRC_CONTEXT(chan);
	}

	g_autofree char *formatted;
	g_autofree char *nick = nick_from_host (msg->sender);

	if (priv->caps & IRC_SERVER_CAP_TWITCH_TAGS)
	{
		const char *display_name = irc_message_get_tag_value (msg, "display-name");
		// It might be safe to support display-name != irc nick but not for now
		if (display_name != NULL && irc_str_equal (nick, display_name))
		{
			g_free (nick);
			nick = g_strdup (display_name);
		}
	}

	if (!is_you && irc_strcasestr (irc_message_get_param(msg, 1), priv->me->nick) != NULL)
		is_highlight = TRUE;

	g_autofree char *stripped = NULL;
	gboolean strip = irc_context_lookup_setting_boolean (dest_ctx, "stripcolor");

	if (is_highlight || !is_chan || strip)
	{
		stripped = irc_strip_attributes (irc_message_get_param(msg, 1) + (is_action ? 8 : 0));
		if (is_action)
			stripped[strlen(stripped) - 1] = '\0';
	}

	if (!is_action)
	{
		const char *text = (strip ? stripped : irc_message_get_param(msg, 1));
		if (is_highlight && is_chan && !is_you)
		{
			if (!msg->timestamp)
				show_notification (dest_ctx, "Highlight", stripped);
			formatted = g_strdup_printf ("\002\00303%s\002 %s", nick, text);
		}
		else if (is_you)
		{
			formatted = g_strdup_printf ("\00304%s \00314%s", nick, text);
		}
		else
		{
			if (!is_chan && !msg->timestamp)
			{
				show_notification (dest_ctx, "Private Message", stripped);
			}
			const char *user_color = NULL;
			if (priv->caps & IRC_SERVER_CAP_TWITCH_TAGS)
				user_color = irc_message_get_tag_value (msg, "color");
			if (user_color && *user_color)
				formatted = g_strdup_printf ("\004%s%s\00399 %s", user_color + 1, nick, text);
			else
				formatted = g_strdup_printf ("\00302%s\00399 %s", nick, text);
		}
	}
	else
	{
		const char *text = (strip ? stripped : (irc_message_get_param(msg, 1) + 8));
		const int len = (int)strlen (text) - (strip ? 0 : 1);
		if (is_highlight && is_chan)
		{
			if (!msg->timestamp)
				show_notification (dest_ctx, "Highlight", stripped);
			formatted = g_strdup_printf ("* \002\00303%s\002 %.*s", nick, len, text);
		}
		else if (is_you)
		{
			formatted = g_strdup_printf ("* \002\00304%s\002\00314 %.*s", nick, len, text);
		}
		else
		{
			if (!is_chan && !msg->timestamp)
			{
				show_notification (dest_ctx, "Private Message", stripped);
			}
			formatted = g_strdup_printf ("* \002\00302%s\00399\002 %.*s", nick, len, text);
		}
	}

	if (!msg->timestamp && !is_you) // Ignore znc messages
		g_signal_emit_by_name (dest_ctx, "activity", is_highlight || !is_chan);
	irc_context_print_with_time (dest_ctx, formatted, msg->timestamp);
}
static void
inbound_part (IrcServer *self, IrcMessage *msg)
{
	IrcServerPrivate *priv = irc_server_get_instance_private (self);

	IrcChannel *channel = g_hash_table_lookup (priv->chantable, irc_message_get_param(msg, 0));
	if (channel == NULL)
	{
		g_warning ("Got PART for unknown channel");
		return;
	}

	g_autofree char *nick = nick_from_host (msg->sender);
	g_autoptr(IrcUser) user = usertable_lookup (self, nick);
	if (user == NULL)
	{
		g_warning ("Got PART for uknown user");
		return;
	}
	else if (user == priv->me)
	{
		irc_channel_set_joined (channel, FALSE);
		return;
	}

  	if (!irc_context_lookup_setting_boolean (IRC_CONTEXT(channel), "hide-joinpart"))
	{
		g_autofree char *formatted = g_strdup_printf ("\035<-- %s parted", nick);
		irc_context_print_with_time (IRC_CONTEXT(channel), formatted, msg->timestamp);
	}

	IrcUserList *ulist = irc_channel_get_users (channel);
	irc_user_list_remove (ulist, user);
}


static void
quit_foreach (gpointer key, gpointer value, gpointer data)
{
	IrcChannel *channel = IRC_CHANNEL(value);
	IrcUser *user = IRC_USER(data);
	IrcUserList *ulist = irc_channel_get_users (channel);

	if (irc_user_list_remove (ulist, user)	&&
		!irc_context_lookup_setting_boolean (IRC_CONTEXT(channel), "hide-joinpart"))
	{
  		g_autofree char *formatted = g_strdup_printf ("\035<-- %s quit", user->nick);
		irc_context_print (IRC_CONTEXT(channel), formatted);
	}
}

static void
inbound_quit (IrcServer *self, IrcMessage *msg)
{
  	IrcServerPrivate *priv = irc_server_get_instance_private (self);
	g_autofree char *nick = nick_from_host (msg->sender);
	g_autoptr(IrcUser) user = usertable_lookup (self, nick);
	if (user == NULL)
	{
		g_warning ("Incoming QUIT for unknown user: %s", nick);
		return;
	}

	g_hash_table_foreach (priv->chantable, quit_foreach, user);
}

static void
inbound_ujoin (IrcServer *self, IrcMessage *msg)
{
  	IrcServerPrivate *priv = irc_server_get_instance_private (self);
	const char *chan_name = irc_message_get_param(msg, 0);
	if (chan_name[0] == ':')
		chan_name++;

  	IrcChannel *channel = g_hash_table_lookup (priv->chantable, chan_name);
	if (channel == NULL)
	{
		channel = irc_channel_new (IRC_CONTEXT(self), chan_name);
		if (!g_hash_table_replace (priv->chantable, channel->name, channel))
			g_assert_not_reached ();

		IrcContextManager *mgr = irc_context_manager_get_default ();
		irc_context_manager_add (mgr, IRC_CONTEXT(channel));
	}
	else
	{
		irc_channel_set_joined (channel, TRUE);
	}
}

static void
inbound_join (IrcServer *self, IrcMessage *msg)
{
	IrcServerPrivate *priv = irc_server_get_instance_private (self);

	g_autofree char *nick = nick_from_host (msg->sender);
	if (irc_str_equal(nick, priv->me->nick))
	{
		inbound_ujoin (self, msg);
		return;
	}

	const char *chan_name = irc_message_get_param(msg, 0);
	IrcChannel *channel = g_hash_table_lookup (priv->chantable, chan_name);
	if (channel == NULL)
	{
		g_warning ("Got join for unknown channel: %s", chan_name);
		return;
	}

	g_autoptr(IrcUser) user = usertable_lookup (self, nick);
	if (user == NULL)
	{
		user = irc_user_new (msg->sender);
		if (priv->caps & IRC_SERVER_CAP_EXTENDED_JOIN)
		{
			const char *account = irc_message_get_param(msg, 1);
			const char *realname = irc_message_get_param(msg, 2);
			g_object_set (user, "account", *account == '*' ? NULL : account, "realname", realname, NULL);
		}
		usertable_insert (self, user);
	}

	if (!irc_context_lookup_setting_boolean (IRC_CONTEXT(channel), "hide-joinpart"))
	{
  		g_autofree char *formatted = g_strdup_printf ("\035--> %s joined", nick);
		irc_context_print_with_time (IRC_CONTEXT(channel), formatted, msg->timestamp);
	}

	IrcUserList *ulist = irc_channel_get_users (channel);
	irc_user_list_add (ulist, user, NULL);
}

static void
inbound_names (IrcServer *self, IrcMessage *msg)
{
	IrcServerPrivate *priv = irc_server_get_instance_private (self);
	gsize len = g_strv_length (msg->params);

	if (len < 4)
	{
		g_warning ("names too short");
		return;
	}

	IrcChannel *channel = g_hash_table_lookup (priv->chantable, irc_message_get_param(msg, 2));
	if (channel == NULL)
	{
		g_warning ("Got names for unknown channel");
		return;
	}

	IrcUserList *ulist = irc_channel_get_users (channel);
#if 0
	// First part of NAMES, clear existing list
	if (FALSE) // TODO
	{
		GtkListStore *list = irc_channel_get_users (channel);
		gtk_list_store_clear (list); // FIXME: This might free users that will just be re-created...
	}
#endif

	g_auto(GStrv) names = g_strsplit (irc_message_get_param (msg, 3), " ", 0);
	for (gsize i = 0; names[i]; ++i)
	{
		g_autofree char *nick;
		if (priv->caps & IRC_SERVER_CAP_USERHOST_IN_NAMES)
			nick = nick_from_host (names[i]);
		else
			nick = g_strdup (names[i]);

		gsize offset = 0;
		while (strchr (priv->nick_prefixes, nick[offset]) != NULL)
			++offset;

		g_autoptr(IrcUser) user = usertable_lookup (self, nick + offset);
		if (user == NULL)
		{
			user = irc_user_new (names[i] + offset); // Want full-host here
			usertable_insert (self, user);
		}

		g_autofree char *prefix = NULL;
		if (offset)
			prefix = g_strndup (nick, offset);

		irc_user_list_add (ulist, user, prefix);
	}
}

static void
inbound_endofnames (IrcServer *self, IrcMessage *msg)
{
	IrcServerPrivate *priv = irc_server_get_instance_private (self);
	if (!(priv->caps & IRC_SERVER_SUPPORT_WHOX))
		return;

	irc_server_write_linef (self, "WHO %s %%chtsunfra,152", irc_message_get_param(msg, 1));
}

static char *
join_list (GList *list, const char sep)
{
	GString *str = g_string_new (list->data);

	while ((list = g_list_next (list)))
	{
		g_string_append_c (str, sep);
		g_string_append (str, list->data);
	}

	return g_string_free (str, FALSE);
}

static void
inbound_endofmotd (IrcServer *self)
{
  	IrcServerPrivate *priv = irc_server_get_instance_private (self);

	if (g_hash_table_size (priv->chantable) != 0) // Had previous connection
	{
		g_autoptr(GList) channels = g_hash_table_get_keys (priv->chantable);
		g_autofree char *join_str = join_list (channels, ',');

		// TODO: Keys and long lines
		irc_server_write_linef (self, "JOIN %s", join_str);
	}
	if (g_hash_table_size (priv->querytable) != 0)
	{
		g_autoptr(GList) queries = g_hash_table_get_keys (priv->querytable);
		if (priv->caps & IRC_SERVER_SUPPORT_MONITOR)
		{
			g_autofree char *join_str = join_list (queries, ',');

			irc_server_write_linef (self, "MONITOR + %s", join_str);
		}
		else
		{
			g_autofree char *join_str = join_list (queries, ' ');

			irc_server_write_linef (self, "ISON %s", join_str);
		}
	}
}

static void
inbound_whox (IrcServer *self, IrcMessage *msg)
{
	IrcServerPrivate *priv = irc_server_get_instance_private (self);

	// We use this numeric in automated WHOs (same as [he]xchat)
	if (!g_str_equal (irc_message_get_param(msg, 1), "152"))
		return;

	// yournick 152 #channel ~ident host servname nick H account :realname
	const char *nick = irc_message_get_param(msg, 6);
	IrcUser *user = g_hash_table_lookup (priv->usertable, nick);
	if (user == NULL)
	{
		g_warning ("Incoming WHOX for unknown user: %s", nick);
		return;
	}

	const char *account = irc_message_get_param(msg, 8);
	const char *realname = irc_message_get_param(msg, 9);

	g_object_set (user, "account", *account == '*' ? NULL : account, "realname", realname, NULL);

	if (!(priv->caps & IRC_SERVER_CAP_USERHOST_IN_NAMES))
	{
		g_object_set (user, "hostname", irc_message_get_param(msg, 4), "username", irc_message_get_param(msg, 3), NULL);
	}

	// If server doesn't have away-notify there is no point in setting this
	if ((priv->caps & IRC_SERVER_CAP_AWAY_NOTIFY))
	{
		g_object_set (user, "away", *irc_message_get_param(msg, 7) == 'G', NULL);
	}

	// We could grab the users prefix here, but we don't need it?
}

static void
nick_change_foreach (gpointer key, gpointer val, gpointer data)
{
	IrcChannel *channel = IRC_CHANNEL(val);
	IrcUser *user = IRC_USER(data);
	IrcUserList *ulist = irc_channel_get_users (channel);

	if (irc_user_list_contains (ulist, user))
	{
		g_autofree char *formatted = g_strdup_printf ("\035* %s changed nick", user->nick);
		irc_context_print (IRC_CONTEXT(channel), formatted);
	}
}

static void
change_users_nick (IrcServer *self, IrcUser *user, const char *new_nick)
{
  	IrcServerPrivate *priv = irc_server_get_instance_private (self);

	if (!g_hash_table_steal (priv->usertable, user->nick))
		g_assert_not_reached ();
	g_object_set (user, "nick", new_nick, NULL);
	if (!g_hash_table_replace (priv->usertable, user->nick, user))
		g_assert_not_reached ();

  	g_hash_table_foreach (priv->chantable, nick_change_foreach, user);
}

static void
inbound_nick (IrcServer *self, IrcMessage *msg)
{
  	IrcServerPrivate *priv = irc_server_get_instance_private (self);
  	g_autofree char *nick = nick_from_host (msg->sender);
	IrcUser *user = g_hash_table_lookup (priv->usertable, nick);
	if (user == NULL)
	{
		g_warning ("Incoming NICK for unknown user: %s", nick);
		return;
	}

	change_users_nick (self, user, irc_message_get_param(msg, 0));
}

static void
inbound_user_online (IrcServer *self, const char *users, gboolean online, const char *str_sep)
{
  	IrcServerPrivate *priv = irc_server_get_instance_private (self);
	g_auto(GStrv) nicks = g_strsplit (users, str_sep, -1);

	for (gsize i = 0; nicks[i]; ++i)
	{
		g_autofree char *nick = nick_from_host(nicks[i]);
		IrcQuery *query = g_hash_table_lookup (priv->querytable, nick);
		if (query == NULL)
		{
			g_warning ("Inbound MONITOR/ISON for unknown user");
			continue;
		}
		irc_query_set_online (query, online);
		if (irc_query_get_user (query) == NULL)
		{
			g_autoptr(IrcUser) user = irc_user_new (nicks[i]);
			usertable_insert (self, user);
			g_object_set (query, "user", user, NULL);
		}
	}
}

static void
inbound_topic (IrcServer *self, const char *chan, const char *topic)
{
  	IrcServerPrivate *priv = irc_server_get_instance_private (self);

  	IrcChannel *channel = g_hash_table_lookup (priv->chantable, chan);
	if (channel == NULL)
	{
		g_warning ("Got TOPIC for unknown channel %s", chan);
		return;
	}
	g_object_set (channel, "topic", topic, NULL);
}

static void
inbound_account (IrcServer *self, IrcMessage *msg)
{
	IrcServerPrivate *priv = irc_server_get_instance_private (self);
	g_autofree char *nick = nick_from_host (msg->sender);
	IrcUser *user = g_hash_table_lookup (priv->usertable, nick);
	if (user == NULL)
	{
		g_warning ("Incoming ACCOUNT for unknown user: %s", nick);
		return;
	}

	const char *account = irc_message_get_param(msg, 0);
	g_object_set (user, "account", *account == '*' ? NULL : account, NULL);
}

static void
inbound_mode (IrcServer *self, IrcMessage *msg)
{
	IrcServerPrivate *priv = irc_server_get_instance_private (self);
	IrcChannel *channel = g_hash_table_lookup (priv->chantable, irc_message_get_param(msg, 0));
	if (channel == NULL)
	{
		g_warning("Incoming MODE for unknown channel %s", irc_message_get_param(msg, 0));
		return;
	}

#if 0
	// TODO: Loop over mode changes and handle them

		IrcUser *user = g_hash_table_lookup (priv->usertable, ...);
		if (user == NULL)
		{
			g_warning ("Incoming MODE for unknown user %s", ...);
			return;
		}

		IrcUserList *ulist = irc_channel_get_users (channel);
		irc_user_list_set_users_prefix (ulist, user, ...);

		// TODO: Print out MODE message
#endif
}

static void
inbound_away (IrcServer *self, IrcMessage *msg)
{
	IrcServerPrivate *priv = irc_server_get_instance_private (self);
	g_autofree char *nick = nick_from_host (msg->sender);
	IrcUser *user = g_hash_table_lookup (priv->usertable, nick);
	if (user == NULL)
	{
		g_warning ("Incoming AWAY for unknown user: %s", nick);
		return;
	}

	g_object_set (user, "away", msg->params[0] != NULL,
			   "away-reason", msg->params[0], NULL);
}

static void
inbound_chghost (IrcServer *self, IrcMessage *msg)
{
	IrcServerPrivate *priv = irc_server_get_instance_private (self);
	g_autofree char *nick = nick_from_host (msg->sender);
	IrcUser *user = g_hash_table_lookup (priv->usertable, nick);
	if (user == NULL)
	{
		g_warning ("Incoming CHGHOST for unknown user: %s", nick);
		return;
	}

	g_object_set (user, "username", irc_message_get_param(msg, 0), "hostname", irc_message_get_param(msg, 1), NULL);
}

static void
inbound_authenticate_response (IrcServer *self, IrcMessage *msg)
{
	IrcServerPrivate *priv = irc_server_get_instance_private (self);
	priv->waiting_on_sasl = FALSE;
	if (!priv->sent_capend && !priv->waiting_on_cap)
	{
		irc_server_write_line (self, "CAP END");
		priv->sent_capend = TRUE;
	}
}

static void
inbound_authenticate (IrcServer *self, IrcMessage *msg)
{
	if (g_str_equal (irc_message_get_param(msg, 0), "+"))
	{
	  	IrcServerPrivate *priv = irc_server_get_instance_private (self);
		g_autofree char *path = g_strconcat ("/se/tingping/IrcClient/", priv->network_name, "/", NULL);
		g_autoptr(GSettings) settings = g_settings_new_with_path ("se.tingping.network", path);
		g_autofree char *user = g_settings_get_string (settings, "sasl-username");
		g_autofree char *pass = g_settings_get_string (settings, "sasl-password");
		g_autofree char*encoded = irc_sasl_encode_plain (user, pass);
		irc_server_write_linef (self, "AUTHENTICATE %s", encoded);
	}
	else
	{
		g_warning ("Unknown AUTHENTICATE");
	}
}

struct SupportedCap
{
	const char *name;
	const IrcServerCap cap;
};

const struct SupportedCap supported_caps[] =
{
	{ "server-time", IRC_SERVER_CAP_SERVER_TIME },
	{ "userhost-in-names", IRC_SERVER_CAP_USERHOST_IN_NAMES },
	{ "extended-join", IRC_SERVER_CAP_EXTENDED_JOIN },
	{ "account-notify", IRC_SERVER_CAP_ACCOUNT_NOTIFY },
	{ "chghost", IRC_SERVER_CAP_CHGHOST },
	{ "multi-prefix", IRC_SERVER_CAP_MULTI_PREFIX },
	{ "sasl", IRC_SERVER_CAP_SASL },
	{ "away-notify" , IRC_SERVER_CAP_AWAY_NOTIFY },
	{ "cap-notify", IRC_SERVER_CAP_CAP_NOTIFY },
	{ "znc.in/server-time-iso", IRC_SERVER_CAP_SERVER_TIME },
	{ "znc.in/self-message", IRC_SERVER_CAP_ZNC_SELF_MESSAGE },
	{ "twitch.tv/membership", IRC_SERVER_CAP_TWITCH_MEMBERSHIP },
	{ "twitch.tv/tags", IRC_SERVER_CAP_TWITCH_TAGS },
};

static void
inbound_cap (IrcServer *self, IrcMessage *msg)
{
  	IrcServerPrivate *priv = irc_server_get_instance_private (self);

	if (irc_str_equal (irc_message_get_param(msg, 1), "LS") || irc_str_equal (irc_message_get_param(msg, 1), "NEW"))
	{
	  	g_autofree char *path = g_strconcat ("/se/tingping/IrcClient/", priv->network_name, "/", NULL);
		g_autoptr(GSettings) settings = g_settings_new_with_path ("se.tingping.network", path);
		char *user = g_settings_get_string (settings, "sasl-username");
		char *pass = g_settings_get_string (settings, "sasl-password");
		const gboolean wants_sasl = *user && *pass;
		g_free (user);
		g_free (pass);

		const gboolean multiline = *irc_message_get_param(msg, 2) == '*';
		priv->waiting_on_cap = multiline;
		g_auto(GStrv) caps = g_strsplit (irc_message_get_param (msg, 2 + (multiline ? 1 : 0)), " ", 0);
		char outbuf[512];
		outbuf[0] = '\0';

		for (gsize i = 0; caps[i]; ++i)
		{
			const char *cap = caps[i];
			char *value = strchr (cap, '=');
			if (value)
			{
				*value = '\0';
				value++;
			}

			if (irc_str_equal (cap, "sasl"))
			{
				if (!wants_sasl)
					continue;

				// We only support PLAIN
				if (value && !strstr (value, "PLAIN"))
					continue;

				g_strlcat (outbuf, "sasl ", sizeof (outbuf));
				continue;
			}

			for (gsize y = 0; y < G_N_ELEMENTS(supported_caps); ++y)
			{
				if (irc_str_equal (cap, supported_caps[y].name))
				{
					g_strlcat (outbuf, cap, sizeof (outbuf));
					g_strlcat (outbuf, " ", sizeof (outbuf));
					break;
				}
			}
		}

		if (*outbuf)
			irc_server_write_linef (self, "CAP REQ :%s", g_strchomp(outbuf));
		else if (!priv->sent_capend && !priv->waiting_on_cap)
		{
			irc_server_write_line (self, "CAP END");
			priv->sent_capend = TRUE;
		}
	}
	else if (irc_str_equal (irc_message_get_param(msg, 1), "ACK"))
	{
		g_auto(GStrv) caps = g_strsplit (irc_message_get_param(msg, 2), " ", 0);

		for (gsize i = 0; caps[i]; ++i)
		{
			const char *cap = caps[i];
			for (gsize y = 0; y < G_N_ELEMENTS(supported_caps); ++y)
			{
				if (irc_str_equal (cap, supported_caps[y].name))
				{
					priv->caps |= supported_caps[y].cap;
					break;
				}
			}
		}

		if ((priv->caps & IRC_SERVER_CAP_SASL) && !priv->waiting_on_sasl)
		{
			irc_server_write_line (self, "AUTHENTICATE PLAIN");
			priv->waiting_on_sasl = TRUE;
		}
		else if (!priv->waiting_on_sasl && !priv->waiting_on_cap && !priv->sent_capend)
		{
			irc_server_write_line (self, "CAP END");
			priv->sent_capend = TRUE;
		}
	}
	else if (irc_str_equal (irc_message_get_param(msg, 1), "DEL"))
	{
		g_auto(GStrv) caps = g_strsplit (irc_message_get_param(msg, 2), " ", 0);

		for (gsize i = 0; caps[i]; ++i)
		{
			const char *cap = caps[i];
			for (gsize y = 0; y < G_N_ELEMENTS(supported_caps); ++y)
			{
				if (irc_str_equal (cap, supported_caps[y].name))
				{
					priv->caps ^= supported_caps[y].cap;
					break;
				}
			}
		}
	}
	else if (irc_str_equal (irc_message_get_param(msg, 1), "LIST"))
	{
		// Could clean up formatting
		irc_context_print_with_time (IRC_CONTEXT(self), irc_message_get_param(msg, 2), msg->timestamp);
	}
	else if (irc_str_equal (irc_message_get_param(msg, 1), "NAK"))
	{
		// Don't realy have anything to do
	}
}

static void
inbound_005 (IrcServer *self, IrcMessage *msg)
{
	IrcServerPrivate *priv = irc_server_get_instance_private (self);
	const gsize len = g_strv_length (msg->params) - 1; // Ends with :are supported by this server

	for (gsize i = 1; i < len; ++i)
	{
		const char *word = irc_message_get_param(msg, i);

		if (g_str_has_prefix (word, "PREFIX="))
		{
			if (strlen (word) == 7) // No prefixes is valid
			{
				g_object_set (self, "nickmodes", "", "nickprefixes", "", NULL);
				continue;
			}

			char *modes = NULL, *prefixes = NULL;
			if ((sscanf (word + 7, "(%ms)%ms", &modes, &prefixes) != 2))
				g_warning ("Bad PREFIX in 005");
			else
				g_object_set (self, "nickmodes", modes, "nickprefixes", prefixes, NULL);

			free (modes);
			free (prefixes);
		}
		else if (g_str_has_prefix (word, "CHANTYPES="))
		{
			g_object_set (self, "chantypes", word + 10, NULL);
		}
		else if (g_str_has_prefix (word, "CHANMODES="))
		{
			g_object_set (self, "chanmodes", word + 10, NULL);
		}
		else if (g_str_equal (word, "WHOX"))
			priv->caps |= IRC_SERVER_SUPPORT_WHOX;
		else if (g_str_has_prefix (word, "MONITOR"))
			priv->caps |= IRC_SERVER_SUPPORT_MONITOR;
		else if (g_str_has_prefix (word, "-MONITOR"))
			priv->caps ^= IRC_SERVER_SUPPORT_MONITOR;
		else if (g_str_equal (word, "-WHOX"))
			priv->caps ^= IRC_SERVER_SUPPORT_WHOX;
		// CASEMAPPING
		// STATUSMSG
	}

	irc_context_print_with_time (IRC_CONTEXT(self), irc_message_get_word_eol(msg, 1), msg->timestamp);
}

enum {
	QUIT = 0x7c8aed88u,
	PART = 0x7c8a0d3cu,
	JOIN = 0x7c86fd55u,
	PRIVMSG = 0xd693152du,
	AWAY = 0x7c822ef7u,
	PING = 0x7c8a2eb3u,
	NOTICE = 0xc39644e7u,
	ERROR= 0xd0df94fu,
	CAP = 0xb87d899u,
	MODE = 0x7c88a1cau,
	NICK = 0x7c89148au,
	ACCOUNT = 0x307bdc32u,
	CHGHOST = 0xd65e9355u,
	TOPIC = 0xe1bba64u,
	AUTHENTICATE = 0xfea21e4u,
};

static gboolean
handle_incoming (IrcServer *self, const char *line)
{
	IrcServerPrivate *priv = irc_server_get_instance_private (self);
	g_autoptr(IrcMessage) msg = irc_message_new (line);
	if (msg == NULL)
		return TRUE;

	if (msg->numeric)
	{
		//g_signal_emit_by_name (self, "numeric", g_quark_from_string (msg->command), msg);
		switch (msg->numeric)
		{
		case 1: // RPL_WELCOME
			irc_context_print_with_time (IRC_CONTEXT(self), irc_message_get_param(msg, 1), msg->timestamp);
			break;
		case 5: // RPL_ISUPPORT
			inbound_005 (self, msg);
			break;
		case 303: // RPL_ISON
			inbound_user_online (self, irc_message_get_param(msg, 1), TRUE, " ");
			break;
		case 315: // RPL_ENDOFWHO
			break;
		case 332: // RPL_TOPIC
			inbound_topic (self, irc_message_get_param(msg, 1), irc_message_get_param(msg, 2));
			break;
		case 333: // RPL_TOPICWHO_TIME
			break; // We don't care?
		case 353: // RPL_NAMREPLY
			inbound_names (self, msg);
			break;
		case 354: // RPL_WHOSPCRPL (WHOX)
			inbound_whox (self, msg);
			break;
		case 366: // RPL_ENDOFNAMES
			inbound_endofnames (self, msg);
			break;
		case 375: // RPL_MOTDSTART
		case 372: // RPL_MOTD
			irc_context_print_with_time (IRC_CONTEXT(self), irc_message_get_param(msg, 1), msg->timestamp);
			break;
		case 376: // RPL_ENDOFMOTD
			inbound_endofmotd (self);
			break;
		case 433: // ERR_NICKNAMEINUSE
			{
				// TODO: configurable
				g_autofree char *new_nick = g_strconcat (irc_message_get_param(msg, 1), "_", NULL);
				change_users_nick (self, priv->me, new_nick);
				irc_server_write_linef (self, "NICK %s", priv->me->nick);
			}
			break;
		case 730: // RPL_MONONLINE
			inbound_user_online (self, irc_message_get_param(msg, 1), TRUE, ",");
			break;
		case 731: // RPL_MONOFFLINE
			inbound_user_online (self, irc_message_get_param(msg, 1), FALSE, ",");
			break;
		case 903: // RPL_SASLSUCCESS
		case 904: // RPL_SASLFAIL
		case 905: // ERR_SASLTOOLONG
		case 906: // ERR_SASLABORTED
			// TODO: Show info to user and improve handling
			inbound_authenticate_response (self, msg);
			break;
		default:
			g_debug ("Unhandled numeric %"G_GUINT16_FORMAT, msg->numeric);
			return FALSE;
		}
	}
	else if (msg->command)
	{
		switch (g_str_hash(msg->command))
		{
		case PRIVMSG:
			inbound_privmsg (self, msg);
			break;
		case JOIN:
			inbound_join (self, msg);
			break;
		case QUIT:
			inbound_quit (self, msg);
			break;
		case PART:
			inbound_part (self, msg);
			break;
		case AWAY:
			inbound_away (self, msg);
			break;
		case NOTICE:
			irc_context_print_with_time (IRC_CONTEXT(self), irc_message_get_param(msg, 1), msg->timestamp);
			break;
		case NICK:
			inbound_nick (self, msg);
			break;
		case ACCOUNT:
			inbound_account (self, msg);
			break;
		case PING:
			irc_server_write_linef (self, "PONG %s", msg->content);
			break;
		case ERROR:
			irc_context_print_with_time (IRC_CONTEXT(self), irc_message_get_param(msg, 0), msg->timestamp);
			break;
		case CAP:
			inbound_cap (self, msg);
			break;
		case MODE:
			inbound_mode (self, msg);
			break;
		case CHGHOST:
			inbound_chghost (self, msg);
			break;
		case TOPIC:
			inbound_topic (self, irc_message_get_param(msg, 0), irc_message_get_param(msg, 1));
			break;
		case AUTHENTICATE:
			inbound_authenticate (self, msg);
			break;
		default:
			g_debug ("Unhandled command %s", msg->command);
			return FALSE;
		}
	}
	else
		g_assert_not_reached();

	return TRUE;
}

void
irc_server_write_linef (IrcServer *self, const char *fmt, ...)
{
	char *formatted;
	va_list args;

	va_start (args, fmt);
	formatted = g_strdup_vprintf (fmt, args);
	va_end (args);

	irc_server_write_line (self, formatted);
	g_free (formatted);
}

static void
on_writeline_ready (GObject *source, GAsyncResult *res, gpointer data)
{
	GError *err = NULL;

	g_output_stream_write_finish (G_OUTPUT_STREAM(source), res, &err);
	if (err != NULL)
	{
		g_warning ("Writing error: %s", err->message);
		g_clear_error (&err);
	}
}

static gboolean
process_sendq (gpointer data)
{
	IrcServer *self = IRC_SERVER(data);
	IrcServerPrivate *priv = irc_server_get_instance_private (self);
	GOutputStream *out_stream = g_io_stream_get_output_stream (G_IO_STREAM(priv->conn));

	if (g_output_stream_has_pending (out_stream))
		return G_SOURCE_CONTINUE;

	g_autofree char *out_buf = g_queue_pop_head (priv->sendq);
	g_print ("\033[31m<<\033[0m %s", out_buf);
	g_autofree char *out_encoded;
	if (g_ascii_strcasecmp (priv->encoding, "UTF-8") == 0)
		out_encoded = g_utf8_make_valid (out_buf, (gssize)strlen(out_buf));
	else
		out_encoded = irc_convert_invalid_text (out_buf, (gssize)strlen(out_buf), priv->out_encoder, "?");
	g_output_stream_write_async (out_stream, out_encoded, strlen(out_encoded), G_PRIORITY_DEFAULT, NULL,
								on_writeline_ready, self);

	if (g_queue_get_length (priv->sendq))
		return G_SOURCE_CONTINUE;

	priv->has_sendq = 0;
	return G_SOURCE_REMOVE;
}

/**
 * irc_server_get_me:
 * Returns: (transfer none): Your user or %NULL
 */
IrcUser *
irc_server_get_me (IrcServer *self)
{
	IrcServerPrivate *priv = irc_server_get_instance_private (self);
	return priv->me;
}

void
irc_server_write_line (IrcServer *self, const char *line)
{
  	IrcServerPrivate *priv = irc_server_get_instance_private (self);
	GOutputStream *out_stream;

	g_return_if_fail (priv->conn != NULL);
	g_return_if_fail (g_socket_connection_is_connected (priv->conn));

	out_stream = g_io_stream_get_output_stream (G_IO_STREAM(priv->conn));

	char *out_buf = g_strdup_printf ("%s\r\n", line);
	if (g_output_stream_has_pending (out_stream) || priv->has_sendq)
	{
		// Might need to tweak to be faster at first but throttle with tons of lines
		// Also maybe queue certain events before others (PRIVMSG > WHO)
		g_queue_push_tail (priv->sendq, out_buf);
		if (!priv->has_sendq)
		{
			priv->has_sendq = g_timeout_add_seconds (1, process_sendq, self);
		}
	}
	else
	{
		g_print ("\033[31m<<\033[0m %s", out_buf);
		g_autofree char *out_encoded;
		if (g_ascii_strcasecmp (priv->encoding, "UTF-8") == 0)
			out_encoded = g_utf8_make_valid (out_buf, (gssize)strlen(out_buf));
		else
			out_encoded = irc_convert_invalid_text (out_buf, (gssize)strlen(out_buf), priv->out_encoder, "?");
		g_output_stream_write_async (out_stream, out_encoded, strlen(out_encoded), G_PRIORITY_DEFAULT, NULL,
									on_writeline_ready, self);
		g_free (out_buf);
	}
}

static void
on_readline_ready (GObject *source, GAsyncResult *res, gpointer data)
{
	g_autofree char *input = NULL;
	GError *err = NULL;
	GDataInputStream *in_stream = G_DATA_INPUT_STREAM(source);
	gsize len;
	IrcServer *server = IRC_SERVER(data);

	input = g_data_input_stream_read_line_finish (in_stream, res, &len, &err);
	if (err != NULL)
	{
		g_warning ("Reading error: %s", err->message);
		g_clear_error (&err);
		irc_server_disconnect (server);
		return;
	}
	else if (input == NULL)
	{
		g_warning ("Empty line, End of stream");
		irc_server_disconnect (server);
		return;
	}

	IrcServerPrivate *priv = irc_server_get_instance_private (server);

	g_assert (len <= G_MAXSSIZE);
	g_autofree char *utf8_input;
	if (g_ascii_strcasecmp (priv->encoding, "UTF-8") == 0)
		utf8_input = g_utf8_make_valid (input, (gssize)len);
	else
		utf8_input = irc_convert_invalid_text (input, (gssize)len, priv->in_decoder, "�");
	g_print ("\033[32m>>\033[0m %s\n", utf8_input);
	gboolean handled;
	g_signal_emit (server, obj_signals[INBOUND], 0, utf8_input, &handled);

	g_data_input_stream_read_line_async (in_stream, G_PRIORITY_LOW, priv->read_cancel,
										on_readline_ready, server);
}

static void
connect_ready(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *err = NULL;
	GSocketConnection *connection;

	connection = g_socket_client_connect_to_uri_finish (G_SOCKET_CLIENT(source), res, &err);
  	if (err != NULL)
	{
		g_warning ("Connecting error: %s", err->message);
		g_clear_error (&err);
		return;
	}

  	IrcServer *self = IRC_SERVER(data);
  	IrcServerPrivate *priv = irc_server_get_instance_private (self);
	priv->conn = connection;
	g_clear_object(&priv->connect_cancel);

	g_signal_emit (self, obj_signals[CONNECTED], 0);
  	g_object_notify (G_OBJECT(self), "active");

	GIOStream *io_stream;
  	GDataInputStream *in_stream;

	io_stream = G_IO_STREAM(priv->conn);
	in_stream = g_data_input_stream_new (g_io_stream_get_input_stream (io_stream));
	priv->read_cancel = g_cancellable_new ();

	g_data_input_stream_set_newline_type (in_stream, G_DATA_STREAM_NEWLINE_TYPE_CR_LF);
	g_data_input_stream_read_line_async (in_stream, G_PRIORITY_LOW, priv->read_cancel,
										on_readline_ready, self);

  	g_autofree char *path = g_strconcat ("/se/tingping/IrcClient/", priv->network_name, "/", NULL);
	g_autoptr(GSettings) settings = g_settings_new_with_path ("se.tingping.network", path);
	g_autofree char *nick = g_settings_get_string (settings, "nickname");
	g_autofree char *realname = g_settings_get_string (settings, "realname");
	g_autofree char *username = g_settings_get_string (settings, "server-username");
	g_autofree char *password = g_settings_get_string (settings, "server-password");
	priv->me = irc_user_new (nick);
	g_object_set (priv->me, "realname", realname, "username", username, NULL); // FIXME: Username might be wrong
	if (!g_hash_table_replace (priv->usertable, priv->me->nick, priv->me))
		g_assert_not_reached ();

	if (*password)
		irc_server_write_linef (self, "PASS %s\r\nCAP LS 302\r\nNICK %s\r\nUSER %s * * :%s",
							password, priv->me->nick, priv->me->username, priv->me->realname);
	else
		irc_server_write_linef (self, "CAP LS 302\r\nNICK %s\r\nUSER %s * * :%s",
							priv->me->nick, priv->me->username, priv->me->realname);
}

void
irc_server_connect (IrcServer *self)
{
  	g_return_if_fail (IRC_IS_SERVER(self));

	IrcServerPrivate *priv = irc_server_get_instance_private (self);
	const gboolean tls = g_socket_client_get_tls (priv->socket);

	irc_server_disconnect (self);

	g_autofree char *uri = g_strdup_printf ("%s://%s", tls ? "ircs" : "irc", priv->host);
	priv->connect_cancel = g_cancellable_new ();
	g_socket_client_connect_to_uri_async (priv->socket, uri, tls ? 6697 : 6667, priv->connect_cancel, connect_ready, self);
}

static void
foreach_channel_set_parted (gpointer key, gpointer value, gpointer data)
{
	IrcChannel *channel =  IRC_CHANNEL(value);
	irc_channel_set_joined (channel, FALSE);
}

static void
foreach_query_set_offline (gpointer key, gpointer value, gpointer data)
{
	IrcQuery *query = IRC_QUERY(value);
	irc_query_set_online (query, FALSE);
}

void
irc_server_disconnect (IrcServer *self)
{
	g_return_if_fail (IRC_IS_SERVER(self));

	IrcServerPrivate *priv = irc_server_get_instance_private (self);

	if (priv->connect_cancel)
	{
		g_cancellable_cancel (priv->connect_cancel);
		g_clear_object (&priv->connect_cancel);
	}

  	if (priv->read_cancel)
	{
		g_cancellable_cancel (priv->read_cancel);
		g_clear_object(&priv->read_cancel);
	}

  	if (priv->conn)
	{
		irc_server_write_line (self, "QUIT");
		g_io_stream_close_async (G_IO_STREAM(priv->conn), G_PRIORITY_HIGH, NULL, NULL, NULL);
		g_clear_object (&priv->conn);
	}

	g_hash_table_foreach (priv->chantable, foreach_channel_set_parted, NULL);
  	g_hash_table_foreach (priv->querytable, foreach_query_set_offline, NULL);
	//g_hash_table_remove_all (priv->usertable); // Chan/Query references users
	if (priv->me)
	{
		g_assert (g_hash_table_remove (priv->usertable, priv->me->nick));
		g_clear_object (&priv->me);
	}

	g_assert (g_hash_table_size (priv->usertable) == 0); // Nothing should be left

	// Reset CAP state
	priv->sent_capend = FALSE;
	priv->waiting_on_cap = FALSE;
	priv->waiting_on_sasl = FALSE;

	g_object_notify (G_OBJECT(self), "active");
}

void
irc_server_flushq (IrcServer *self)
{
	IrcServerPrivate *priv = irc_server_get_instance_private (self);

	if (priv->conn)
		g_io_stream_clear_pending (G_IO_STREAM(priv->conn));

	if (priv->has_sendq)
	{
		g_source_remove (priv->has_sendq);
		priv->has_sendq = 0;
	}

	char *p;
	while ((p = g_queue_pop_head (priv->sendq)))
		g_free (p);
}

gboolean
irc_server_get_is_connected (IrcServer *self)
{
	IrcServerPrivate *priv = irc_server_get_instance_private (self);

	if (priv->conn && !g_io_stream_is_closed (G_IO_STREAM(priv->conn)))
		return TRUE;

	return FALSE;
}

/**
 * irc_server_get_action_group:
 *
 * Returns: (transfer full): Valid actions for this context
 */
GActionGroup *
irc_server_get_action_group (void)
{
	GSimpleActionGroup *group = g_simple_action_group_new ();
	GAction *disconnect_action = irc_context_action_new ("disconnect",
									IRC_CONTEXT_ACTION_CALLBACK(irc_server_disconnect));
	GAction *connect_action = irc_context_action_new ("connect",
								IRC_CONTEXT_ACTION_CALLBACK(irc_server_connect));

	g_action_map_add_action (G_ACTION_MAP(group), disconnect_action);
	g_action_map_add_action (G_ACTION_MAP(group), connect_action);
	return G_ACTION_GROUP (group);
}

static GMenuModel *
irc_server_get_menu (IrcContext *self)
{
	GMenu *menu = g_menu_new ();
  	const char *id = irc_context_get_id (self);

	if (irc_server_get_is_connected (IRC_SERVER(self)))
	{
		g_autofree char *action = g_strdup_printf ("server.disconnect('%s')", id);
		g_menu_append (menu, _("Disconnect"), action);
	}
	else
	{
		g_autofree char *action = g_strdup_printf ("server.connect('%s')", id);
		g_menu_append (menu, _("Connect"), action);
	}

	return G_MENU_MODEL(menu);
}

static char *
irc_server_get_name (IrcContext *ctx)
{
	IrcServer *self = IRC_SERVER(ctx);
  	IrcServerPrivate *priv = irc_server_get_instance_private (self);

	return priv->network_name;
}

static void
irc_server_remove_child (IrcContext *ctx, IrcContext *child)
{
	IrcServer *self = IRC_SERVER(ctx);
  	IrcServerPrivate *priv = irc_server_get_instance_private (self);
	const char *name = irc_context_get_name (child);

	if (IRC_IS_CHANNEL(child))
	{
		irc_channel_part (IRC_CHANNEL(child));
		g_hash_table_remove (priv->chantable, name);
	}
	else if (IRC_IS_QUERY(child))
	{
		if (priv->caps & IRC_SERVER_SUPPORT_MONITOR)
		{
			irc_server_write_linef (self, "MONITOR - %s", name);
		}
		g_hash_table_remove (priv->querytable, name);
	}
}

/**
 * irc_server_new_from_network:
 * @network_name: Name of network to read from in settings
 *
 * Returns: (transfer full): A new #IrcServer instance. Free with g_object_unref()
 */
IrcServer *
irc_server_new_from_network (const char *network_name)
{
	g_autofree char *path = g_strconcat ("/se/tingping/IrcClient/", network_name, "/", NULL);
	g_autoptr(GSettings) settings = g_settings_new_with_path ("se.tingping.network", path);
	g_autofree char *host = g_settings_get_string (settings, "hostname");
	g_autofree char *encoding = g_settings_get_string (settings, "encoding");
	gboolean tls = g_settings_get_boolean (settings, "tls");

	return g_object_new (IRC_TYPE_SERVER, "host", host, "tls", tls,
								"encoding", encoding, "name", network_name, NULL);
}

static void
irc_server_finalize (GObject *object)
{
	IrcServer *self = IRC_SERVER(object);
	IrcServerPrivate *priv = irc_server_get_instance_private (self);

	irc_server_flushq (self);
	irc_server_disconnect (self);
	g_queue_free (priv->sendq);
	g_clear_object (&priv->socket);
	g_free (priv->host);
  	g_hash_table_unref (priv->chantable);
	g_hash_table_unref (priv->querytable);
  	g_hash_table_unref (priv->usertable); // channels reference users
  	g_clear_object (&priv->me);

	G_OBJECT_CLASS (irc_server_parent_class)->finalize (object);
}

enum {
	PROP_0,
	PROP_HOST,
	PROP_PORT,
	PROP_TLS,
  	PROP_NAME,
  	PROP_PARENT,
	PROP_CAPS,
	PROP_ENCODING,
	PROP_NICKPREFIXES,
	PROP_NICKMODES,
	PROP_CHANTYPES,
	PROP_CHANMODES,
	PROP_CONNECTED,
	PROP_ME,
	PROP_SOCKET,
	PROP_CONN,
  	N_PROPS,
};

static void
irc_server_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
	IrcServer *self = IRC_SERVER (object);
  	IrcServerPrivate *priv = irc_server_get_instance_private (self);

	switch (prop_id)
	{
	case PROP_NAME:
		g_value_set_string (value, priv->network_name);
		break;
	case PROP_HOST:
		g_value_set_string (value, priv->host);
		break;
	case PROP_TLS:
		g_value_set_boolean (value, g_socket_client_get_tls (priv->socket));
		break;
	case PROP_PORT:
		g_value_set_uint (value, priv->port);
		break;
	case PROP_PARENT:
		g_value_set_object (value, NULL);
		break;
	case PROP_CAPS:
		g_value_set_flags (value, priv->caps);
		break;
	case PROP_CHANTYPES:
		g_value_set_string (value, priv->chan_types);
		break;
	case PROP_CHANMODES:
		g_value_set_string (value, priv->chan_modes);
		break;
	case PROP_NICKPREFIXES:
		g_value_set_string (value, priv->nick_prefixes);
		break;
	case PROP_NICKMODES:
		g_value_set_string (value, priv->nick_modes);
		break;
	case PROP_ENCODING:
		g_value_set_string (value, priv->encoding);
		break;
	case PROP_CONNECTED:
		g_value_set_boolean (value, irc_server_get_is_connected (self));
		break;
	case PROP_ME:
		g_value_set_object (value, priv->me);
		break;
	case PROP_SOCKET:
		g_value_set_object (value, priv->socket);
		break;
	case PROP_CONN:
		g_value_set_object (value, priv->conn);
		break;
	default:
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
irc_server_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
	IrcServer *self = IRC_SERVER (object);
	IrcServerPrivate *priv = irc_server_get_instance_private (self);

	switch (prop_id)
	{
	case PROP_PARENT:
		break;
	case PROP_NAME:
		g_free (priv->network_name);
		priv->network_name = g_value_dup_string (value);
		break;
	case PROP_HOST:
		g_free (priv->host);
		priv->host = g_value_dup_string (value);
		break;
	case PROP_TLS:
		g_socket_client_set_tls (priv->socket, g_value_get_boolean (value));
		break;
	case PROP_PORT:
		priv->port = (guint16)g_value_get_uint (value);
		break;
	case PROP_CAPS:
		priv->caps = g_value_get_flags (value);
		break;
	case PROP_CHANTYPES:
		g_free (priv->chan_types);
		priv->chan_types = g_value_dup_string (value);
		break;
	case PROP_CHANMODES:
		g_free (priv->chan_modes);
		priv->chan_modes = g_value_dup_string (value);
		break;
	case PROP_NICKPREFIXES:
		g_free (priv->nick_prefixes);
		priv->nick_prefixes = g_value_dup_string (value);
		break;
	case PROP_NICKMODES:
		g_free (priv->nick_modes);
		priv->nick_modes = g_value_dup_string (value);
		break;
	case PROP_ENCODING:
		if (priv->encoding)
		{
			g_free (priv->encoding);
			g_iconv_close (priv->in_decoder);
			g_iconv_close (priv->out_encoder);
			priv->in_decoder = NULL;
			priv->out_encoder = NULL;
		}
		priv->encoding = g_value_dup_string (value);
		if (g_ascii_strcasecmp (priv->encoding, "UTF-8") != 0)
		{
			// TODO: Ensure valid encoding
			priv->in_decoder = g_iconv_open ("UTF-8", priv->encoding);
			priv->out_encoder = g_iconv_open (priv->encoding, "UTF-8");
		}
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
irc_server_class_init (IrcServerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = irc_server_finalize;
	object_class->get_property = irc_server_get_property;
	object_class->set_property = irc_server_set_property;
	klass->inbound_line = handle_incoming;

	g_object_class_override_property (object_class, PROP_PARENT, "parent");
	g_object_class_override_property (object_class, PROP_NAME, "name");
	g_object_class_override_property (object_class, PROP_CONNECTED, "active");
	g_object_class_install_property (object_class, PROP_PORT,
								  g_param_spec_uint ("port", _("Port"), _("Port to connect to"),
										0, G_MAXUINT16, 6697,
										G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
	g_object_class_install_property (object_class, PROP_HOST,
									g_param_spec_string ("host", _("Hostname"), _("Host to connect to"),
										NULL, G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
	g_object_class_install_property (object_class, PROP_TLS,
									g_param_spec_boolean ("tls", _("tls"), _("Use tls for a secure connection"),
										TRUE, G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
	g_object_class_install_property (object_class, PROP_CAPS,
									g_param_spec_flags ("caps", _("Capabilities"), _("Capabilities supported by the server"),
										IRC_TYPE_SERVER_CAP, IRC_SERVER_CAP_NONE, G_PARAM_READABLE));
	g_object_class_install_property (object_class, PROP_CHANTYPES,
									g_param_spec_string ("chantypes", _("ChanTypes"), _("Valid symbols for channels"),
										"#&", G_PARAM_READWRITE|G_PARAM_CONSTRUCT)); // RFC 1459 (RFC 2812 also had !+)
	g_object_class_install_property (object_class, PROP_CHANMODES,
									g_param_spec_string ("chanmodes", _("ChanModess"), _("Valid modes for channels"),
										"beI,k,l,imnpst", G_PARAM_READWRITE|G_PARAM_CONSTRUCT)); // RFC 1459
	g_object_class_install_property (object_class, PROP_NICKPREFIXES,
									g_param_spec_string ("nickprefixes", _("NickPrefixes"), _("Valid symbols for nicks"),
										"@+", G_PARAM_READWRITE|G_PARAM_CONSTRUCT)); // RFC 1459
	g_object_class_install_property (object_class, PROP_NICKMODES,
									g_param_spec_string ("nickmodes", _("NickModes"), _("Valid modes for nicks"),
										"ov", G_PARAM_READWRITE|G_PARAM_CONSTRUCT)); // RFC 1459
	g_object_class_install_property (object_class, PROP_ENCODING,
									g_param_spec_string ("encoding", _("Encoding"), _("Encoding for incoming and outgoing messages"),
										"UTF-8", G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
  	g_object_class_install_property (object_class, PROP_ME,
									g_param_spec_object ("me", _("Me"), _("Your user on the server"),
										IRC_TYPE_USER, G_PARAM_READABLE));
  	g_object_class_install_property (object_class, PROP_SOCKET,
									g_param_spec_object ("socket", _("Socket"), _("GSocketClient for the server"),
										G_TYPE_SOCKET_CLIENT, G_PARAM_READABLE));
  	g_object_class_install_property (object_class, PROP_CONN,
									g_param_spec_object ("connection", _("Connection"), _("GSocketConnection for the server"),
										G_TYPE_SOCKET_CONNECTION, G_PARAM_READABLE));
	// modes


	obj_signals[CONNECTED] = g_signal_new ("connected", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST,
										   0, NULL, NULL, NULL, G_TYPE_NONE, 0);

  	obj_signals[INBOUND] = g_signal_new ("inbound", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST|G_SIGNAL_ACTION|G_SIGNAL_NO_RECURSE,
										G_STRUCT_OFFSET(IrcServerClass, inbound_line),
										g_signal_accumulator_true_handled, NULL,
										irc_marshal_BOOLEAN__STRING, G_TYPE_BOOLEAN, 1, G_TYPE_STRING);
}

static void
irc_server_iface_init (IrcContextInterface *iface)
{
	iface->get_name = irc_server_get_name;
	iface->get_menu = irc_server_get_menu;
	iface->remove_child = irc_server_remove_child;
}

static void
irc_server_init (IrcServer *self)
{
	IrcServerPrivate *priv = irc_server_get_instance_private (self);

	priv->socket = g_socket_client_new ();
  	g_socket_client_set_timeout (priv->socket, 180);

	priv->usertable = g_hash_table_new_full ((GHashFunc)irc_str_hash, (GEqualFunc)irc_str_equal,
												NULL, NULL);

	priv->chantable = g_hash_table_new_full ((GHashFunc)irc_str_hash, (GEqualFunc)irc_str_equal,
												NULL, g_object_unref);

  	priv->querytable = g_hash_table_new_full ((GHashFunc)irc_str_hash, (GEqualFunc)irc_str_equal,
												NULL, g_object_unref);

	priv->sendq = g_queue_new ();
}
