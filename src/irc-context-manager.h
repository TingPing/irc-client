/* irc-context-manager.h
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
#include "irc-context.h"
#include "irc-utils.h"

G_BEGIN_DECLS

#define IRC_TYPE_CONTEXT_MANAGER (irc_context_manager_get_type())
G_DECLARE_FINAL_TYPE (IrcContextManager, irc_context_manager, IRC, CONTEXT_MANAGER, GObject)

IrcContextManager *irc_context_manager_get_default (void) RETURNS_NON_NULL;
void irc_context_manager_add (IrcContextManager *self, IrcContext *ctx) NON_NULL();
IrcContext *irc_context_manager_find (IrcContextManager *self, const char *id) NON_NULL();
void irc_context_manager_remove (IrcContextManager *self, IrcContext *ctx) NON_NULL();
void irc_context_manager_remove_by_id (IrcContextManager *self, const char *id) NON_NULL();
void irc_context_manager_set_front_context (IrcContextManager *self, IrcContext *front) NON_NULL();
IrcContext *irc_context_manager_get_front_context (IrcContextManager *self) NON_NULL();
void irc_context_manager_foreach_parent (IrcContextManager *self, GNodeForeachFunc func, gpointer data) NON_NULL();

G_END_DECLS

