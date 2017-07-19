/* irc-chatview.c
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

#include "irc-chatview.h"

struct _IrcChatview
{
	GtkScrolledWindow parent_instance;
};

typedef struct
{
	gboolean scrolled_down;
} IrcChatviewPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (IrcChatview, irc_chatview, GTK_TYPE_SCROLLED_WINDOW)

static void
on_vadj_changed (GtkAdjustment *adj, gpointer data)
{
	IrcChatview *self = IRC_CHATVIEW(data);
	IrcChatviewPrivate *priv = irc_chatview_get_instance_private (self);
	double value = gtk_adjustment_get_value (adj);

	if (value >= gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj))
		priv->scrolled_down = TRUE;
	else
		priv->scrolled_down = FALSE;
}

static void
irc_chatview_size_allocate (GtkWidget *wid, GtkAllocation *alloc)
{
	GTK_WIDGET_CLASS(irc_chatview_parent_class)->size_allocate(wid, alloc);

	IrcChatview *self = IRC_CHATVIEW(wid);
  	IrcChatviewPrivate *priv = irc_chatview_get_instance_private (self);
	GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(self));
	if (priv->scrolled_down)
		gtk_adjustment_set_value (adj, gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj));
}

IrcChatview *
irc_chatview_new (void)
{
	IrcChatview *view = g_object_new (IRC_TYPE_CHATVIEW, "shadow-type", GTK_SHADOW_NONE,
											"hscrollbar-policy", GTK_POLICY_NEVER, NULL);
	//IrcTextview *textview = irc_textview_new ();
	//gtk_container_add (GTK_CONTAINER(view), GTK_WIDGET(textview));

	GtkAdjustment *vadj = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW(view));
	g_signal_connect (vadj, "value-changed", G_CALLBACK(on_vadj_changed), view);

	return view;
}

static void
irc_chatview_finalize (GObject *object)
{
	G_OBJECT_CLASS (irc_chatview_parent_class)->finalize (object);
}

static void
irc_chatview_class_init (IrcChatviewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);

	object_class->finalize = irc_chatview_finalize;
	wid_class->size_allocate = irc_chatview_size_allocate;
}

static void
irc_chatview_init (IrcChatview *self)
{
  	IrcChatviewPrivate *priv = irc_chatview_get_instance_private (self);
	priv->scrolled_down = TRUE;
}
