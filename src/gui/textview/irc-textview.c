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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <glib/gstdio.h>
#include "irc-textview.h"
#include "irc-utils.h"
#include "irc-colorscheme.h"

struct _IrcTextview
{
	GtkTextView parent_instance;
};

typedef struct
{
	char *search;
	GtkTextMark *search_mark;
} IrcTextviewPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (IrcTextview, irc_textview, GTK_TYPE_TEXT_VIEW)

static inline void
apply_color_tag (GtkTextBuffer *buf, const GtkTextIter *start, const GtkTextIter *end, char *fgcolor, char *bgcolor)
{
	char colorstr[10];
	guint64 col_num;
	if (*fgcolor)
	{
		col_num = g_ascii_strtoull (fgcolor, NULL, 0);
		if (col_num <= 15)
		{
			g_sprintf (colorstr, "fgcolor%02u", (guint)col_num); // Always fits.
			gtk_text_buffer_apply_tag_by_name (buf, colorstr, start, end);
		}
		fgcolor[0] = '\0'; fgcolor[1] = '\0';
	}
	if (*bgcolor)
	{
	  	col_num = g_ascii_strtoull (bgcolor, NULL, 0);
		if (col_num <= 15)
		{
			g_sprintf (colorstr, "bgcolor%02u", (guint)col_num); // Always fits.
			gtk_text_buffer_apply_tag_by_name (buf, colorstr, start, end);
		}
		bgcolor[0] = '\0'; bgcolor[1] = '\0';
	}
}

static void
apply_irc_tags (GtkTextBuffer *buf, const char *text, int offset)
{
	gboolean bold = FALSE, italic = FALSE, underline = FALSE;
	char fgcol[3] = { 0 }, bgcol[3] = { 0 };
	GtkTextIter bstart, istart, ustart, cur_iter, cstart;
	const int line = gtk_text_buffer_get_line_count (buf);
	gushort parsing_color = 0;
	gtk_text_buffer_get_iter_at_line_offset (buf, &cur_iter, line, offset);

#define STOP_COLOR G_STMT_START \
{ \
	parsing_color = 0; \
} \
G_STMT_END

	// NOTE: This must be exactly in sync with irc_strip_attributes();
	for (char *p = (char*)text; *p ;p += g_utf8_skip[(guchar)*p])
	{
		switch(*p)
		{
		case COLOR:
			STOP_COLOR;
			if (*fgcol || *bgcol)
			{
				apply_color_tag (buf, &cstart, &cur_iter, fgcol, bgcol);
			}
			cstart = cur_iter;
			parsing_color = 1;
			continue;
		case RESET:
			STOP_COLOR;
			if (*fgcol || *bgcol)
			{
				apply_color_tag (buf, &cstart, &cur_iter, fgcol, bgcol);
			}
			if (bold)
			{
				gtk_text_buffer_apply_tag_by_name (buf, "bold", &bstart, &cur_iter);
				bold = FALSE;
			}
			if (italic)
			{
				gtk_text_buffer_apply_tag_by_name (buf, "italic", &istart, &cur_iter);
				italic = FALSE;
			}
			if (underline)
			{
				gtk_text_buffer_apply_tag_by_name (buf, "underline", &ustart, &cur_iter);
				underline = FALSE;
			}
			continue;
		case ITALIC:
			STOP_COLOR;
			if (!italic)
				istart = cur_iter;
			else
				gtk_text_buffer_apply_tag_by_name (buf, "italic", &istart, &cur_iter);
			italic = !italic;
			continue;
		case UNDERLINE:
			STOP_COLOR;
			if (!underline)
				ustart = cur_iter;
			else
				gtk_text_buffer_apply_tag_by_name (buf, "underline", &ustart, &cur_iter);
			underline = !underline;
			continue;
		case BOLD:
			STOP_COLOR;
			if (!bold)
				bstart = cur_iter;
			else
				gtk_text_buffer_apply_tag_by_name (buf, "bold", &bstart, &cur_iter);
			bold = !bold;
			continue;
		case HIDDEN:
			STOP_COLOR;
			// There is an "invisible" property, not sure if i want inbound text
			// to be able to use this though
			continue;
		case REVERSE:
			STOP_COLOR;
			// Reverse is a bit harder to handle since we use the default
			// themes colors for text, i don't want to tag all text
			continue;
		default:
			if (!parsing_color)
			{
				gtk_text_iter_forward_char (&cur_iter);
				continue;
			}

			if (!g_ascii_isdigit (*p))
			{
				if (*p == ',' && parsing_color <= 3)
				{
					parsing_color = 4;
					continue;
				}
				else if (parsing_color == 4) // We had a comma but never used it
				{
					parsing_color = 6;
					gtk_text_iter_forward_char (&cur_iter);
				}
				else
					parsing_color = 6;
			}
			// don't parse background color without a comma
			else if (parsing_color == 3 && *p != ',')
				parsing_color = 6;

			switch (parsing_color)
			{
			case 1:
				fgcol[0] = *p;
				parsing_color++;
				continue;
			case 2:
				fgcol[1] = *p;
				parsing_color++;
				continue;
			case 4:
				bgcol[0] = *p;
				parsing_color++;
				continue;
			case 5:
				bgcol[1] = *p;
				parsing_color++;
				continue;
			}

			g_assert (parsing_color == 6);
			parsing_color = 0;
			gtk_text_iter_forward_char (&cur_iter);
		}
	}

	// End of line, add leftover tags
	g_assert (gtk_text_iter_is_end (&cur_iter));
	if (*fgcol || *bgcol)
		apply_color_tag (buf, &cstart, &cur_iter, fgcol, bgcol);
	if (bold)
		gtk_text_buffer_apply_tag_by_name (buf, "bold", &bstart, &cur_iter);
	if (italic)
		gtk_text_buffer_apply_tag_by_name (buf, "italic", &istart, &cur_iter);
	if (underline)
		gtk_text_buffer_apply_tag_by_name (buf, "underline", &ustart, &cur_iter);

}

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

static char *
make_timestamp (time_t stamp, int *stamp_len_out)
{
	static char *format;
  	char dest[64];
	gsize len;
	struct tm tm;

	if (stamp == 0)
		stamp = time (NULL);

	localtime_r (&stamp, &tm);

	if (G_UNLIKELY(format == NULL))
	{
		// TODO: Config
		// strftime() expects locale
		format = g_locale_from_utf8 ("%H:%M:%S| ", -1, NULL, NULL, NULL);
	}

	len = strftime (dest, sizeof(dest), format, &tm);
	if (len == 0)
	{
		g_warning ("Failed to format timestamp");
		return NULL;
	}

	g_assert (len <= G_MAXSSIZE);
	char *utf8_stamp = g_locale_to_utf8 (dest, (gssize)len, NULL, NULL, NULL);
  	*stamp_len_out = (int)g_utf8_strlen (dest, -1); // Chars needed for textiter offsets
	return utf8_stamp;
}

/**
 * irc_textview_append_text:
 * @text: Line of text to append
 * @stamp: Timestamp of event or 0 for current time
 */
void
irc_textview_append_text (IrcTextview *self, const char *text, time_t stamp)
{
	GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW(self));
	GtkTextIter iter;
	gtk_text_buffer_get_end_iter (buf, &iter);

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

	if (G_LIKELY(!gtk_text_iter_is_start (&iter)))
	{
  		gtk_text_buffer_insert (buf, &iter, "\n", 1);
	}

	int stamp_len = 0;
	g_autofree char *stampstr = make_timestamp (stamp, &stamp_len);
	if (stampstr != NULL)
		gtk_text_buffer_insert_with_tags_by_name (buf, &iter, stampstr, stamp_len, "time", NULL);

	g_autofree char *stripped = irc_strip_attributes (text);
	gtk_text_buffer_insert (buf, &iter, stripped, -1);
	apply_irc_tags (buf, text, stamp_len);
	apply_misc_tags (buf, stripped, stamp_len);
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
					gtk_show_uri (NULL, url, event->time, NULL);

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
											"right-margin", 8, // Extra for scrollbar
											"left-margin", 5,
											"indent", -10, // Odd?
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
	gtk_application_set_accels_for_action (app, "textview.search-previous", previous_accels );
	gtk_application_set_accels_for_action (app, "textview.search-next", next_accels );
}