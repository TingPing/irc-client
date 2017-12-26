#include <string.h>
#include <glib/gstdio.h>
#include "irc-text-common.h"
#include "irc-utils.h"
#include "irc-private.h"

typedef enum
{
	NONE = 0,
	FG = 2,
	BG = 4,
	ALL = FG|BG
} ColorType;

static inline void
apply_color_tag (GtkTextBuffer *buf, const GtkTextIter *fgstart, const GtkTextIter *bgstart,
				 const GtkTextIter *end, char *fgcolor, char *bgcolor, ColorType flags)
{
	char colorstr[10];
	guint64 col_num;
	if (*fgcolor && flags & FG)
	{
		col_num = g_ascii_strtoull (fgcolor, NULL, 0);
		if (col_num <= 98)
		{
			g_sprintf (colorstr, "fgcolor%02u", (guint)col_num); // Always fits.
			gtk_text_buffer_apply_tag_by_name (buf, colorstr, fgstart, end);
		}
		fgcolor[0] = '\0'; fgcolor[1] = '\0';
	}
	if (*bgcolor && flags & BG)
	{
	  	col_num = g_ascii_strtoull (bgcolor, NULL, 0);
		if (col_num <= 98)
		{
			g_sprintf (colorstr, "bgcolor%02u", (guint)col_num); // Always fits.
			gtk_text_buffer_apply_tag_by_name (buf, colorstr, bgstart, end);
		}
		bgcolor[0] = '\0'; bgcolor[1] = '\0';
	}
}

static inline void
apply_hex_color_tag (GtkTextBuffer *buf, const GtkTextIter *fgstart, const GtkTextIter *bgstart,
					 const GtkTextIter *end, char *fgcolor, char *bgcolor, ColorType flags)
{
	char colorstr[8];
	char namestr[10];
	GtkTextTagTable *table = gtk_text_buffer_get_tag_table (buf);
	GtkTextTag *tag;

	if (*fgcolor && flags & FG)
	{
		g_sprintf (colorstr, "#%s", fgcolor);
		g_sprintf (namestr, "fg-%s", fgcolor);

		if (!(tag = gtk_text_tag_table_lookup (table, namestr)))
		{
			tag = gtk_text_tag_new (namestr);
			g_object_set (G_OBJECT(tag), "foreground", colorstr, NULL);
			g_assert (gtk_text_tag_table_add (table, tag));
		}

		gtk_text_buffer_apply_tag (buf, tag, fgstart, end);
		memset (fgcolor, '\000', 7);
	}
	if (*bgcolor && flags & BG)
	{
		g_sprintf (colorstr, "#%s", bgcolor);
		g_sprintf (namestr, "bg-%s", bgcolor);

		if (!(tag = gtk_text_tag_table_lookup (table, namestr)))
		{
			tag = gtk_text_tag_new (namestr);
			g_object_set (G_OBJECT(tag), "background", colorstr, NULL);
			g_assert (gtk_text_tag_table_add (table, tag));
		}

		gtk_text_buffer_apply_tag (buf, tag, bgstart, end);
		memset (bgcolor, '\000', 7);
	}
}

static ColorType
extract_colors (GtkTextIter *it, char *fgcolor, char *bgcolor)
{
	ColorType extracted_colors = NONE;
	GtkTextIter end = *it;
	gtk_text_iter_forward_chars (&end, 6);
	g_autofree char *text = gtk_text_iter_get_text (it, &end);
	char *p = text;

	if (!*p)
		return extracted_colors;
	++p; // Skip control code

	if (*p && g_ascii_isdigit (*p))
	{
		fgcolor[0] = *p++;
		if (*p && g_ascii_isdigit (*p))
			fgcolor[1] = *p++;
		extracted_colors |= FG;
	}
	if (*p == ',')
	{
		p++;
		if (*p && g_ascii_isdigit (*p))
		{
			bgcolor[0] = *p++;
			if (*p && g_ascii_isdigit (*p))
				bgcolor[1] = *p++;
			extracted_colors |= BG;
		}
	}


	if (p != text)
	{
		gtk_text_iter_forward_chars (it, (int)(p - text));
	}

	return extracted_colors;
}

static ColorType
extract_hexcolors (GtkTextIter *it, char *fgcolor, char *bgcolor)
{
	ColorType extracted_colors = NONE;
	GtkTextIter end = *it;
	gtk_text_iter_forward_chars (&end, 14);
	g_autofree char *text = gtk_text_iter_get_text (it, &end);
	char *p = text;

	if (!*p)
		return extracted_colors;
	++p; // Skip control code

	gsize len = strlen (p);
	if (irc_util_is_valid_hex_color (p, MIN(6, len)))
	{
		strncpy (fgcolor, p, 6);
		len -= 6;
		p += 6;
		extracted_colors |= FG;
	}
	if (*p == ',' && irc_util_is_valid_hex_color (p + 1, MIN(6, len - 1)))
	{
		strncpy (bgcolor, p + 1, 6);
		p += 7;
		extracted_colors |= BG;
	}

	if (p != text)
	{
		gtk_text_iter_forward_chars (it, (int)(p - text));
	}

	return extracted_colors;
}

void
apply_irc_tags (GtkTextBuffer *buf, const GtkTextIter *start, const GtkTextIter *end, gboolean clear)
{
	char fgcol[3] = { 0 }, bgcol[3] = { 0 }, hexfgcol[7] = { 0 }, hexbgcol[7] = { 0 };
	GtkTextIter bold_start, italic_start, underline_start, hidden_start,
	            strikethrough_start, monospace_start, fgcolor_start, bgcolor_start;
	gboolean bold = FALSE, italic = FALSE, underline = FALSE,
	         hidden = FALSE, strikethrough = FALSE, monospace = FALSE;
	GtkTextIter cur_iter = *start;

#define APPLY_COLOR(flags) G_STMT_START \
{ \
	if (*fgcol || *bgcol) \
		apply_color_tag (buf, &fgcolor_start, &bgcolor_start, &cur_iter, fgcol, bgcol, flags); \
	if (*hexfgcol || *hexbgcol) \
		apply_hex_color_tag (buf, &fgcolor_start, &bgcolor_start, &cur_iter, hexfgcol, hexbgcol, flags); \
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

#define EXTRACT_AND_APPLY_COLOR(prefix) G_STMT_START \
{ \
	GtkTextIter it = cur_iter; \
	char new_fgcol[7] = { 0 }, new_bgcol[7] = { 0 }; \
	ColorType extracted = extract_##prefix##colors (&it, new_fgcol, new_bgcol); \
 \
	if (extracted == NONE) \
		APPLY_COLOR(ALL); \
	else \
	{ \
		if (extracted & FG) \
		{ \
			APPLY_COLOR(FG); \
			fgcolor_start = cur_iter; \
			strcpy (prefix##fgcol, new_fgcol); \
		} \
		if (extracted & BG) \
		{ \
			APPLY_COLOR(BG); \
			bgcolor_start = cur_iter; \
			strcpy (prefix##bgcol, new_bgcol); \
		} \
	} \
 \
	gtk_text_buffer_apply_tag_by_name (buf, "hidden", &cur_iter, &it); \
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
			EXTRACT_AND_APPLY_COLOR();
			continue;
		case HEXCOLOR:
			APPLY_HIDDEN;
			EXTRACT_AND_APPLY_COLOR(hex);
			continue;
		case RESET:
			APPLY_HIDDEN;
			APPLY_COLOR(ALL);
			APPLY(hidden);
			APPLY(bold);
			APPLY(italic);
			APPLY(underline);
			APPLY(strikethrough);
			APPLY(monospace);
			continue;
		case ITALIC:
			APPLY_HIDDEN;
			START_OR_APPLY(italic);
			continue;
		case UNDERLINE:
			APPLY_HIDDEN;
			START_OR_APPLY(underline);
			continue;
		case BOLD:
			APPLY_HIDDEN;
			START_OR_APPLY(bold);
			continue;
		case STRIKETHROUGH:
			APPLY_HIDDEN;
			START_OR_APPLY(strikethrough);
			continue;
		case MONOSPACE:
			APPLY_HIDDEN;
			START_OR_APPLY(monospace);
			continue;
		case HIDDEN:
			APPLY_HIDDEN;
			START_OR_APPLY(hidden);
			continue;
		case REVERSE:
			APPLY_HIDDEN;
			// Reverse is a bit harder to handle since we use the default
			// themes colors for text
			continue;
		}
	} while (gtk_text_iter_compare (&cur_iter, end) < 0 && gtk_text_iter_forward_char (&cur_iter));

	// End of line, add leftover tags
	APPLY_COLOR(ALL);
	APPLY(bold);
	APPLY(italic);
	APPLY(underline);
	APPLY(hidden);
	APPLY(strikethrough);
	APPLY(monospace);
}
