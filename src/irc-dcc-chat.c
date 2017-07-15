/* irc-dcc-chat.c
 *
 * Copyright (C) 2017 Patrick Griffis <tingping@tingping.se>
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

#include "irc-dcc-chat.h"

struct _IrcDccChat
{
	GObject parent_instance;
};

typedef struct
{
	IrcServer *server;
	GSocketService *service;
	char *name;
} IrcDccChatPrivate;

static void irc_dcc_chat_iface_init (IrcContextInterface *);

G_DEFINE_TYPE_WITH_CODE (IrcDccChat, irc_dcc_chat, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (IRC_TYPE_CONTEXT, irc_dcc_chat_iface_init)
						 G_ADD_PRIVATE (IrcDccChat))

enum {
	PROP_0,
	PROP_PARENT,
	PROP_NAME,
	PROP_SERVER,
	N_PROPS
};

IrcDccChat *
irc_dcc_chat_new (IrcServer *server, const char *nick)
{
	return g_object_new (IRC_TYPE_DCC_CHAT, "name", nick, "server", server, NULL);
}

static void
irc_dcc_chat_finalize (GObject *object)
{
	IrcDccChat *self = (IrcDccChat *)object;
	IrcDccChatPrivate *priv = irc_dcc_chat_get_instance_private (self);

	g_clear_pointer (&priv->name, g_free);
	g_clear_object (&priv->server);

	G_OBJECT_CLASS (irc_dcc_chat_parent_class)->finalize (object);
}

static char *
irc_dcc_chat_iface_get_name (IrcContext *ctx)
{
	IrcDccChat *self = IRC_DCC_CHAT (ctx);
	IrcDccChatPrivate *priv = irc_dcc_chat_get_instance_private (self);
	return priv->name;
}

static void
irc_dcc_chat_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
	IrcDccChat *self = IRC_DCC_CHAT (object);
	IrcDccChatPrivate *priv = irc_dcc_chat_get_instance_private (self);

	switch (prop_id)
	{
	case PROP_SERVER:
		g_value_set_object (value, priv->server);
		break;
	case PROP_PARENT:
		g_value_set_object (value, NULL);
		break;
	case PROP_NAME:
		g_value_set_string (value, priv->name);
		break;
	default:
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
irc_dcc_chat_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
	IrcDccChat *self = IRC_DCC_CHAT (object);
	IrcDccChatPrivate *priv = irc_dcc_chat_get_instance_private (self);

	switch (prop_id)
	{
	case PROP_SERVER:
		g_clear_object (&priv->server);
		priv->server = g_value_dup_object (value);
		break;
	case PROP_PARENT:
		break;
	case PROP_NAME:
		g_free (priv->name);
		priv->name = g_value_dup_string (value);
		break;
	default:
	    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
irc_dcc_chat_class_init (IrcDccChatClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = irc_dcc_chat_finalize;
	object_class->get_property = irc_dcc_chat_get_property;
	object_class->set_property = irc_dcc_chat_set_property;


	g_object_class_override_property (object_class, PROP_PARENT, "parent");
	g_object_class_override_property (object_class, PROP_NAME, "name");
	//g_object_class_override_property (object_class, PROP_ONLINE, "active");
	g_object_class_install_property (object_class, PROP_SERVER,
								g_param_spec_object ( "server", "Server", "Server chat was initiated from",
										IRC_TYPE_SERVER, G_PARAM_WRITABLE|G_PARAM_CONSTRUCT|G_PARAM_CONSTRUCT_ONLY));
}

static void
irc_dcc_chat_iface_init (IrcContextInterface *iface)
{
	//iface->get_parent = irc_dcc_chat_iface_get_parent;
	iface->get_name = irc_dcc_chat_iface_get_name;
	//iface->get_menu = irc_channel_get_menu;
}

static void
irc_dcc_chat_init (IrcDccChat *self)
{
}
