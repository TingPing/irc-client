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

#include <glib/gi18n.h>
#include "irc-context.h"
#include "irc-server.h"
#include "irc-user.h"

typedef struct
{
	IrcServer *server;
	char *away_reason;
	gboolean away;
} IrcUserPrivate;

enum
{
	PROP_0,
	PROP_NICK,
	PROP_REAL,
	PROP_HOST,
	PROP_USER,
	PROP_ACCOUNT,
	PROP_AWAY,
	PROP_AWAY_REASON,
	N_PROPS
};

static GParamSpec *obj_props[N_PROPS];

G_DEFINE_TYPE_WITH_PRIVATE (IrcUser, irc_user, G_TYPE_OBJECT)

static void
split_user_details (const char *userhost, char **nick, char **username, char **hostname)
{
	gsize len;
	char *p;
	p = strchr (userhost, '!');
	if (p == NULL)
	{
		*nick = g_strdup (userhost);
		return;
	}

	len = (gsize)(p - userhost);
	*nick = g_strndup (userhost, len);

	userhost += len + 1;

	p = strchr (userhost, '@');
	if (p == NULL)
		return;
 	len = (gsize)(p - userhost);
	*username = g_strndup (userhost, len);

	userhost += len + 1;
	*hostname = g_strdup (userhost);
}

/**
 * irc_user_new:
 *
 * Returns: (transfer full): A new #IrcUser instance. Free with g_object_unref()
 */
IrcUser *
irc_user_new (const char *userhost)
{
	g_autofree char *nick;
	g_autofree char *username = NULL;
	g_autofree char *hostname = NULL;

	split_user_details (userhost, &nick, &username, &hostname);
	g_assert (nick != NULL);
	return g_object_new (IRC_TYPE_USER, "nick", nick,
										"hostname", hostname,
										"username", username, NULL);
}

static void
irc_user_get_property (GObject *obj, guint prop_id, GValue *val, GParamSpec *pspec)
{
	IrcUser *self = IRC_USER (obj);
	IrcUserPrivate *priv = irc_user_get_instance_private (self);

	switch (prop_id)
	{
	case PROP_NICK:
		g_value_set_string (val, self->nick);
		break;
	case PROP_REAL:
		g_value_set_string (val, self->realname);
		break;
	case PROP_HOST:
		g_value_set_string (val, self->hostname);
		break;
	case PROP_USER:
		g_value_set_string (val, self->username);
		break;
	case PROP_ACCOUNT:
		g_value_set_string (val, self->account);
		break;
	case PROP_AWAY:
		g_value_set_boolean (val, priv->away);
		break;
	case PROP_AWAY_REASON:
		g_value_set_string (val, priv->away_reason);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
irc_user_set_property (GObject *obj, guint prop_id, const GValue *val, GParamSpec *pspec)
{
	IrcUser *self = IRC_USER (obj);
	IrcUserPrivate *priv = irc_user_get_instance_private (self);

	switch (prop_id)
	{
	case PROP_NICK:
		g_free (self->nick);
		self->nick = g_value_dup_string (val);
		break;
	case PROP_USER:
		g_free (self->username);
		self->username = g_value_dup_string (val);
		break;
	case PROP_HOST:
		g_free (self->hostname);
		self->hostname = g_value_dup_string (val);
		break;
	case PROP_ACCOUNT:
		g_free (self->account);
		self->account = g_value_dup_string (val);
		break;
	case PROP_REAL:
		g_free (self->realname);
		self->realname = g_value_dup_string (val);
		break;
	case PROP_AWAY:
		priv->away = g_value_get_boolean (val);
		break;
	case PROP_AWAY_REASON:
		g_free (priv->away_reason);
		priv->away_reason = g_value_dup_string (val);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
irc_user_class_init (IrcUserClass *cls)
{
	GObjectClass *object_class = G_OBJECT_CLASS(cls);

	object_class->get_property = irc_user_get_property;
	object_class->set_property = irc_user_set_property;

  	obj_props[PROP_REAL] = g_param_spec_string ("realname", _("Real name"), _("Real name of user"),
							NULL, G_PARAM_READWRITE|G_PARAM_CONSTRUCT);

  	obj_props[PROP_HOST] = g_param_spec_string ("hostname", _("Hostname"), _("Hostname of user"),
							NULL, G_PARAM_READWRITE|G_PARAM_CONSTRUCT);

  	obj_props[PROP_NICK] = g_param_spec_string ("nick", _("Nickname"), _("Nickname of user"),
							NULL, G_PARAM_READWRITE|G_PARAM_CONSTRUCT);

	obj_props[PROP_USER] = g_param_spec_string ("username", _("Username"), _("Username of user"),
							NULL, G_PARAM_READWRITE|G_PARAM_CONSTRUCT);

	/**
	 * IrcUser:account:
	 * Users account name or %NULL
	 */
	obj_props[PROP_ACCOUNT] = g_param_spec_string ("account", _("Account"), _("Account of user"),
							NULL, G_PARAM_READWRITE|G_PARAM_CONSTRUCT);
	obj_props[PROP_AWAY_REASON] = g_param_spec_string ("away-reason", _("Away Reason"), _("Away reason of user"),
							NULL, G_PARAM_READWRITE|G_PARAM_CONSTRUCT);
	obj_props[PROP_AWAY] = g_param_spec_boolean ("away", _("Away"), _("User is away"),
							FALSE, G_PARAM_READWRITE|G_PARAM_CONSTRUCT);

	g_object_class_install_properties (object_class, N_PROPS, obj_props);
}

static void
irc_user_init (IrcUser *user)
{

}
