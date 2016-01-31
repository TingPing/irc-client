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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <glib/gstdio.h>
#include "irc-message.h"
#include "irc-utils.h"
#include "irc-private.h"

G_DEFINE_BOXED_TYPE(IrcMessage, irc_message, irc_message_copy, irc_message_free)

void
create_words (const char *content, GStrv *words_in, GStrv *words_eol_in)
{
	char *p = (char*)content;
	char *spaces[512];
	char **word_eol;
	char **word;
	guint i;

	// Get each space
	for (i = 0; i < sizeof(spaces); ++i)
	{
		if (p != NULL)
		{
			spaces[i] = i ? ++p : p++;
			p = strchr (p, ' ');
		}
		else
			break;
	}
	spaces[i] = NULL;
	const guint len = i + 1; // Including nul

	// Allocate a pointer to these
	word_eol = g_new (char*, len);
	for (i = 0; spaces[i] != NULL; ++i)
		word_eol[i] = spaces[i];
	word_eol[i] = NULL;
	*words_eol_in = word_eol;

	// Split them up
	word = g_new (char*, len);
	for (i = 0; spaces[i] != NULL; ++i)
	{
		if (spaces[i + 1] != NULL)
			word[i] = g_strndup (spaces[i], (guintptr)(spaces[i + 1] - spaces[i] - 1));
		else
			word[i] = g_strdup (spaces[i]); // Last word
	}
	word[i] = NULL;
	*words_in = word;
}

static inline time_t
tags_get_time (IrcMessage *msg)
{
	GTimeVal tv;
	const char *val = irc_message_get_tag_value (msg, "time");
	if (val == NULL)
		return 0;

	if (!g_time_val_from_iso8601 (val, &tv))
		return 0;

	// NOTE: This still uses 32bit timestamps
	// glib supposedly will fix this someday...
	// or we can just modify theirs if needed
	return tv.tv_sec;
}

static inline char
get_escaped_char (const char c)
{
	switch (c)
	{
	case 's':
		return ' ';
	case 'r':
		return '\r';
	case 'n':
		return '\n';
	case ':':
		return ';';
	default:
		return c;
	}
}

static gssize
parse_tags (GHashTable *table, const char *tags)
{
	const char *start = tags;
	gsize ki = 0, vi = 0;
	char key_buf[510], value_buf[510];
	gboolean in_val = FALSE, in_esc = FALSE;

	while (*tags)
	{
		if (*tags == ' ' || *tags == ';')
		{
			in_val = in_esc = FALSE;

			if (G_UNLIKELY(ki == 0))
			{
				g_debug ("Got empty key when parsing tags");
			}
			else
			{
				key_buf[ki] = '\0';
				value_buf[vi] = '\0';

				if (G_UNLIKELY(!g_hash_table_replace (table, g_strdup (key_buf), vi ? g_strdup (value_buf) : NULL)))
					g_debug ("Duplicate tag in message: %s=%s", key_buf, value_buf);

				ki = vi = 0;
			}
			if (*tags == ' ')
				return tags - start;
		}
		else if (*tags == '=')
		{
			in_val = TRUE;
		}
		else
		{
			if (!in_val)
			{
#ifdef G_DEBUG
				if (G_UNLIKELY(!g_ascii_isalnum (*tags) && *tags != '-'))
					g_debug ("Tag key has invalid character '%c'", *tags);
#endif
				key_buf[ki++] = *tags;
			}
			else
			{
				if (*tags == '\\' && !in_esc)
					in_esc = TRUE;
				else
				{
					value_buf[vi++] = in_esc ? get_escaped_char (*tags) : *tags;
					in_esc = FALSE;
				}
			}
		}
		tags++;
	}

	return -1;
}

/**
 * irc_message_new:
 * @line: Valid irc line
 *
 * Returns: (transfer full): Newly allocated message. Free with irc_message_free().
 */
IrcMessage *
irc_message_new (const char *line)
{
  	g_return_val_if_fail (line != NULL, NULL);

	IrcMessage *msg = g_new0 (IrcMessage, 1);
	msg->tags = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	char *p = (char*)line;

	if (*p == '@')
	{
		gssize len = parse_tags (msg->tags, p + 1);
		if (len == -1)
			goto cleanup;

		msg->timestamp = tags_get_time (msg);

		p += len + 1;
	}

	if (*p == ':')
	{
		char *s = strchr (p, ' ');
		if (s != NULL)
			msg->sender = g_strndup (p + 1, (guintptr)(s - p - 1));
		else
			goto cleanup;

		p = s + 1;
	}

	if (g_ascii_isdigit (*p))
	{
		char *end;
		guint64 numeric = g_ascii_strtoull (p, &end, 0);
		if (G_UNLIKELY (numeric > G_MAXUINT16))
			goto cleanup;
		if (G_UNLIKELY (end == NULL))
			goto cleanup;
		else if (G_UNLIKELY (*end != ' '))
			goto cleanup;

		msg->numeric = (guint16)numeric;
		p = end + 1;
	}
	else
	{
		char *s = strchr (p, ' ');
		if (s != NULL)
		{
			msg->command = g_ascii_strup (p, (gintptr)(s - p));
			p = s + 1;
		}
		else
		{
			msg->command = g_ascii_strup (p, -1);
			p = NULL;
		}
	}

	msg->content = g_strdup (p);
	create_words (msg->content, &msg->words, &msg->words_eol);

	return msg;

cleanup:
	g_warning ("Failed to parse message");
	irc_message_free (msg);
	return NULL;
}

/**
 * irc_message_free:
 * @msg: (transfer full): Message to free
 *
 * Frees an #IrcMessage
 */
void
irc_message_free (IrcMessage *msg)
{
	if (msg)
	{
		g_hash_table_unref (msg->tags);
		g_free (msg->sender);
		g_free (msg->command);
		g_free (msg->content);
		g_free (msg->words_eol);
		g_strfreev (msg->words);
		g_free (msg);
	}
}

/**
 * irc_message_copy:
 * @msg: Message to copy
 *
 * Returns: (transfer full): Newly allocated #IrcMessage
 */
IrcMessage *
irc_message_copy (IrcMessage *msg)
{
	g_return_val_if_fail (msg != NULL, NULL);

	IrcMessage *copy = g_new (IrcMessage, 1);

	copy->tags = g_hash_table_ref (msg->tags);
	copy->sender = g_strdup (msg->sender);
	copy->command = g_strdup (msg->command);
	copy->numeric = msg->numeric;
	copy->content = g_strdup (msg->content);
	create_words (copy->content, &copy->words, &copy->words_eol);

	return copy;
}

/**
 * irc_message_has_tag:
 * @msg: Message to check
 * @tag: Tag to look for
 *
 * Returns: %TRUE if tag is in message
 */
gboolean
irc_message_has_tag (IrcMessage *msg, const char *tag)
{
	return g_hash_table_contains (msg->tags, tag);
}

/**
 * irc_message_get_tags:
 * @msg: Message to get tags
 * @len: (out): Length of returned array
 *
 * Returns: (transfer full): Newly allocated array of keys use g_strfreev() to free
 */
GStrv
irc_message_get_tags (IrcMessage *msg, guint *len)
{
	return (GStrv)g_hash_table_get_keys_as_array (msg->tags, len);
}

/**
 * irc_message_get_tag_value:
 * @msg: Message to get value from
 * @tag: Key to lookup value to
 *
 * Returns: Value to key or %NULL
 */
const char *
irc_message_get_tag_value (IrcMessage *msg, const char *tag)
{
	return g_hash_table_lookup (msg->tags, tag);
}
