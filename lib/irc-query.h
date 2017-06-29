/* irc-query.h
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

#pragma once

#include <glib-object.h>
#include "irc-user.h"
#include "irc-context.h"
#include "irc-utils.h"

G_BEGIN_DECLS

#define IRC_TYPE_QUERY (irc_query_get_type())
G_DECLARE_FINAL_TYPE (IrcQuery, irc_query, IRC, QUERY, GObject)

IrcQuery *irc_query_new (IrcContext *parent, IrcUser *user) NON_NULL();
void irc_query_set_online (IrcQuery *self, gboolean online) NON_NULL(1);

G_END_DECLS
