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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <glib/gi18n.h>
#include "irc-user.h"
#include "irc-userlist.h"

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

static void
on_search_changed (GtkSearchEntry *entry, gpointer data)
{
	GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER(data);

	gtk_tree_model_filter_refilter (filter);
}

static gboolean
filter_func (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	const char *filter = gtk_entry_get_text (GTK_ENTRY(data));

	if (!*filter)
		return TRUE;

   	g_autoptr(IrcUser) user;
	gtk_tree_model_get (model, iter, COL_USER, &user, -1);
	if (irc_strcasestr (user->nick, filter) != NULL)
		return TRUE;
	return FALSE;
}

/**
 * irc_userlist_new:
 * @model: Model of users to show
 *
 * Returns: (transfer full): New userlist
 */
IrcUserlist *
irc_userlist_new (GtkTreeModel *model)
{
	GtkBox *box = GTK_BOX(gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
	GtkWidget *sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request (sw, -1, 400); // TODO: Correct sizing
	GtkTreeModel *modelfilter = gtk_tree_model_filter_new (model, NULL);
  	GtkWidget *view = gtk_tree_view_new_with_model (modelfilter);
	gtk_tree_view_set_fixed_height_mode (GTK_TREE_VIEW(view), TRUE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(view), FALSE);
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW(view), 0, _("Users"),
												gtk_cell_renderer_text_new (), "text", 0, NULL);
	gtk_container_add (GTK_CONTAINER(sw), view);


	GtkWidget *entry = gtk_search_entry_new ();
	g_signal_connect (entry, "search-changed", G_CALLBACK(on_search_changed), modelfilter);
  	gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER(modelfilter), filter_func, entry, NULL);

	gtk_box_pack_start (box, entry, FALSE, TRUE, 0);
	gtk_box_pack_start (box, sw, TRUE, TRUE, 0);
	gtk_widget_show_all (GTK_WIDGET(box));
	IrcUserlist *list = g_object_new (IRC_TYPE_USERLIST, "child", box, NULL);
  	IrcUserlistPrivate *priv = irc_userlist_get_instance_private (list);
	priv->search_entry = GTK_SEARCH_ENTRY(entry);


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
