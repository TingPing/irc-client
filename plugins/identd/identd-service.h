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

struct IdentdService;
typedef struct IdentdService IdentdService;

IdentdService *identd_service_new (void);
void identd_service_add_user (IdentdService *service, const char *username, guint16 port);
void identd_service_add_address (IdentdService *service, const char *address);
void identd_service_destroy (IdentdService *service);
