/* irc-cellrenderer-bubble.c
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

#include "irc-cellrenderer-bubble.h"

struct _IrcCellrendererBubble
{
	GtkCellRendererText parent_instance;
};

struct _IrcCellrendererBubbleClass
{
	GtkCellRendererTextClass parent_class;
};

typedef struct
{
	GdkRGBA *bg_color_norm;
	GdkRGBA *bg_color_highlight;
	gboolean has_hightlight;
	guint32 activity_count;
} IrcCellrendererBubblePrivate;

static void irc_cellrenderer_bubble_class_init (IrcCellrendererBubbleClass *);

G_DEFINE_TYPE_WITH_PRIVATE (IrcCellrendererBubble, irc_cellrenderer_bubble, GTK_TYPE_CELL_RENDERER_TEXT)

GtkCellRenderer *
irc_cellrenderer_bubble_new (void)
{
	GdkRGBA fg_color = { .red = 1.0, .blue = 1.0, .green = 1.0 }; // TODO: Theme
	GtkCellRenderer *self = g_object_new (IRC_TYPE_CELLRENDERER_BUBBLE,
														"xalign", 0.5,
														"xpad", 6,
														"ypad", 3,
														"foreground-rgba", &fg_color,
														"weight", PANGO_WEIGHT_BOLD,
														"scale", PANGO_SCALE_SMALL, NULL);
	gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT(self), 1);
	return self;
}

static void
irc_cellrenderer_bubble_render (GtkCellRenderer *cell, cairo_t *cr, GtkWidget *widget,
							const GdkRectangle *background_area, const GdkRectangle *cell_area,
							GtkCellRendererState flags)
{
	IrcCellrendererBubble *self = IRC_CELLRENDERER_BUBBLE(cell);
	IrcCellrendererBubblePrivate *priv = irc_cellrenderer_bubble_get_instance_private (self);

	if (!(flags & GTK_CELL_RENDERER_SELECTED) && priv->activity_count)
	{
		const int height = cell_area->height - 6;
		const int width = cell_area->width;
		const int x = cell_area->x;
		const int y = cell_area->y + 3;
		const double radius = height / 2.5;
		const double degrees = G_PI / 180.0;
		const GdkRGBA *color;
		if (priv->has_hightlight)
			color = priv->bg_color_highlight;
		else
			color = priv->bg_color_norm;

		// Rounded rectangle
		cairo_new_sub_path (cr);
		cairo_arc (cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
		cairo_arc (cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
		cairo_arc (cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
		cairo_arc (cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
		cairo_close_path (cr);
		cairo_set_source_rgb (cr, color->red, color->green, color->blue);
		cairo_fill (cr);
	}

	GTK_CELL_RENDERER_CLASS(irc_cellrenderer_bubble_parent_class)->
		render(cell, cr, widget, background_area, cell_area, flags);
}

static void
irc_cellrenderer_bubble_finalize (GObject *object)
{
  	IrcCellrendererBubble *self = IRC_CELLRENDERER_BUBBLE(object);
	IrcCellrendererBubblePrivate *priv = irc_cellrenderer_bubble_get_instance_private (self);

	gdk_rgba_free (priv->bg_color_norm);
	gdk_rgba_free (priv->bg_color_highlight);

	G_OBJECT_CLASS (irc_cellrenderer_bubble_parent_class)->finalize (object);
}

enum
{
	PROP_0,
	PROP_ACTIVITY,
	PROP_HIGHLIGHT,
	N_PROPS,
};

static void
irc_cellrenderer_bubble_get_property (GObject *obj, guint prop_id, GValue *val, GParamSpec *pspec)
{
  	IrcCellrendererBubble *self = IRC_CELLRENDERER_BUBBLE(obj);
	IrcCellrendererBubblePrivate *priv = irc_cellrenderer_bubble_get_instance_private (self);

	switch (prop_id)
	{
	case PROP_ACTIVITY:
		g_value_set_uint (val, priv->activity_count);
		break;
	case PROP_HIGHLIGHT:
		g_value_set_boolean (val, priv->has_hightlight);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
irc_cellrenderer_bubble_set_property (GObject *obj, guint prop_id, const GValue *val, GParamSpec *pspec)
{
  	IrcCellrendererBubble *self = IRC_CELLRENDERER_BUBBLE(obj);
	IrcCellrendererBubblePrivate *priv = irc_cellrenderer_bubble_get_instance_private (self);

	switch (prop_id)
	{
	case PROP_ACTIVITY:
		{
			guint32 new_count = g_value_get_uint (val);
			priv->activity_count = new_count;
			if (new_count)
			{
				char new_text[12];
				g_snprintf (new_text, sizeof(new_text), "%u", new_count);
				g_object_set (obj, "text", new_text, NULL);
			}
			else
				g_object_set (obj, "text", "", NULL);
		}
		break;
	case PROP_HIGHLIGHT:
		priv->has_hightlight = g_value_get_boolean (val);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
irc_cellrenderer_bubble_class_init (IrcCellrendererBubbleClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (klass);

    object_class->get_property = irc_cellrenderer_bubble_get_property;
	object_class->set_property = irc_cellrenderer_bubble_set_property;
	object_class->finalize = irc_cellrenderer_bubble_finalize;
	cell_class->render = irc_cellrenderer_bubble_render;

	g_object_class_install_property (object_class, PROP_ACTIVITY,
									g_param_spec_uint ("activity", "Activity", "Activity Count",
										0, G_MAXUINT32, 0, G_PARAM_READWRITE));

	g_object_class_install_property (object_class, PROP_HIGHLIGHT,
									g_param_spec_boolean ("highlight", "Highlight", "Context has a highlight",
										FALSE, G_PARAM_READWRITE));
}

static void
irc_cellrenderer_bubble_init (IrcCellrendererBubble *self)
{
	IrcCellrendererBubblePrivate *priv = irc_cellrenderer_bubble_get_instance_private (self);
	GdkRGBA norm, highlight;

	// TODO: Change with theme
	g_assert (gdk_rgba_parse (&norm, "#729FCF"));
	g_assert (gdk_rgba_parse (&highlight, "#73D216"));

	priv->bg_color_norm = gdk_rgba_copy (&norm);
	priv->bg_color_highlight = gdk_rgba_copy (&highlight);
}
