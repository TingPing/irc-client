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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib/gi18n.h>
#include <gio/gio.h>
#include "irc-user.h"
#include "irc-context.h"
#include "irc-channel.h"
#include "irc-server.h"
#include "irc-context-action.h"

typedef struct
{
	IrcContext *parent;
	GtkListStore *userlist;
	char *topic;
	gboolean joined;
} IrcChannelPrivate;

static void irc_channel_iface_init (IrcContextInterface *iface);

G_DEFINE_TYPE_WITH_CODE (IrcChannel, irc_channel, G_TYPE_OBJECT,
						G_IMPLEMENT_INTERFACE (IRC_TYPE_CONTEXT, irc_channel_iface_init)
						G_ADD_PRIVATE (IrcChannel))

enum {
	COL_NICK,
	COL_USER,
	N_COL,
};

enum {
	PROP_0,
	PROP_NAME,
	PROP_PARENT,
	PROP_TOPIC,
	PROP_JOINED,
	N_PROPS
};

/**
 * irc_channel_set_joined:
 * @self:
 * @joined: If the channel is joined
 */
void
irc_channel_set_joined (IrcChannel *self, gboolean joined)
{
  	IrcChannelPrivate *priv = irc_channel_get_instance_private (self);
	priv->joined = joined;

	if (!joined)
	{
		gtk_list_store_clear (priv->userlist);
		g_clear_pointer (&priv->topic, g_free);
	}
	g_object_notify (G_OBJECT(self), "active");
}

/**
 * irc_channel_get_users:
 *
 * Returns: (transfer none): Userlist
 */
GtkListStore *
irc_channel_get_users (IrcChannel *self)
{
	IrcChannelPrivate *priv = irc_channel_get_instance_private (self);
	return priv->userlist;
}

/**
 * irc_channel_part:
 */
void
irc_channel_part (IrcChannel *self)
{
	g_return_if_fail (IRC_IS_CHANNEL(self));

	IrcServer *server = IRC_SERVER(irc_context_get_parent (IRC_CONTEXT(self)));
  	irc_server_write_linef (server, "PART %s", self->name);
}

/**
 * irc_channel_refresh_user:
 */
gboolean
irc_channel_refresh_user (IrcChannel *self, IrcUser *user)
{
	IrcChannelPrivate *priv = irc_channel_get_instance_private (self);
	GtkTreeModel *model = GTK_TREE_MODEL(priv->userlist);
	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter_first (model, &iter))
		return FALSE;

	do {
		g_autoptr(IrcUser) this_user;
		gtk_tree_model_get (model, &iter, COL_USER, &this_user, -1);
		if (this_user == user)
		{
			gtk_list_store_set (priv->userlist, &iter, COL_NICK, user->nick, -1);
			return TRUE;
		}
	} while (gtk_tree_model_iter_next (model, &iter));

	return FALSE;
}

/**
 * irc_channel_remove_user:
 *
 * Returns: %TRUE if found and removed else %FALSE
 */
gboolean
irc_channel_remove_user (IrcChannel *self, IrcUser *user)
{
	IrcChannelPrivate *priv = irc_channel_get_instance_private (self);
	GtkTreeModel *model = GTK_TREE_MODEL(priv->userlist);
	GtkTreeIter iter;

	if (!gtk_tree_model_get_iter_first (model, &iter))
		return FALSE;

	do {
		g_autoptr(IrcUser) this_user;
		gtk_tree_model_get (model, &iter, COL_USER, &this_user, -1);
		if (this_user == user)
		{
			gtk_list_store_remove (priv->userlist, &iter);
			return TRUE;
		}
	} while (gtk_tree_model_iter_next (model, &iter));

	return FALSE;
}

/**
 * irc_channel_add_users:
 * @users: (element-type IrcUser): List of users
 */
void
irc_channel_add_users (IrcChannel *self, GPtrArray *users)
{
	IrcChannelPrivate *priv = irc_channel_get_instance_private (self);

	// Only trigger the resort once
	// Ideally we could use gtk_list_store_insert_with_values() and avoid
	// mutliple row_inserted signals
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(priv->userlist),
										GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, GTK_SORT_ASCENDING);
	for (uint i = 0; i < users->len; ++i)
	{
		irc_channel_add_user (self, users->pdata[i]);
	}
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(priv->userlist), COL_USER, GTK_SORT_ASCENDING);
}

/**
 * irc_channel_add_user:
 */
void
irc_channel_add_user (IrcChannel *self, IrcUser *user)
{
	IrcChannelPrivate *priv = irc_channel_get_instance_private (self);

	gtk_list_store_insert_with_values (priv->userlist, NULL, -1,
									COL_NICK, user->nick, COL_USER, user, -1);
}

static int
irc_channel_sort_nicks (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer data)
{
	g_autoptr(IrcUser) user1;
	g_autoptr(IrcUser) user2;

	gtk_tree_model_get (model, a, COL_USER, &user1, -1);
	gtk_tree_model_get (model, b, COL_USER, &user2, -1);

	return irc_str_cmp (user1->nick, user2->nick);
}

static GMenuModel *
irc_channel_get_menu (IrcContext *self)
{
	GMenu *menu = g_menu_new ();
  	const char *id = irc_context_get_id (self);
	g_autofree char *action = g_strdup_printf ("channel.part('%s')", id);
	g_menu_append (menu, _("Part"), action);

	return G_MENU_MODEL(menu);
}

/**
 * irc_channel_get_action_group:
 *
 * Returns: (transfer full): Valid actions for this context
 */
GActionGroup *
irc_channel_get_action_group (void)
{
	GSimpleActionGroup *group = g_simple_action_group_new ();
	GAction *part_action = irc_context_action_new ("part",
								IRC_CONTEXT_ACTION_CALLBACK(irc_channel_part));

	g_action_map_add_action (G_ACTION_MAP(group), part_action);
	return G_ACTION_GROUP (group);
}

/**
 * irc_channel_new:
 * @parent: Parent #IrcContext of this channel
 * @name: Name of the channel
 *
 * Returns: (transfer full): A new #IrcChannel instance. Free with g_object_unref()
 */
IrcChannel *
irc_channel_new (IrcContext *parent, const char *name)
{
	return g_object_new (IRC_TYPE_CHANNEL, "name", name, "parent", parent, NULL);
}

static void
irc_channel_finalize (GObject *object)
{
	IrcChannel *self = IRC_CHANNEL(object);
	IrcChannelPrivate *priv = irc_channel_get_instance_private (self);

	g_free (self->name);
	g_free (priv->topic);
	g_object_unref (priv->userlist);

	G_OBJECT_CLASS (irc_channel_parent_class)->finalize (object);
}

static void
irc_channel_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
	IrcChannel *self = IRC_CHANNEL (object);
	IrcChannelPrivate *priv = irc_channel_get_instance_private (self);

	switch (prop_id)
	{
	case PROP_NAME:
		g_value_set_string (value, self->name);
		break;
	case PROP_PARENT:
		g_value_set_object (value, priv->parent);
		break;
	case PROP_TOPIC:
		g_value_set_string (value, priv->topic);
		break;
	case PROP_JOINED:
		g_value_set_boolean (value, priv->joined);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
irc_channel_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
	IrcChannel *self = IRC_CHANNEL (object);
	IrcChannelPrivate *priv = irc_channel_get_instance_private (self);

	switch (prop_id)
	{
	case PROP_NAME:
		g_free (self->name);
		self->name = g_value_dup_string (value);
		break;
	case PROP_PARENT:
		priv->parent = g_value_get_object (value);
		break;
	case PROP_TOPIC:
		priv->topic = g_value_dup_string (value);
		break;
	case PROP_JOINED:
		priv->joined = g_value_get_boolean (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
irc_channel_class_init (IrcChannelClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = irc_channel_finalize;
	object_class->get_property = irc_channel_get_property;
	object_class->set_property = irc_channel_set_property;

	g_object_class_override_property (object_class, PROP_NAME, "name");
	g_object_class_override_property (object_class, PROP_PARENT, "parent");
  	g_object_class_override_property (object_class, PROP_JOINED, "active");

	g_object_class_install_property (object_class, PROP_TOPIC,
								  g_param_spec_string ("topic", "Topic", "Topic of channel",
										NULL, G_PARAM_READWRITE));
}

static IrcContext *
irc_channel_iface_get_parent (IrcContext *ctx)
{
  	IrcChannel *self = IRC_CHANNEL (ctx);
  	IrcChannelPrivate *priv = irc_channel_get_instance_private (self);
	return priv->parent;
}

static char *
irc_channel_iface_get_name (IrcContext *ctx)
{
  	IrcChannel *self = IRC_CHANNEL (ctx);
	return self->name;
}

static void
irc_channel_iface_init (IrcContextInterface *iface)
{
	iface->get_parent = irc_channel_iface_get_parent;
	iface->get_name = irc_channel_iface_get_name;
	iface->get_menu = irc_channel_get_menu;
}

static void
irc_channel_init (IrcChannel *self)
{
	IrcChannelPrivate *priv = irc_channel_get_instance_private (self);

	priv->joined = TRUE;

	priv->userlist = gtk_list_store_new (N_COL, G_TYPE_STRING, IRC_TYPE_USER);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(priv->userlist), COL_USER, GTK_SORT_ASCENDING);
	gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE(priv->userlist), COL_USER, irc_channel_sort_nicks, NULL, NULL);
}
