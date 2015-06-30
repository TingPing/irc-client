/* context.h
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

#include <gio/gio.h>
#include "irc-utils.h"

G_BEGIN_DECLS

#define IRC_TYPE_CONTEXT (irc_context_get_type())
G_DECLARE_INTERFACE (IrcContext, irc_context, IRC, CONTEXT, GObject)

struct _IrcContextInterface
{
	GTypeInterface parent;

	char *(*get_name) (IrcContext *self);
	IrcContext *(*get_parent) (IrcContext *self);
	GMenuModel *(*get_menu) (IrcContext *self);
	void (*remove_child) (IrcContext *self, IrcContext *child);
};

void irc_context_run_command (IrcContext *self, const char *command) NON_NULL();
void irc_context_print (IrcContext *self, const char *message) NON_NULL();
void irc_context_print_with_time (IrcContext *self, const char *message, time_t stamp);
const char *irc_context_get_id (IrcContext *self) NON_NULL() RETURNS_NON_NULL;
const char *irc_context_get_name (IrcContext *self) RETURNS_NON_NULL;
IrcContext *irc_context_get_parent (IrcContext *self) NON_NULL();
GMenuModel *irc_context_get_menu (IrcContext *self) RETURNS_NON_NULL NON_NULL();
void irc_context_remove_child (IrcContext *self, IrcContext *child) NON_NULL();
gboolean irc_context_lookup_setting_boolean (IrcContext *self, const char *setting_name) NON_NULL();
GActionGroup *irc_context_get_action_group (void) NON_NULL();

G_END_DECLS
