/* irc-user-list-item.h
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

#pragma once

#include <glib-object.h>
#include "irc-utils.h"
#include "irc-user.h"

G_BEGIN_DECLS

#define IRC_TYPE_USER_LIST_ITEM (irc_user_list_item_get_type())
G_DECLARE_FINAL_TYPE (IrcUserListItem, irc_user_list_item, IRC, USER_LIST_ITEM, GObject)

struct _IrcUserListItem
{
	GObject parent_instance;

	char *prefix;
	IrcUser *user;
};

IrcUserListItem *irc_user_list_item_new (IrcUser *user, const char *prefix) NON_NULL(1);

G_END_DECLS
