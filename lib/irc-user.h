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

#include <glib-object.h>
#include "irc-utils.h"

G_BEGIN_DECLS

#define IRC_TYPE_USER (irc_user_get_type())
G_DECLARE_FINAL_TYPE (IrcUser, irc_user, IRC, USER, GObject)

struct _IrcUser
{
	GObject parent_instance;
	char *nick;
	char *hostname;
	char *username;
	char *account;
	char *realname;
};

IrcUser *irc_user_new (const char *userhost) NON_NULL();

G_END_DECLS
