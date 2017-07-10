/* irc-entrybuffer.c
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

#include "irc-entrybuffer.h"
#include "irc-colorscheme.h"
#include "irc-user-list-item.h"

struct _IrcEntrybuffer
{
	GtkSourceBuffer parent_instance;
};

typedef struct
{
	GQueue *history;
	GListModel *comp_model;
	char *last_prefix;
	char *last_match;
	int last_cursor;
	int index;
	gboolean ignore_changed;
} IrcEntrybufferPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (IrcEntrybuffer, irc_entrybuffer, GTK_SOURCE_TYPE_BUFFER)

void
irc_entrybuffer_history_up (IrcEntrybuffer *self)
{
	IrcEntrybufferPrivate *priv = irc_entrybuffer_get_instance_private (self);
	const uint len = g_queue_get_length (priv->history);

	if (len == 0 || priv->index == 0)
		return; // Out of history
	else if (priv->index == -1)
		priv->index = (int)len - 1; // Just starting
	else
		--priv->index; // Continuing

	char *text = g_queue_peek_nth (priv->history, (guint)priv->index);
	gtk_text_buffer_set_text (GTK_TEXT_BUFFER(self), text, -1);
}

void
irc_entrybuffer_history_down (IrcEntrybuffer *self)
{
	IrcEntrybufferPrivate *priv = irc_entrybuffer_get_instance_private (self);
	const uint len = g_queue_get_length (priv->history);

	if (len == 0 || priv->index == -1)
		return; // No future history
	else if (priv->index == (int)len - 1)
	{
		// Saw everything, back to normal
		priv->index = -1;
		gtk_text_buffer_set_text (GTK_TEXT_BUFFER(self), "", 0);
		return;
	}
	else
		++priv->index;

	char *text = g_queue_peek_nth (priv->history, (guint)priv->index);
	gtk_text_buffer_set_text (GTK_TEXT_BUFFER(self), text, -1);
}

void
irc_entrybuffer_push_into_history (IrcEntrybuffer *self)
{
  	IrcEntrybufferPrivate *priv = irc_entrybuffer_get_instance_private (self);
	GtkTextBuffer *buf = GTK_TEXT_BUFFER(self);

	GtkTextIter start, end;
	gtk_text_buffer_get_start_iter (buf, &start);
	gtk_text_buffer_get_end_iter (buf, &end);

	char *current_text = gtk_text_buffer_get_text (buf, &start, &end, FALSE);

	g_queue_push_tail (priv->history, current_text);
	gtk_text_buffer_set_text (buf, "", 0);
	priv->index = -1;
}

static const char *
lookup_nick_by_prefix (GListModel *model, const char *prefix, const char *last_match)
{
	for (guint i = 0; i < g_list_model_get_n_items (model); ++i)
	{
		g_autoptr(IrcUserListItem) item = IRC_USER_LIST_ITEM(g_list_model_get_item(model, i));
		const char *nick = IRC_USER(item->user)->nick;

		// If we had a previous match skip to after there
		if (last_match && irc_str_cmp (nick, last_match) <= 0)
			continue;

		if (irc_str_has_prefix (nick, prefix))
			return nick;
	}

	return NULL;
}

static void
reset_completion_state (IrcEntrybuffer *self)
{
	IrcEntrybufferPrivate *priv = irc_entrybuffer_get_instance_private (self);

	if (!priv->ignore_changed)
	{
		if (priv->last_match)
		{
			GtkTextBuffer *buffer = GTK_TEXT_BUFFER(self);
			gtk_text_buffer_delete_mark_by_name (buffer, "comp-start");
			gtk_text_buffer_delete_mark_by_name (buffer, "comp-end");
		}
		g_clear_pointer (&priv->last_match, g_free);
		g_clear_pointer (&priv->last_prefix, g_free);
	}
}

static inline gboolean
iter_ends_irc_word (const GtkTextIter *iter)
{
	switch (gtk_text_iter_get_char (iter))
	{
	case '[':
	case ']':
	case '{':
	case '}':
	case '\\':
	case '|':
		return FALSE;
	default:
		return gtk_text_iter_ends_word (iter);
	}
}

static inline gboolean
iter_backwards_irc_word (GtkTextIter *iter)
{
	while (gtk_text_iter_backward_word_start (iter))
	{
		GtkTextIter prev = *iter;
		if (!gtk_text_iter_backward_char (&prev))
			return TRUE;

		switch (gtk_text_iter_get_char (&prev))
		{
		case '[':
		case ']':
		case '{':
		case '}':
		case '\\':
		case '|':
			// This wasn't actually a word start, so go back one more
			iter = &prev;
			continue;
		default:
			return TRUE;
		}
	}
	return FALSE;
}

gboolean
irc_entrybuffer_tab_complete (IrcEntrybuffer *self)
{
	GtkTextBuffer *buf = GTK_TEXT_BUFFER(self);
	IrcEntrybufferPrivate *priv = irc_entrybuffer_get_instance_private (self);

	if (!priv->comp_model)
		return FALSE;

	int cursor_offset = 0;
	g_object_get (buf, "cursor-position", &cursor_offset, NULL);

	if (cursor_offset != priv->last_cursor)
	{
		priv->last_cursor = cursor_offset;
		reset_completion_state (self);
	}

	GtkTextIter start, end;
	GtkTextMark *comp_start, *comp_end;

	comp_start = gtk_text_buffer_get_mark (buf, "comp-start");
	comp_end = gtk_text_buffer_get_mark (buf, "comp-end");

	// First tab completion needs to meet some preconditions and
	// then we will store state related to it
	if (priv->last_prefix == NULL)
	{
		gtk_text_buffer_get_start_iter (buf, &start);
		gtk_text_iter_forward_chars (&start, cursor_offset);

		// Can't complete the middle of a word
		if (!iter_ends_irc_word (&start))
			return FALSE;

		end = start;
		// Has to already have a partial word
		if (!iter_backwards_irc_word (&start))
			return FALSE;

		priv->last_prefix = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
	}
	else if (!comp_start)
	{
		// We already tried and failed
		return FALSE;
	}
	else
	{
		gtk_text_buffer_get_iter_at_mark (buf, &start, comp_start);
		gtk_text_buffer_get_iter_at_mark (buf, &end, comp_end);
	}

	const char *nick = lookup_nick_by_prefix (priv->comp_model, priv->last_prefix, priv->last_match);
	g_free (priv->last_match);
	priv->last_match = g_strdup (nick);
	if (nick)
	{
		if (comp_start)
			gtk_text_buffer_move_mark (buf, comp_start, &start);
		else
			comp_start = gtk_text_buffer_create_mark (buf, "comp-start", &start, TRUE);

		priv->ignore_changed = TRUE;

		gtk_text_buffer_delete (buf, &start, &end);
		gtk_text_buffer_insert (buf, &start, nick, -1);

		end = start; // insert moves the iter forward
		gtk_text_buffer_get_iter_at_mark (buf, &start, comp_start);

		if (comp_end)
			gtk_text_buffer_move_mark (buf, comp_end, &end);
		else
		{
			gtk_text_buffer_create_mark (buf, "comp-end", &end, TRUE);
			if (gtk_text_iter_is_start (&start))
				gtk_text_buffer_insert (buf, &end, ", ", 2);
		}

		g_object_get (buf, "cursor-position", &priv->last_cursor, NULL);
		priv->ignore_changed = FALSE;

		return TRUE;
	}
	return FALSE;
}

void
irc_entrybuffer_set_completion_model (IrcEntrybuffer *self, GListModel *model)
{
	IrcEntrybufferPrivate *priv = irc_entrybuffer_get_instance_private (self);

	g_clear_object (&priv->comp_model);

	if (model != NULL)
	{
		g_return_if_fail (g_list_model_get_item_type (model) == IRC_TYPE_USER_LIST_ITEM);
		priv->comp_model = g_object_ref (model);
	}
}


IrcEntrybuffer *
irc_entrybuffer_new (void)
{
	return g_object_new (IRC_TYPE_ENTRYBUFFER, "tag-table", irc_colorscheme_get_default(), NULL);
}

static void
irc_entrybuffer_changed (GtkTextBuffer *buffer)
{
	reset_completion_state (IRC_ENTRYBUFFER(buffer));
	GTK_TEXT_BUFFER_CLASS(irc_entrybuffer_parent_class)->changed (buffer);
}

static void
irc_entrybuffer_finalize (GObject *object)
{
	IrcEntrybuffer *self = IRC_ENTRYBUFFER(object);
	IrcEntrybufferPrivate *priv = irc_entrybuffer_get_instance_private (self);

	g_queue_free_full (priv->history, g_free);
	g_clear_object (&priv->comp_model);
	g_clear_pointer (&priv->last_match, g_free);
	g_clear_pointer (&priv->last_prefix, g_free);

	G_OBJECT_CLASS (irc_entrybuffer_parent_class)->finalize (object);
}

static void
irc_entrybuffer_class_init (IrcEntrybufferClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkTextBufferClass *buffer_class = GTK_TEXT_BUFFER_CLASS (klass);

	buffer_class->changed = irc_entrybuffer_changed;
	object_class->finalize = irc_entrybuffer_finalize;
}

static void
irc_entrybuffer_init (IrcEntrybuffer *self)
{
  	IrcEntrybufferPrivate *priv = irc_entrybuffer_get_instance_private (self);

	priv->history = g_queue_new ();
	priv->index = -1;
}
