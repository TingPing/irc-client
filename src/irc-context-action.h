/* irc-context-action.h
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
#include "irc-context.h"

G_BEGIN_DECLS

#define IRC_TYPE_CONTEXT_ACTION (irc_context_action_get_type())
G_DECLARE_FINAL_TYPE (IrcContextAction, irc_context_action, IRC, CONTEXT_ACTION, GObject)

/**
 * IrcContextActionCallback:
 * @ctx: Context where action is called
 */
typedef void (*IrcContextActionCallback) (IrcContext *ctx);
#define IRC_CONTEXT_ACTION_CALLBACK(f) ((IrcContextActionCallback)(f))

GAction *irc_context_action_new (const char *name, IrcContextActionCallback callback) NON_NULL();

G_END_DECLS
