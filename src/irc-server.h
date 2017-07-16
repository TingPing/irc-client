/* irc-server.h
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
#include "irc-channel.h"
#include "irc-utils.h"

G_BEGIN_DECLS

#define IRC_TYPE_SERVER (irc_server_get_type())
G_DECLARE_DERIVABLE_TYPE (IrcServer, irc_server, IRC, SERVER, GObject)

/**
 * IrcServerCap:
 * @IRC_SERVER_CAP_NONE: No capabilities supported
 * @IRC_SERVER_CAP_SERVER_TIME: server-time or znc.in/server-time-iso cap
 * @IRC_SERVER_CAP_ZNC_SELF_MESSAGE: znc.in/self-message
 * @IRC_SERVER_CAP_SASL: sasl
 * @IRC_SERVER_CAP_USERHOST_IN_NAMES: userhost-in-names
 * @IRC_SERVER_CAP_EXTENDED_JOIN: extended-join
 * @IRC_SERVER_CAP_ACCOUNT_NOTIFY: account-notify
 * @IRC_SERVER_CAP_AWAY_NOTIFY: away-notify
 * @IRC_SERVER_CAP_MULTI_PREFIX: mutli-prefix
 * @IRC_SERVER_SUPPORT_WHOX: WHOX in ISUPPORT
 * @IRC_SERVER_CAP_CHGHOST: chghost
 * @IRC_SERVER_SUPPORT_MONITOR: MONITOR in ISUPPORT
 * @IRC_SERVER_CAP_TWITCH_MEMBERSHIP: twitch.tv/membership
 * @IRC_SERVER_CAP_CAP_NOTIFY: cap-notify
 * @IRC_SERVER_CAP_TWITCH_TAGS: twitch.tv/tags
 *
 * See http://ircv3.net/ for more information.
 *
 */
typedef enum
{
	IRC_SERVER_CAP_NONE = 1 << 0,
	IRC_SERVER_CAP_SERVER_TIME = 1 << 1,
	IRC_SERVER_CAP_ZNC_SELF_MESSAGE = 1 << 2,
	IRC_SERVER_CAP_SASL = 1 << 3,
	IRC_SERVER_CAP_USERHOST_IN_NAMES = 1 << 4,
	IRC_SERVER_CAP_EXTENDED_JOIN = 1 << 5,
	IRC_SERVER_CAP_ACCOUNT_NOTIFY = 1 << 6,
	IRC_SERVER_CAP_AWAY_NOTIFY = 1 << 7,
	IRC_SERVER_CAP_MULTI_PREFIX = 1 << 8,
	IRC_SERVER_SUPPORT_WHOX = 1 << 9,
	IRC_SERVER_CAP_CHGHOST = 1 << 10,
	IRC_SERVER_SUPPORT_MONITOR = 1 << 11,
	IRC_SERVER_CAP_TWITCH_MEMBERSHIP = 1 << 12,
	IRC_SERVER_CAP_CAP_NOTIFY = 1 << 13,
	IRC_SERVER_CAP_TWITCH_TAGS = 1 << 14,
} IrcServerCap;

IrcServer *irc_server_new_from_network (const char *network_name) NON_NULL();
IrcUser *irc_server_get_me (IrcServer *self);
void irc_server_connect (IrcServer *self) NON_NULL();
void irc_server_disconnect (IrcServer *self) NON_NULL();
void irc_server_flushq (IrcServer *self) NON_NULL();
gboolean irc_server_get_is_connected (IrcServer *self) NON_NULL();
void irc_server_write_line (IrcServer *self, const char *line) NON_NULL();
gboolean irc_server_str_equal (IrcServer *self, const char *str1, const char *str2) NON_NULL();
void irc_server_write_linef (IrcServer *self, const char *format, ...) G_GNUC_PRINTF(2, 3);
GActionGroup *irc_server_get_action_group (void);

G_END_DECLS
