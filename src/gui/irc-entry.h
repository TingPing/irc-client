/* irc-entry.h
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

#pragma once

#include "irc-textview.h"
#include "irc-utils.h"

G_BEGIN_DECLS

#define IRC_TYPE_ENTRY (irc_entry_get_type())

G_DECLARE_FINAL_TYPE (IrcEntry, irc_entry, IRC, ENTRY, IrcTextview)

IrcEntry *irc_entry_new (void);
void irc_entry_set_completion_model (IrcEntry *self, GListModel *model) NON_NULL();

G_END_DECLS

