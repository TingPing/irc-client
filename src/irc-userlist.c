/* irc-userlist.c
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
#include <glib/gi18n.h>
#include "irc-user.h"
#include "irc-userlist.h"
#include "irc-user-list.h"

struct _IrcUserlist
{
	GtkPopover parent_instance;
};

typedef struct
{
	GtkSearchEntry *search_entry;
} IrcUserlistPrivate;

enum {
	COL_NICK,
	COL_USER,
	N_COL,
};

G_DEFINE_TYPE_WITH_PRIVATE (IrcUserlist, irc_userlist, GTK_TYPE_POPOVER)

#if 0
static void
on_search_changed (GtkSearchEntry *entry, gpointer data)
{
	gtk_list_box_invalidate_filter (GTK_LIST_BOX(data));
}

static gboolean
filter_func (GtkListBoxRow *row, gpointer data)
{
	const char *filter = gtk_entry_get_text (GTK_ENTRY(data));

	if (!*filter)
		return TRUE;

   	IrcUser *user = g_object_get_data (G_OBJECT(row), "user");
	if (irc_strcasestr (user->nick, filter) != NULL)
		return TRUE;
	return FALSE;
}
#endif

static GtkWidget *
create_widget (gpointer ptr, gpointer data)
{
	IrcUserListItem *item = IRC_USER_LIST_ITEM(ptr);
	GtkWidget *row = gtk_list_box_row_new ();

	GtkWidget *lbl = gtk_label_new (item->user->nick);
	gtk_container_add (GTK_CONTAINER(row), lbl);

	gtk_widget_show_all (row);
	return row;
}

/**
 * irc_userlist_new:
 * @model: Model of users to show
 *
 * Returns: (transfer full): New userlist
 */
IrcUserlist *
irc_userlist_new (GListModel *model)
{
	GtkBox *box = GTK_BOX(gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
	GtkWidget *sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request (sw, -1, 400); // TODO: Correct sizing

	GtkWidget *view = gtk_list_box_new ();
	gtk_list_box_bind_model (GTK_LIST_BOX(view), model, create_widget, NULL, NULL);
	gtk_container_add (GTK_CONTAINER(sw), view);

#if 0
	GtkWidget *entry = gtk_search_entry_new ();
  	gtk_list_box_set_filter_func (GTK_LIST_BOX(view), filter_func, entry, NULL);
	g_signal_connect (entry, "search-changed", G_CALLBACK(on_search_changed), view);

	gtk_box_pack_start (box, entry, FALSE, TRUE, 0);
#endif

  	gtk_box_pack_start (box, sw, TRUE, TRUE, 0);
	gtk_widget_show_all (GTK_WIDGET(box));
	IrcUserlist *list = g_object_new (IRC_TYPE_USERLIST, "child", box, NULL);

#if 0
  	IrcUserlistPrivate *priv = irc_userlist_get_instance_private (list);
	priv->search_entry = GTK_SEARCH_ENTRY(entry);
#endif

	return list;
}

static void
irc_userlist_finalize (GObject *object)
{
	//IrcUserlist *self = (IrcUserlist *)object;
	//IrcUserlistPrivate *priv = irc_userlist_get_instance_private (self);

	G_OBJECT_CLASS (irc_userlist_parent_class)->finalize (object);
}

static void
irc_userlist_class_init (IrcUserlistClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = irc_userlist_finalize;
}

static void
irc_userlist_init (IrcUserlist *self)
{
}
