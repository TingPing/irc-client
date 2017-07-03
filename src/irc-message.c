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

/*
 * Creates a new %GStrv with the contents of another array
 */
static inline GStrv
new_strv (const GStrv array, const gsize len)
{
	GStrv new = g_new (char*, len + 1);
	for (gsize i = 0; i < len; ++i)
		new[i] = array[i];
	new[len] = NULL;
	return new;
}

static GStrv
parse_params (const char *content)
{
	char *param[256];
	char *sp, *p = (char*)content;
	guint i;

	if (content == NULL)
		return g_new0 (char*, 1);

	sp = strchr (p, ' ');
	// Get each space
	for (i = 0; i < sizeof(param); ++i)
	{
		if (*p == ':')  // Trailing param
		{
			param[i] = g_strdup (p + 1);
			break;
		}
		else if (sp == NULL) // Last word
		{
			param[i] = g_strdup (p);
			break;
		}
		else
		{
			param[i] = g_strndup (p, (gsize)(sp - p));
			p = sp + 1;
			sp = strchr (p, ' '); // Next word
		}
	}
	param[++i] = NULL;
	return new_strv (param, i);
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
		if (G_LIKELY(*p == ' '))
			p += 1;
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
	msg->params = parse_params (p);

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
		g_strfreev (msg->params);
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
	copy->params = g_strdupv (msg->params);

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

/**
 * irc_message_get_param:
 * @msg: Message to get parameter from
 * @i: Index of parameter (from 0)
 *
 * Note that parameters are parsed according to RFC1459 which means
 * a word starting with ':' will include all trailing words and the ':'
 * is ignored
 *
 * Returns: parameter or empty string
 */
const char *
irc_message_get_param (IrcMessage *msg, gsize i)
{
	if (i >= g_strv_length (msg->params))
	{
		g_debug ("Requested param beyond length");
		return "";
	}

	return msg->params[i];
}

/**
 * irc_message_get_word_eol:
 * @msg: Message to get words from
 * @i: Index of words (from 0)
 *
 * This is similar to irc_message_get_param() except it ignores
 * the irc parsing rules and just gives you all words after @i
 * number of words (delimiated by spaces).
 *
 * Returns: words or empty string
 */
const char *
irc_message_get_word_eol (IrcMessage *msg, gsize i)
{
	char *p = msg->content;
	while (i-- > 0)
	{
		p = strchr (p, ' ');
		if (p == NULL)
		{
			g_debug ("Requested word_eol beyond length");
			return "";
		}
		else
			++p; // Skip space
	}
	return p;
}
