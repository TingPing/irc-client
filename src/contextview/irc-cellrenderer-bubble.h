/* irc-cellrenderer-bubble.h
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

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#if 0 // Upstream doesn't use these yet
#define IRC_TYPE_CELLRENDERER_BUBBLE (irc_cellrenderer_bubble_get_type())
G_DECLARE_FINAL_TYPE (IrcCellrendererBubble, irc_cellrenderer_bubble, IRC, CELLRENDERER_BUBBLE, IRC_CELLRENDERER_BUBBLE)

#else

#define IRC_TYPE_CELLRENDERER_BUBBLE (irc_cellrenderer_bubble_get_type())
#define IRC_CELLRENDERER_BUBBLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), IRC_TYPE_CELLRENDERER_BUBBLE, IrcCellrendererBubble))
#define IRC_CELLRENDERER_BUBBLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), IRC_TYPE_CELLRENDERER_BUBBLE, IrcCellrendererBubbleClass))
#define IRC_IS_CELLRENDERER_BUBBLE(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), IRC_TYPE_CELLRENDERER_BUBBLE))
#define IRC_IS_CELLRENDERER_BUBBLE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), IRC_TYPE_CELLRENDERER_BUBBLE))
#define IRC_CELLRENDERER_BUBBLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), IRC_TYPE_CELLRENDERER_BUBBLE, IrcCellrendererBubbleClass))

typedef struct _IrcCellrendererBubble IrcCellrendererBubble;
typedef struct _IrcCellrendererBubbleClass IrcCellrendererBubbleClass;
GType irc_cellrenderer_bubble_get_type (void) G_GNUC_CONST;
#endif

GtkCellRenderer *irc_cellrenderer_bubble_new (void);

G_END_DECLS
