/* irc-colorscheme.c
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

#include <pango/pango.h>
#include <glib/gstdio.h>
#include "irc-colorscheme.h"

#if 0
static const char* mirc_colors[16] = {
	"#D3D7CF", // white
	"#2E3436", // black
	"#3465A4", // blue
	"#4E9A06", // green
	"#CC0000", // red
	"#8F3902", // brown
	"#5C3566", // purple
	"#CE5C00", // orange
	"#C4A000", // yellow
	"#73D216", // lightgreen
	"#11A879", // cyan
	"#58A19D", // lightcyan
	"#57799E", // lightblue
	"#A04365", // pink
	"#555753", // grey
	"#888A85", // lightgrey
};
#endif

static GtkTextTagTable *
irc_colorscheme_new (void)
{
	GtkTextTagTable *table = gtk_text_tag_table_new ();
	g_autoptr(GSettings) color_settings = g_settings_new_with_path ("se.tingping.theme",
																	"/se/tingping/IrcClient/");

	for (gushort i = 0; i < 16; ++i)
	{
		char tag_name[10];
		GtkTextTag *tag;

		g_sprintf (tag_name, "fgcolor%02u", i);
		tag = gtk_text_tag_new (tag_name);
		g_settings_bind (color_settings, tag_name + 2, tag, "foreground", G_SETTINGS_BIND_GET);
		gtk_text_tag_table_add (table, tag);

		g_sprintf (tag_name, "bgcolor%02u", i);
		tag = gtk_text_tag_new (tag_name);
		g_settings_bind (color_settings, tag_name + 2, tag, "background", G_SETTINGS_BIND_GET);
		gtk_text_tag_table_add (table, tag);
	}

	GtkTextTag *tag;
	tag = gtk_text_tag_new ("bold");
	g_object_set (tag, "weight", PANGO_WEIGHT_BOLD, NULL);
	gtk_text_tag_table_add (table, tag);

	tag = gtk_text_tag_new ("italic");
	g_object_set (tag, "style", PANGO_STYLE_ITALIC, NULL);
	gtk_text_tag_table_add (table, tag);

	tag = gtk_text_tag_new ("underline");
	g_object_set (tag, "underline", PANGO_UNDERLINE_SINGLE, NULL);
	gtk_text_tag_table_add (table, tag);

  	tag = gtk_text_tag_new ("time");
	g_object_set (tag, "foreground", "grey", "scale", PANGO_SCALE_SMALL, NULL);
	gtk_text_tag_table_add (table, tag);

	gtk_text_tag_table_add (table, gtk_text_tag_new ("link"));

	return table;
}

GtkTextTagTable *
irc_colorscheme_get_default (void)
{
	static GtkTextTagTable *colors;

	if (G_UNLIKELY(colors == NULL))
		colors = irc_colorscheme_new ();

	return colors;
}
