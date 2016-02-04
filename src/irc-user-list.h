/* irc-user-list.h
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

#include <gio/gio.h>
#include "irc-user-list-item.h"

G_BEGIN_DECLS

#define IRC_TYPE_USER_LIST (irc_user_list_get_type())
G_DECLARE_FINAL_TYPE (IrcUserList, irc_user_list, IRC, USER_LIST, GObject)

IrcUserList *irc_user_list_new (void);
void irc_user_list_add (IrcUserList *list, IrcUser *user, const char *prefix) NON_NULL(1,2);
void irc_user_list_clear (IrcUserList *list) NON_NULL();
gboolean irc_user_list_remove (IrcUserList *list, IrcUser *user) NON_NULL();
gboolean irc_user_list_contains (IrcUserList *list, IrcUser *user) NON_NULL();
const char *irc_user_list_get_users_prefix (IrcUserList *list, IrcUser *user) NON_NULL();
void irc_user_list_set_users_prefix (IrcUserList *list, IrcUser *user, const char *prefix) NON_NULL(1,2);

G_END_DECLS
