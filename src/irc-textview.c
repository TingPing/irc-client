/* irc-textview.c
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

#include <string.h>
#include "irc-textview.h"
#include "irc-colorscheme.h"
#include "irc-text-common.h"

typedef struct
{
	char *search;
	GtkTextMark *search_mark;
} IrcTextviewPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (IrcTextview, irc_textview, GTK_TYPE_TEXT_VIEW)

static void
apply_misc_tags (GtkTextBuffer *buf, const char *text, int offset)
{
  	const int line = gtk_text_buffer_get_line_count (buf);
	GtkTextIter word_start, word_end, iter;

	gtk_text_buffer_get_iter_at_line (buf, &iter, line);

	// This is pretty dumb for now...
	// Perhaps take the regex from HexChat
	// Sadly the text iter word detection is useless on urls
	g_auto(GStrv) words = g_strsplit (text, " ", 0);
	for (gsize i = 0; i < g_strv_length (words); ++i)
	{
		if (g_regex_match_simple ("^https?://.*$", words[i], G_REGEX_OPTIMIZE, 0))
		{
			gtk_text_iter_set_line_offset (&iter, offset);
			word_start = iter;
			gtk_text_iter_set_line_offset (&iter, offset + (int)g_utf8_strlen (words[i], -1));
			word_end = iter;
			gtk_text_buffer_apply_tag_by_name (buf, "link", &word_start, &word_end);
		}
		offset += (int)g_utf8_strlen (words[i], -1) + 1;
	}
}

/**
 * irc_textview_append_text:
 * @text: Line of text to append
 * @stamp: Timestamp of event or 0 for current time
 */
void
irc_textview_append_text (IrcTextview *self, const char *text, time_t stamp)
{
	// This is designed for one-line at a time
	if (G_UNLIKELY(strchr (text, '\n') != NULL || strchr (text, '\r') != NULL))
	{
		g_auto(GStrv) lines = g_strsplit_set (text, "\r\n", 0);
		for (gsize i = 0; lines[i]; ++i)
		{
			if (*lines[i])
				irc_textview_append_text (self, lines[i], stamp);
		}
		return;
	}

	GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW(self));
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter (buf, &iter);

	if (G_LIKELY(!gtk_text_iter_is_start (&iter)))
	{
  		gtk_text_buffer_insert (buf, &iter, "\n", 1);
	}

	if (stamp == 0)
		stamp = time (NULL);

	g_autoptr(GDateTime) timestamp = g_date_time_new_from_unix_utc (stamp);
	g_autofree char *stampstr = g_date_time_format (timestamp, "%T ");
	int stamp_len = (int)strlen (stampstr);
	if (stampstr != NULL)
		gtk_text_buffer_insert_with_tags_by_name (buf, &iter, stampstr, stamp_len, "time", NULL);
	gtk_text_buffer_insert (buf, &iter, text, -1);

	const int line = gtk_text_buffer_get_line_count (buf);
	GtkTextIter start, end;
	gtk_text_buffer_get_iter_at_line (buf, &start, line);
	gtk_text_iter_set_line_offset (&start, stamp_len);
	end = start;
	gtk_text_iter_forward_to_line_end (&end);

	apply_irc_tags (buf, &start, &end, FALSE);
	apply_misc_tags (buf, text, stamp_len);
}


static char *
get_tagged_word (GtkTextTag *tag, GtkTextIter *iter)
{
	GtkTextIter start, end, cached;
	cached = *iter;

	if (!gtk_text_iter_forward_to_tag_toggle (iter, tag))
		return NULL;
	start = *iter;

	iter = &cached;
	if (!gtk_text_iter_backward_to_tag_toggle (iter, tag))
		return NULL;
	end = *iter;

	return gtk_text_iter_get_text (&start, &end);
}

static gboolean
irc_textview_button_press_event (GtkWidget *wid, GdkEventButton *event)
{
  	static GtkTextTag *link_tag;
  	if (G_UNLIKELY(link_tag == NULL))
		link_tag = gtk_text_tag_table_lookup (irc_colorscheme_get_default(), "link");

	if (event->type == GDK_BUTTON_PRESS)
	{
		if (event->button == GDK_BUTTON_PRIMARY)
		{
	  		GtkTextView *view = GTK_TEXT_VIEW(wid);
			int x, y;
			GtkTextIter iter;
			gtk_text_view_window_to_buffer_coords (view, GTK_TEXT_WINDOW_WIDGET, (int)event->x, (int)event->y, &x, &y);
			gtk_text_view_get_iter_at_location (view, &iter, x, y);
			if (gtk_text_iter_has_tag (&iter, link_tag))
			{
				g_autofree char *url = get_tagged_word (link_tag, &iter);
				if (url)

				{
					GtkWidget *win = gtk_widget_get_toplevel (wid);
					gtk_show_uri_on_window (GTK_WINDOW(win), url, event->time, NULL);
				}


				return GDK_EVENT_STOP;
			}
		}
		// TODO: secondary
	}
	return GTK_WIDGET_CLASS(irc_textview_parent_class)->button_press_event(wid, event);
}

static void
underline_link (GtkTextBuffer *buf, GtkTextIter *iter, GtkTextTag *link_tag, GtkTextTag *underline_tag)
{
	GtkTextIter start, end, cached;
	cached = *iter;

	if (!gtk_text_iter_toggles_tag (iter, link_tag))
	{
		if (!gtk_text_iter_forward_to_tag_toggle (iter, link_tag))
			return;
	}
	end = *iter;

	iter = &cached;
	if (!gtk_text_iter_toggles_tag (iter, link_tag))
	{
		if (!gtk_text_iter_backward_to_tag_toggle (iter, link_tag))
			return;
	}
	start = *iter;

	gtk_text_buffer_apply_tag (buf, underline_tag, &start, &end);
	GtkTextMark *mark = gtk_text_mark_new ("underlined_link", FALSE);
	gtk_text_buffer_add_mark (buf, mark, &start);
}

static void
remove_underlined_link (GtkTextBuffer *buf, GtkTextMark *mark, GtkTextTag *underline_tag)
{
	GtkTextIter iter;
	gtk_text_buffer_get_iter_at_mark (buf, &iter, mark);
	gtk_text_buffer_delete_mark (buf, mark);

	GtkTextIter end = iter;
	if (!gtk_text_iter_forward_to_tag_toggle (&end, underline_tag))
		return;

	gtk_text_buffer_remove_tag (buf, underline_tag, &iter, &end);
}

static gboolean
irc_textview_motion_notify_event (GtkWidget *wid, GdkEventMotion *event)
{
  	static GdkCursor *cursor;
	static GtkTextTag *link_tag, *underline_tag;
	GtkTextView *view = GTK_TEXT_VIEW(wid);
	GtkTextBuffer *buf = gtk_text_view_get_buffer (view);
	GdkWindow *win = event->window;

	if (G_UNLIKELY(cursor == NULL))
		cursor = gdk_cursor_new_for_display (gdk_window_get_display(win), GDK_HAND1);
	if (G_UNLIKELY(link_tag == NULL))
	{
		link_tag = gtk_text_tag_table_lookup (irc_colorscheme_get_default(), "link");
  		underline_tag = gtk_text_tag_table_lookup (irc_colorscheme_get_default(), "underline");
	}

	int x, y;
	GtkTextIter iter;
	gtk_text_view_window_to_buffer_coords (view, GTK_TEXT_WINDOW_WIDGET, (int)event->x, (int)event->y, &x, &y);
	gtk_text_view_get_iter_at_location (view, &iter, x, y);
	if (gtk_text_iter_has_tag (&iter, link_tag))
	{
		if (!gtk_text_iter_has_tag (&iter, underline_tag))
		{
			GtkTextMark *mark;
			if ((mark = gtk_text_buffer_get_mark (buf, "underlined_link")))
				remove_underlined_link (buf, mark, underline_tag); // Only one may exist at a time

			gdk_window_set_cursor (win, cursor);
			underline_link (buf, &iter, link_tag, underline_tag);
		}
	}
	else
	{
		GtkTextMark *mark;
		if ((mark = gtk_text_buffer_get_mark (buf, "underlined_link")))
		{
			gdk_window_set_cursor (win, NULL);
			remove_underlined_link (buf, mark, underline_tag);
		}
	}

	return GTK_WIDGET_CLASS(irc_textview_parent_class)->motion_notify_event(wid, event);
}

static void
irc_textview_get_preferred_width (GtkWidget *wid, int *min_width, int *natural_width)
{
	// This is just to work around a bug in GtkScrolledWindow
	*min_width = 1;
	*natural_width = 1;
}

static void
irc_textview_search_previous (GSimpleAction *action, GVariant *param, gpointer data)
{
  	IrcTextview *self = IRC_TEXTVIEW(data);
	IrcTextviewPrivate *priv = irc_textview_get_instance_private (self);
	GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW(self));
	GtkTextIter iter, start, end;

	if (!priv->search)
		return;

	if (!priv->search_mark)
		gtk_text_buffer_get_end_iter (buf, &iter);
	else
		gtk_text_buffer_get_iter_at_mark (buf, &iter, priv->search_mark);

	if (!gtk_text_iter_backward_search (&iter, priv->search, GTK_TEXT_SEARCH_TEXT_ONLY|GTK_TEXT_SEARCH_CASE_INSENSITIVE,
								&start, &end, NULL))
	{
		if (priv->search_mark)
		{
			gtk_text_buffer_delete_mark (buf, priv->search_mark);
			priv->search_mark = NULL;
		}
		return;
	}

	if (!priv->search_mark)
		priv->search_mark = gtk_text_buffer_create_mark (buf, "search", &start, FALSE);
	else
		gtk_text_buffer_move_mark (buf, priv->search_mark, &start);
	gtk_text_buffer_select_range (buf, &start, &end);
	gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW(self), priv->search_mark);
	return;
}

static void
irc_textview_search_next (GSimpleAction *action, GVariant *param, gpointer data)
{
  	IrcTextview *self = IRC_TEXTVIEW(data);
	IrcTextviewPrivate *priv = irc_textview_get_instance_private (self);
	GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW(self));
	GtkTextIter iter, start, end;

  	if (!priv->search)
		return;

	if (!priv->search_mark)
		gtk_text_buffer_get_start_iter (buf, &iter);
	else
		gtk_text_buffer_get_iter_at_mark (buf, &iter, priv->search_mark);

	if (!gtk_text_iter_forward_search (&iter, priv->search, GTK_TEXT_SEARCH_TEXT_ONLY|GTK_TEXT_SEARCH_CASE_INSENSITIVE,
								&start, &end, NULL))
	{
		if (priv->search_mark)
		{
			gtk_text_buffer_delete_mark (buf, priv->search_mark);
			priv->search_mark = NULL;
		}
		return;
	}

	if (!priv->search_mark)
		priv->search_mark = gtk_text_buffer_create_mark (buf, "search", &end, FALSE);
	else
		gtk_text_buffer_move_mark (buf, priv->search_mark, &end);
	gtk_text_buffer_select_range (buf, &start, &end);
	gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW(self), priv->search_mark);
	return;
}

void
irc_textview_set_search (IrcTextview *self, const char *text)
{
	IrcTextviewPrivate *priv = irc_textview_get_instance_private (self);
	GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW(self));

	if (text && !*text)
		text = NULL;

	g_free (priv->search);
	priv->search = g_strdup (text);

	if (priv->search_mark)
	{
		gtk_text_buffer_delete_mark (buf, priv->search_mark);
		priv->search_mark = NULL;
	}

	//gtk_text_buffer_delete_mark_by_name (buf, "selection_bound");
	if (priv->search)
		irc_textview_search_previous (NULL, NULL, self);
}

IrcTextview *
irc_textview_new (void)
{
	GtkTextBuffer *buffer = gtk_text_buffer_new (irc_colorscheme_get_default());
	return g_object_new (IRC_TYPE_TEXTVIEW, "buffer", buffer,
	                                        "editable", FALSE,
	                                        "right-margin", 9,
	                                        "left-margin", 9,
	                                        "top-margin", 9,
	                                        "bottom-margin", 9,
	                                        "indent", -10, // Indent wrap
	                                        "cursor-visible", FALSE,
	                                        "wrap-mode", GTK_WRAP_WORD_CHAR, NULL);
}

static void
irc_textview_finalize (GObject *object)
{
	IrcTextviewPrivate *priv = irc_textview_get_instance_private (IRC_TEXTVIEW(object));

	g_free (priv->search);

	G_OBJECT_CLASS (irc_textview_parent_class)->finalize (object);
}


static void
irc_textview_class_init (IrcTextviewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);

	object_class->finalize = irc_textview_finalize;
	wid_class->motion_notify_event = irc_textview_motion_notify_event;
	wid_class->button_press_event = irc_textview_button_press_event;
	wid_class->get_preferred_width = irc_textview_get_preferred_width;
}

static void
irc_textview_init (IrcTextview *self)
{
	GtkStyleContext *style = gtk_widget_get_style_context (GTK_WIDGET(self));
	gtk_style_context_add_class (style, "irc-textview");

  	//IrcTextviewPrivate *priv = irc_textview_get_instance_private (self);
	GSimpleActionGroup *group = g_simple_action_group_new ();
	const GActionEntry actions[] = {
		{ .name = "search-previous", .activate = irc_textview_search_previous },
		{ .name = "search-next", .activate = irc_textview_search_next },
	};
	const char * const previous_accels[] = { "<Primary>g", NULL };
	const char * const next_accels[] = { "<Primary><Shift>g", NULL };

	g_action_map_add_action_entries (G_ACTION_MAP (group), actions, G_N_ELEMENTS(actions), self);
	gtk_widget_insert_action_group (GTK_WIDGET(self), "textview", G_ACTION_GROUP(group));

	GtkApplication *app = GTK_APPLICATION(g_application_get_default ());
	if (app)
	{
		gtk_application_set_accels_for_action (app, "textview.search-previous", previous_accels );
		gtk_application_set_accels_for_action (app, "textview.search-next", next_accels );
	}
}
