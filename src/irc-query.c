/* irc-query.c
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

#include "irc-query.h"

struct _IrcQuery
{
	GObject parent_instance;
};

typedef struct
{
	IrcUser *user;
	IrcContext *parent;
	char *name;
	gboolean online;
} IrcQueryPrivate;

static void irc_query_iface_init (IrcContextInterface *);

G_DEFINE_TYPE_WITH_CODE (IrcQuery, irc_query, G_TYPE_OBJECT,
						G_IMPLEMENT_INTERFACE (IRC_TYPE_CONTEXT, irc_query_iface_init)
						G_ADD_PRIVATE (IrcQuery))

void
irc_query_set_online (IrcQuery *self, gboolean online)
{
	IrcQueryPrivate *priv = irc_query_get_instance_private (self);
	priv->online = online;
	if (!online)
		g_clear_object (&priv->user);
	g_object_notify (G_OBJECT(self), "active");
}

/**
 * irc_query_new:
 *
 * Returns: (transfer full): A new #IrcQuery instance. Free with g_object_unref()
 */
IrcQuery *
irc_query_new (IrcContext *parent, IrcUser *user)
{
	return g_object_new (IRC_TYPE_QUERY, "user", user, "parent", parent, "name", user->nick, NULL);
}

static void
irc_query_finalize (GObject *object)
{
	IrcQuery *self = IRC_QUERY(object);
	IrcQueryPrivate *priv = irc_query_get_instance_private (self);

	g_clear_object (&priv->user);
	g_clear_pointer (&priv->name, g_free);

	G_OBJECT_CLASS (irc_query_parent_class)->finalize (object);
}

enum
{
	PROP_0,
	PROP_USER,
	PROP_PARENT,
	PROP_NAME,
	PROP_ONLINE,
	N_PROPS
};

static void
irc_query_get_property (GObject *obj, guint prop_id, GValue *val, GParamSpec *pspec)
{
	IrcQuery *self = IRC_QUERY (obj);
	IrcQueryPrivate *priv = irc_query_get_instance_private (self);

	switch (prop_id)
	{
	case PROP_USER:
		g_value_set_object (val, priv->user);
		break;
	case PROP_PARENT:
		g_value_set_object (val, priv->parent);
		break;
	case PROP_NAME:
		g_value_set_string (val, priv->name);
		break;
	case PROP_ONLINE:
		g_value_set_boolean (val, priv->online);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
irc_query_set_property (GObject *obj, guint prop_id, const GValue *val, GParamSpec *pspec)
{
	IrcQuery *self = IRC_QUERY (obj);
	IrcQueryPrivate *priv = irc_query_get_instance_private (self);

	switch (prop_id)
	{
	case PROP_USER:
		g_clear_object (&priv->user);
		priv->user = g_value_dup_object (val);
		break;
	case PROP_PARENT:
		priv->parent = g_value_get_object (val);
		break;
	case PROP_NAME:
		g_free (priv->name);
		priv->name = g_value_dup_string (val);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

IrcUser *
irc_query_get_user (IrcQuery *self)
{
	IrcQueryPrivate *priv = irc_query_get_instance_private (self);

	return priv->user;
}

static IrcContext *
irc_query_iface_get_parent (IrcContext *ctx)
{
	IrcQuery *self = IRC_QUERY (ctx);
	IrcQueryPrivate *priv = irc_query_get_instance_private (self);
	return priv->parent;
}

static char *
irc_query_iface_get_name (IrcContext *ctx)
{
	IrcQuery *self = IRC_QUERY (ctx);
  	IrcQueryPrivate *priv = irc_query_get_instance_private (self);
	return priv->name;
}

static void
irc_query_class_init (IrcQueryClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = irc_query_finalize;
	object_class->get_property = irc_query_get_property;
	object_class->set_property = irc_query_set_property;

	g_object_class_override_property (object_class, PROP_NAME, "name");
  	g_object_class_override_property (object_class, PROP_PARENT, "parent");
	g_object_class_override_property (object_class, PROP_ONLINE, "active");
	g_object_class_install_property (object_class, PROP_USER,
									g_param_spec_object ( "user", "User", "User of Query",
											IRC_TYPE_USER, G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
}

static void
irc_query_iface_init (IrcContextInterface *iface)
{
	iface->get_parent = irc_query_iface_get_parent;
	iface->get_name = irc_query_iface_get_name;
	//iface->get_menu = irc_channel_get_menu;
}

static void
irc_query_init (IrcQuery *self)
{
}
