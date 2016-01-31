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

G_BEGIN_DECLS

/**
 * IrcMessage:
 * @sender: Sender of the message
 * @command: Command or %NULL if numeric
 * @numierc: Numeric or 0 if command
 * @words: List of each word in message
 * @words_eol: List of lines starting at each word
 * @timestamp: Unix time of message if #IRC_SERVER_CAP_SERVERTIME enabled
 */
typedef struct {
	/*< public >*/
  	char *sender;
	char *command;
	GStrv words;
	GStrv words_eol;

  	/*< private >*/
	char *content;
	GHashTable *tags;

	/*< public >*/
	time_t timestamp;
	guint16 numeric;
} IrcMessage;

#define IRC_TYPE_MESSAGE (irc_message_get_type())
GType irc_message_get_type (void) G_GNUC_CONST;
IrcMessage *irc_message_new (const char *line);
IrcMessage *irc_message_copy (IrcMessage *msg);
void irc_message_free (IrcMessage *msg);

GStrv irc_message_get_tags (IrcMessage *msg, guint *len);
gboolean irc_message_has_tag (IrcMessage *msg, const char *tag);
const char * irc_message_get_tag_value (IrcMessage *msg, const char *tag);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(IrcMessage, irc_message_free)

G_END_DECLS
