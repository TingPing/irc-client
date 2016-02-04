/* irc-userlist.h
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

#ifndef IRC_USERLIST_H
#define IRC_USERLIST_H

#include <gtk/gtk.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define IRC_TYPE_USERLIST (irc_userlist_get_type())

G_DECLARE_FINAL_TYPE (IrcUserlist, irc_userlist, IRC, USERLIST, GtkPopover)

IrcUserlist *irc_userlist_new (GListModel *model);

G_END_DECLS

#endif /* IRC_USERLIST_H */
