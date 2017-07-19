/* irc-identd-service.h
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

struct IrcIdentdService;
typedef struct IrcIdentdService IrcIdentdService;

IrcIdentdService *irc_identd_service_get_default (void);
void irc_identd_service_add_user (IrcIdentdService *service, const char *username, guint16 port);
void irc_identd_service_add_address (IrcIdentdService *service, const char *address);

