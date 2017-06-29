/* irc-user-list-item.c
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

#include <glib/gi18n.h>
#include "irc-user-list-item.h"

enum {
	PROP_0,
	PROP_PREFIX,
	PROP_USER,
	N_PROPS
};

G_DEFINE_TYPE (IrcUserListItem, irc_user_list_item, G_TYPE_OBJECT)

IrcUserListItem *
irc_user_list_item_new (IrcUser *user, const char *prefix)
{
	return g_object_new (IRC_TYPE_USER_LIST_ITEM, "user", user, "prefix", prefix, NULL);
}

static void
irc_user_list_item_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
	IrcUserListItem *self = IRC_USER_LIST_ITEM (object);

	switch (prop_id)
	{
	case PROP_PREFIX:
		g_value_set_string (value, self->prefix);
		break;
	case PROP_USER:
		g_value_set_object (value, self->user);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
irc_user_list_item_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
	IrcUserListItem *self = IRC_USER_LIST_ITEM (object);

	switch (prop_id)
	{
	case PROP_PREFIX:
		g_free (self->prefix);
		self->prefix = g_value_dup_string (value);
		break;
	case PROP_USER:
		self->user = g_value_dup_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
irc_user_list_item_finalize (GObject *object)
{
	IrcUserListItem *self = IRC_USER_LIST_ITEM (object);

	g_clear_object (&self->user);
	g_clear_pointer (&self->prefix, g_free);

	G_OBJECT_CLASS (irc_user_list_item_parent_class)->finalize (object);
}

static void
irc_user_list_item_class_init (IrcUserListItemClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = irc_user_list_item_finalize;
	object_class->get_property = irc_user_list_item_get_property;
	object_class->set_property = irc_user_list_item_set_property;

	g_object_class_install_property (object_class, PROP_PREFIX,
									g_param_spec_string ("prefix", _("Prefix"), _("Prefix of user"),
										NULL, G_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_STATIC_STRINGS));

  	g_object_class_install_property (object_class, PROP_USER,
									g_param_spec_object ("user", _("User"), _("A User"),
										IRC_TYPE_USER, G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY|G_PARAM_STATIC_STRINGS));
}

static void
irc_user_list_item_init (IrcUserListItem *self)
{
}
