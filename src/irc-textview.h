/* irc-textview.h
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

#include <time.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define IRC_TYPE_TEXTVIEW (irc_textview_get_type())
G_DECLARE_DERIVABLE_TYPE (IrcTextview, irc_textview, IRC, TEXTVIEW, GtkTextView)

struct _IrcTextviewClass {
	GtkTextViewClass parent_class;
};

IrcTextview *irc_textview_new (void);
void irc_textview_append_text (IrcTextview *self, const char *text, time_t stamp);
void irc_textview_set_search (IrcTextview *self, const char *text);

G_END_DECLS
