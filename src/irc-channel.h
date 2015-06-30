/*
 * Copyright 2015 Patrick Griffis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#pragma once

#include <gio/gio.h>
#include <gtk/gtk.h> // Just for GtkListStore
#include "irc-user.h"
#include "irc-context.h"
#include "irc-utils.h"

G_BEGIN_DECLS

#define IRC_TYPE_CHANNEL (irc_channel_get_type())
G_DECLARE_FINAL_TYPE (IrcChannel, irc_channel, IRC, CHANNEL, GObject)

struct _IrcChannel
{
	GObject parent_instance;
  	char *name;
};

IrcChannel *irc_channel_new (IrcContext *parent, const char *name) NON_NULL();
void irc_channel_add_user (IrcChannel *self, IrcUser *user) NON_NULL();
void irc_channel_add_users (IrcChannel *self, GPtrArray *users) NON_NULL();
gboolean irc_channel_remove_user (IrcChannel *self, IrcUser *user) NON_NULL();
gboolean irc_channel_refresh_user (IrcChannel *self, IrcUser *user) NON_NULL();
void irc_channel_part (IrcChannel *self) NON_NULL();
void irc_channel_set_joined (IrcChannel *self, gboolean joined) NON_NULL(1);
GtkListStore *irc_channel_get_users (IrcChannel *self) NON_NULL() RETURNS_NON_NULL;
GActionGroup *irc_channel_get_action_group (void);

G_END_DECLS

