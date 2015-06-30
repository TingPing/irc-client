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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "irc-entrybuffer.h"

struct _IrcEntrybuffer
{
	GtkEntryBuffer parent_instance;
};

typedef struct
{
	GQueue *history;
	int index;
} IrcEntrybufferPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (IrcEntrybuffer, irc_entrybuffer, GTK_TYPE_ENTRY_BUFFER)

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
	gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER(self), text, -1);
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
		gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER(self), "", 0);
		return;
	}
	else
		++priv->index;

	char *text = g_queue_peek_nth (priv->history, (guint)priv->index);
	gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER(self), text, -1);
}

void
irc_entrybuffer_push_into_history (IrcEntrybuffer *self)
{
  	IrcEntrybufferPrivate *priv = irc_entrybuffer_get_instance_private (self);
	const char *current_text = gtk_entry_buffer_get_text (GTK_ENTRY_BUFFER(self));

	g_queue_push_tail (priv->history, g_strdup (current_text));
	gtk_entry_buffer_set_text (GTK_ENTRY_BUFFER(self), "", 0);
	priv->index = -1;
}

IrcEntrybuffer *
irc_entrybuffer_new (void)
{
	return g_object_new (IRC_TYPE_ENTRYBUFFER, NULL);
}

static void
irc_entrybuffer_finalize (GObject *object)
{
	IrcEntrybuffer *self = IRC_ENTRYBUFFER(object);
	IrcEntrybufferPrivate *priv = irc_entrybuffer_get_instance_private (self);

	g_queue_free_full (priv->history, g_free);

	G_OBJECT_CLASS (irc_entrybuffer_parent_class)->finalize (object);
}

static void
irc_entrybuffer_class_init (IrcEntrybufferClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = irc_entrybuffer_finalize;
}

static void
irc_entrybuffer_init (IrcEntrybuffer *self)
{
  	IrcEntrybufferPrivate *priv = irc_entrybuffer_get_instance_private (self);

	priv->history = g_queue_new ();
	priv->index = -1;
}
