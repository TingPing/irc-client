/* irc-user-list.c
 *
 * Copyright (C) 2016 Patrick Griffis <tingping@tingping.se>
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

#include "irc-user-list.h"

struct _IrcUserList
{
	GObject parent_instance;
};

typedef struct
{
	GSequence *users;

	/* cache */
	guint last_position;
	GSequenceIter *last_iter;
} IrcUserListPrivate;

static void irc_user_list_iface_init (GListModelInterface *iface);

G_DEFINE_TYPE_WITH_CODE (IrcUserList, irc_user_list, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, irc_user_list_iface_init)
						 G_ADD_PRIVATE (IrcUserList))

IrcUserList *
irc_user_list_new (void)
{
	return g_object_new (IRC_TYPE_USER_LIST, NULL);
}

static GSequenceIter *
get_iter_by_user (GSequence *seq, IrcUser *user)
{
	g_return_val_if_fail (user != NULL, NULL);

	GSequenceIter *it = g_sequence_get_begin_iter (seq);
	while (!g_sequence_iter_is_end (it))
	{
		IrcUserListItem *item = g_sequence_get (it);
		if (item->user == user)
			return it;

		it = g_sequence_iter_next (it);
	}

	return NULL;
}

const char *
irc_user_list_get_users_prefix (IrcUserList *self, IrcUser *user)
{
	IrcUserListPrivate *priv = irc_user_list_get_instance_private (self);
	GSequenceIter *it = get_iter_by_user (priv->users, user);
	if (it)
		return IRC_USER_LIST_ITEM(g_sequence_get (it))->prefix;

	return NULL;
}

void
irc_user_list_set_users_prefix (IrcUserList *self, IrcUser *user, const char *prefix)
{
	IrcUserListPrivate *priv = irc_user_list_get_instance_private (self);
	GSequenceIter *it = get_iter_by_user (priv->users, user);
	if (!it)
		return;

	IrcUserListItem *item = g_sequence_get (it);
	g_object_set (item, "prefix", prefix, NULL);
}

static int
irc_user_compare_func (IrcUserListItem *i1, IrcUserListItem *i2, gpointer user_data)
{
	return irc_str_cmp (i1->user->nick, i2->user->nick);
}

static void
irc_user_list_items_changed (IrcUserList *self, guint position, guint removed, guint added)
{
	IrcUserListPrivate *priv = irc_user_list_get_instance_private (self);

	/* check if the iter cache may have been invalidated */
	if (position <= priv->last_position)
	{
		priv->last_iter = NULL;
		priv->last_position = -1u;
	}

	g_list_model_items_changed (G_LIST_MODEL (self), position, removed, added);
}

static void
on_nick_changed (IrcUser *user, GParamSpec *pspec, gpointer data)
{
	IrcUserList *self = IRC_USER_LIST(data);
	IrcUserListPrivate *priv = irc_user_list_get_instance_private (self);
	GSequenceIter *it = get_iter_by_user (priv->users, user);
	if (it == NULL)
	{
		g_warning ("Got notify::nick signal from user not in channel user list");
		return;
	}

	// We can't just resort this locally since we need the signals to be sync'd
	// up to anybody listening
	g_autoptr(IrcUserListItem) item = g_object_ref (g_sequence_get (it));
	guint position = (guint)g_sequence_iter_get_position (it);
	g_sequence_remove (it);
	irc_user_list_items_changed (self, position, 1, 0);
	irc_user_list_add (self, item->user, item->prefix);
}

void
irc_user_list_add (IrcUserList *self, IrcUser *user, const char *prefix)
{
	IrcUserListPrivate *priv = irc_user_list_get_instance_private (self);
	IrcUserListItem *item = irc_user_list_item_new (user, prefix);

	GSequenceIter *it = g_sequence_insert_sorted (priv->users, g_object_ref (item),
                                    (GCompareDataFunc)irc_user_compare_func, NULL);
	g_assert (it != NULL);
	guint position = (guint)g_sequence_iter_get_position (it);

	g_signal_connect (user, "notify::nick", G_CALLBACK(on_nick_changed), self);
	irc_user_list_items_changed (self, position, 0, 1);
}

void
irc_user_list_clear (IrcUserList *self)
{
	IrcUserListPrivate *priv = irc_user_list_get_instance_private (self);
	GSequenceIter *begin, *end;

	begin = g_sequence_get_begin_iter (priv->users);
	end = g_sequence_get_end_iter (priv->users);
	if (g_sequence_iter_compare (begin, end))
		return;

	guint len = (guint)g_sequence_get_length(priv->users);
	g_sequence_remove_range (begin, end);

	irc_user_list_items_changed (self, 0, len, 0);
}

gboolean
irc_user_list_remove (IrcUserList *self, IrcUser *user)
{
	IrcUserListPrivate *priv = irc_user_list_get_instance_private (self);
	GSequenceIter *it = get_iter_by_user (priv->users, user);
	if (it)
	{
		guint position = (guint)g_sequence_iter_get_position (it);
		g_sequence_remove (it);

		irc_user_list_items_changed (self, position, 1, 0);
		return TRUE;
	}

	return FALSE;
}

gboolean
irc_user_list_contains (IrcUserList *self, IrcUser *user)
{
	IrcUserListPrivate *priv = irc_user_list_get_instance_private (self);
	GSequenceIter *it = get_iter_by_user (priv->users, user);
	return (it != NULL);
}

static gpointer
irc_user_list_get_item (GListModel *list, guint position)
{
	IrcUserList *self = IRC_USER_LIST (list);
	IrcUserListPrivate *priv = irc_user_list_get_instance_private (self);
	GSequenceIter *it = NULL;

	// Taken from gliststore.c because it is a final type...
	if (priv->last_position != -1u)
	{
		if (priv->last_position == position + 1)
			it = g_sequence_iter_prev (priv->last_iter);
		else if (priv->last_position == position - 1)
			it = g_sequence_iter_next (priv->last_iter);
		else if (priv->last_position == position)
			it = priv->last_iter;
	}

	if (it == NULL)
	{
		g_assert (position < G_MAXINT);
		it = g_sequence_get_iter_at_pos (priv->users, (gint)position);
	}

	priv->last_iter = it;
	priv->last_position = position;

	if (g_sequence_iter_is_end (it))
		return NULL;
	else
		return g_object_ref (g_sequence_get (it));
}

static guint
irc_user_list_get_n_items (GListModel *list)
{
	IrcUserList *self = IRC_USER_LIST (list);
	IrcUserListPrivate *priv = irc_user_list_get_instance_private (self);

	return (guint)g_sequence_get_length (priv->users);
}

static GType irc_user_list_get_item_type (GListModel *list) G_GNUC_CONST;

static GType
irc_user_list_get_item_type (GListModel *list)
{
	return IRC_TYPE_USER_LIST_ITEM;
}

static void
irc_user_list_finalize (GObject *object)
{
	IrcUserList *self = IRC_USER_LIST(object);
	IrcUserListPrivate *priv = irc_user_list_get_instance_private (self);

	g_clear_pointer (&priv->users, g_sequence_free);

	G_OBJECT_CLASS (irc_user_list_parent_class)->finalize (object);
}

static void
irc_user_list_class_init (IrcUserListClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = irc_user_list_finalize;
}

static void
irc_user_list_iface_init (GListModelInterface *iface)
{
	iface->get_item_type = irc_user_list_get_item_type;
	iface->get_item = irc_user_list_get_item;
	iface->get_n_items = irc_user_list_get_n_items;
}

static void
irc_user_list_init (IrcUserList *self)
{
	IrcUserListPrivate *priv = irc_user_list_get_instance_private (self);

	priv->users = g_sequence_new (g_object_unref);
	priv->last_position = -1u;
}
