/* irc-chanstore.c
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

#include "irc-chanstore.h"
#include "irc-context.h"

static void irc_chanstore_iface_init (GtkTreeDragDestIface *);

enum
{
	COL_NAME,
	COL_ACT,
	COL_HIGHLIGHT,
	COL_CTX,
	COL_ACTIVE,
	N_COL
};

struct _IrcChanstore
{
	GtkTreeStore parent_instance;
};

G_DEFINE_TYPE_WITH_CODE (IrcChanstore, irc_chanstore, GTK_TYPE_TREE_STORE,
						G_IMPLEMENT_INTERFACE(GTK_TYPE_TREE_DRAG_DEST, irc_chanstore_iface_init))

static gboolean
irc_chanstore_drop_possible (GtkTreeDragDest *dest, GtkTreePath *dest_path, GtkSelectionData *select_data)
{
	GtkTreePath *src_path;
	GtkTreeModel *src_model;
	GtkTreeModel *dest_model = GTK_TREE_MODEL(dest);

	if (!gtk_tree_get_row_drag_data (select_data, &src_model, &src_path))
		return FALSE;

	if (G_UNLIKELY(src_model != dest_model || !IRC_IS_CHANSTORE(dest)))
	{
		gtk_tree_path_free (src_path); // UPSTREAM: fix g_autoptr(GtkTreePath)
		return FALSE;
	}

	GtkTreeIter src_iter, dest_iter;
	if (!gtk_tree_model_get_iter (dest_model, &dest_iter, dest_path) ||
		!gtk_tree_model_get_iter (src_model, &src_iter, src_path))
	{
		gtk_tree_path_free (src_path);
		return FALSE;
	}
	gtk_tree_path_free (src_path);

  	g_autoptr(IrcContext) dest_ctx;
	g_autoptr(IrcContext) src_ctx;
	gtk_tree_model_get (dest_model, &dest_iter, COL_CTX, &dest_ctx, -1);
	gtk_tree_model_get (src_model, &src_iter, COL_CTX, &src_ctx, -1);

	if (irc_context_get_parent (dest_ctx) != irc_context_get_parent (src_ctx))
		return FALSE;

	return TRUE;
}

static int
irc_chanstore_sort (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
	g_autofree char *name1;
	g_autofree char *name2;

	gtk_tree_model_get (model, a, COL_NAME, &name1, -1);
	gtk_tree_model_get (model, b, COL_NAME, &name2, -1);

	return irc_str_cmp (name1, name2);
}

IrcChanstore *
irc_chanstore_new (void)
{
	IrcChanstore *store = g_object_new (IRC_TYPE_CHANSTORE, NULL);

	GType types[] = { G_TYPE_STRING, G_TYPE_UINT, G_TYPE_BOOLEAN, IRC_TYPE_CONTEXT, G_TYPE_STRING };
	G_STATIC_ASSERT (G_N_ELEMENTS(types) == N_COL);

	gtk_tree_store_set_column_types (GTK_TREE_STORE(store), G_N_ELEMENTS(types), types);

	g_autoptr(GSettings) settings = g_settings_new ("se.tingping.IrcClient");

	gboolean sorted = g_settings_get_boolean (settings, "sort-contextview");
	if (sorted) // TODO: Bind these
	{
		gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE(store), irc_chanstore_sort, NULL, NULL);
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(store), GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
											GTK_SORT_ASCENDING);
	}

	return store;
}

static void
irc_chanstore_finalize (GObject *object)
{
	//IrcChanstore *self = (IrcChanstore *)object;
	//IrcChanstorePrivate *priv = irc_chanstore_get_instance_private (self);

	G_OBJECT_CLASS (irc_chanstore_parent_class)->finalize (object);
}

static void
irc_chanstore_class_init (IrcChanstoreClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = irc_chanstore_finalize;
}

static void
irc_chanstore_iface_init (GtkTreeDragDestIface *iface)
{
	iface->row_drop_possible = irc_chanstore_drop_possible;
}

static void
irc_chanstore_init (IrcChanstore *self)
{
}
