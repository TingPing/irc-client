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

// http://anti.teamidiot.de/static/nei/*/extended_mirc_color_proposal.html
static const char* extended_mirc_colors[83] = {
    "#470000", "#472100", "#474700", "#324700", "#004700", "#00472c", "#004747", "#002747", "#000047", "#2e0047", "#470047", "#47002a",
    "#740000", "#743a00", "#747400", "#517400", "#007400", "#007449", "#007474", "#004074", "#000074", "#4b0074", "#740074", "#740045",
    "#b50000", "#b56300", "#b5b500", "#7db500", "#00b500", "#00b571", "#00b5b5", "#0063b5", "#0000b5", "#7500b5", "#b500b5", "#b5006b",
    "#ff0000", "#ff8c00", "#ffff00", "#b2ff00", "#00ff00", "#00ffa0", "#00ffff", "#008cff", "#0000ff", "#a500ff", "#ff00ff", "#ff0098",
    "#ff5959", "#ffb459", "#ffff71", "#cfff60", "#6fff6f", "#65ffc9", "#6dffff", "#59b4ff", "#5959ff", "#c459ff", "#ff66ff", "#ff59bc",
    "#ff9c9c", "#ffd39c", "#ffff9c", "#e2ff9c", "#9cff9c", "#9cffdb", "#9cffff", "#9cd3ff", "#9c9cff", "#dc9cff", "#ff9cff", "#ff94d3",
    "#000000", "#131313", "#282828", "#363636", "#4d4d4d", "#656565", "#818181", "#9f9f9f", "#bcbcbc", "#e2e2e2", "#ffffff",
};

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

    for (gushort i = 0; i < G_N_ELEMENTS(extended_mirc_colors); ++i)
    {
		char tag_name[10];
		GtkTextTag *tag;

		g_sprintf (tag_name, "fgcolor%02u", i + 16);
		tag = gtk_text_tag_new (tag_name);
		g_object_set (G_OBJECT(tag), "foreground", extended_mirc_colors[i], NULL);
		gtk_text_tag_table_add (table, tag);

		g_sprintf (tag_name, "bgcolor%02u", i + 16);
		tag = gtk_text_tag_new (tag_name);
		g_object_set (G_OBJECT(tag), "background", extended_mirc_colors[i], NULL);
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

	tag = gtk_text_tag_new ("strikethrough");
	g_object_set (tag, "strikethrough", TRUE, NULL);
	gtk_text_tag_table_add (table, tag);

	tag = gtk_text_tag_new ("monospace");
	g_object_set (tag, "family", "Monospace", NULL);
	gtk_text_tag_table_add (table, tag);


	tag = gtk_text_tag_new ("hidden");
	g_object_set (tag, "invisible", TRUE, NULL);
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
