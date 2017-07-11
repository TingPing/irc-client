#include <string.h>
#include <glib/gstdio.h>
#include "irc-text-common.h"
#include "irc-utils.h"
#include "irc-private.h"

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

static inline void
apply_hex_color_tag (GtkTextBuffer *buf, const GtkTextIter *start, const GtkTextIter *end, char *fgcolor, char *bgcolor)
{
	char colorstr[8];
	char namestr[10];
	GtkTextTagTable *table = gtk_text_buffer_get_tag_table (buf);
	GtkTextTag *tag;

	if (*fgcolor)
	{
		g_sprintf (colorstr, "#%s", fgcolor);
		g_sprintf (namestr, "fg-%s", fgcolor);

		if (!(tag = gtk_text_tag_table_lookup (table, namestr)))
		{
			tag = gtk_text_tag_new (namestr);
			g_object_set (G_OBJECT(tag), "foreground", colorstr, NULL);
			g_assert (gtk_text_tag_table_add (table, tag));
		}

		gtk_text_buffer_apply_tag (buf, tag, start, end);
		memset (fgcolor, '\000', 7);
	}
	if (*bgcolor)
	{
		g_sprintf (colorstr, "#%s", bgcolor);
		g_sprintf (namestr, "bg-%s", bgcolor);

		if (!(tag = gtk_text_tag_table_lookup (table, namestr)))
		{
			tag = gtk_text_tag_new (namestr);
			g_object_set (G_OBJECT(tag), "background", colorstr, NULL);
			g_assert (gtk_text_tag_table_add (table, tag));
		}

		gtk_text_buffer_apply_tag (buf, tag, start, end);
		memset (bgcolor, '\000', 7);
	}
}

static void
extract_colors (GtkTextIter *it, char *fgcolor, char *bgcolor)
{
	GtkTextIter end = *it;
	gtk_text_iter_forward_chars (&end, 5);
	g_autofree char *text = gtk_text_iter_get_text (it, &end);
	char *p = text;

	if (*p && g_ascii_isdigit (*p))
	{
		fgcolor[0] = *p++;
		if (*p && g_ascii_isdigit (*p))
			fgcolor[1] = *p++;
	}
	if (*p == ',')
	{
		p++;
		if (*p && g_ascii_isdigit (*p))
		{
			bgcolor[0] = *p++;
			if (*p && g_ascii_isdigit (*p))
				bgcolor[1] = *p++;
		}
	}


	if (p != text)
	{
		gtk_text_iter_forward_chars (it, (int)(p - text));
	}
}

static void
extract_hex_colors (GtkTextIter *it, char *fgcolor, char *bgcolor)
{
	GtkTextIter end = *it;
	gtk_text_iter_forward_chars (&end, 13);
	g_autofree char *text = gtk_text_iter_get_text (it, &end);
	char *p = text;
	gsize len = strlen (text);

	if (_irc_util_is_valid_hex_color (text, MIN(6, len)))
	{
		strncpy (fgcolor, text, 6);
		len -= 6;
		p += 6;
	}
	if (*p == ',' && _irc_util_is_valid_hex_color (p + 1, MIN(6, len - 1)))
	{
		strncpy (bgcolor, p + 1, 6);
		p += 7;
	}

	if (p != text)
	{
		gtk_text_iter_forward_chars (it, (int)(p - text));
	}
}

void
apply_irc_tags (GtkTextBuffer *buf, const GtkTextIter *start, const GtkTextIter *end, gboolean clear)
{
	char fgcol[3] = { 0 }, bgcol[3] = { 0 }, hexfgcol[7] = { 0 }, hexbgcol[7] = { 0 };
	GtkTextIter bold_start, italic_start, underline_start, color_start, hexcolor_start, hidden_start;
	gboolean bold = FALSE, italic = FALSE, underline = FALSE, color = FALSE, hexcolor = FALSE, hidden = FALSE;
	GtkTextIter cur_iter = *start;

#define STOP_COLOR G_STMT_START \
{ \
	color = hexcolor = FALSE; \
} \
G_STMT_END

#define APPLY_COLOR G_STMT_START \
{ \
	if (*fgcol || *bgcol) \
		apply_color_tag (buf, &color_start, &cur_iter, fgcol, bgcol); \
	else if (*hexfgcol || *hexbgcol) \
		apply_hex_color_tag (buf, &hexcolor_start, &cur_iter, hexfgcol, hexbgcol); \
} \
G_STMT_END

#define APPLY_HIDDEN G_STMT_START \
{ \
	GtkTextIter next = cur_iter; \
	gtk_text_iter_forward_char (&next); \
	gtk_text_buffer_apply_tag_by_name (buf, "hidden", &cur_iter, &next); \
} \
G_STMT_END

#define APPLY(x) G_STMT_START \
{ \
	if (x) \
	{ \
		gtk_text_buffer_apply_tag_by_name (buf, #x, &x##_start, &cur_iter); \
		x = FALSE; \
	} \
} \
G_STMT_END

#define START(x) G_STMT_START \
{ \
	x##_start = hidden_start = cur_iter; \
	x = TRUE; \
} \
G_STMT_END

#define START_OR_APPLY(x) G_STMT_START \
{ \
	if (!x) \
	{ \
		x##_start = cur_iter; \
	} \
	else \
	{ \
		gtk_text_buffer_apply_tag_by_name (buf, #x, &x##_start, &cur_iter); \
	} \
	x = !x; \
} \
G_STMT_END

	if (clear)
		gtk_text_buffer_remove_all_tags (buf, start, end);

	do {
		const gunichar c = gtk_text_iter_get_char (&cur_iter);
		switch (c)
		{
		case COLOR:
			APPLY_HIDDEN;
			STOP_COLOR;
			APPLY_COLOR;
			START(color);
			continue;
		case HEXCOLOR:
			APPLY_HIDDEN;
			STOP_COLOR;
			APPLY_COLOR;
			START(hexcolor);
			continue;
		case RESET:
			APPLY_HIDDEN;
			STOP_COLOR;
			APPLY_COLOR;
			APPLY(color);
			APPLY(italic);
			APPLY(underline);
			continue;
		case ITALIC:
			APPLY_HIDDEN;
			STOP_COLOR;
			START_OR_APPLY(italic);
			continue;
		case UNDERLINE:
			APPLY_HIDDEN;
			STOP_COLOR;
			START_OR_APPLY(underline);
			continue;
		case BOLD:
			APPLY_HIDDEN;
			STOP_COLOR;
			START_OR_APPLY(bold);
			continue;
		case HIDDEN:
			APPLY_HIDDEN;
			STOP_COLOR;
			START_OR_APPLY(hidden);
			// There is an "invisible" property, not sure if i want inbound text
			// to be able to use this though
			continue;
		case REVERSE:
			APPLY_HIDDEN;
			STOP_COLOR;
			// Reverse is a bit harder to handle since we use the default
			// themes colors for text, i don't want to tag all text
			continue;
		default:
			if (color)
			{
				GtkTextIter it = cur_iter;
				extract_colors (&it, fgcol, bgcol);
				gtk_text_buffer_apply_tag_by_name (buf, "hidden", &cur_iter, &it);
				STOP_COLOR;
			}
			else if (hexcolor)
			{
				GtkTextIter it = cur_iter;
				extract_hex_colors (&it, hexfgcol, hexbgcol);
				gtk_text_buffer_apply_tag_by_name (buf, "hidden", &cur_iter, &it);
				STOP_COLOR;
			}
		}
	} while (gtk_text_iter_compare (&cur_iter, end) != 0 && gtk_text_iter_forward_char (&cur_iter));

	// End of line, add leftover tags
	APPLY_COLOR;
	APPLY(bold);
	APPLY(italic);
	APPLY(underline);
	APPLY(hidden);
}
