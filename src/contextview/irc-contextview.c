/* irc-contextview.c
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

#include "irc-contextview.h"
#include "irc-chanstore.h"
#include "irc-context.h"
#include "irc-context-manager.h"
#include "irc-cellrenderer-bubble.h"

struct _IrcContextview
{
	GtkTreeView parent_instance;
};

G_DEFINE_TYPE (IrcContextview, irc_contextview, GTK_TYPE_TREE_VIEW)

// Kept in sync with irc-chanstore.c
enum
{
	COL_NAME,
	COL_ACT,
	COL_HIGHLIGHT,
	COL_CTX,
	COL_ACTIVE,
	N_COL
};

static gboolean
remove_ctx_foreach (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	g_autoptr(IrcContext) row_ctx;

	gtk_tree_model_get (model, iter, COL_CTX, &row_ctx, -1);

	if (row_ctx != data)
		return FALSE;

	gtk_tree_store_remove (GTK_TREE_STORE(model), iter);
	//TODO: Change selection here?
	return TRUE;
}

static void
on_context_removed (IrcContextManager *mgr, IrcContext *ctx, gpointer data)
{
	GtkTreeView *view = GTK_TREE_VIEW(data);
	GtkTreeModel *model = gtk_tree_view_get_model (view);
	gtk_tree_model_foreach (model, remove_ctx_foreach, ctx);
}

static void
on_selection_changed (GtkTreeSelection *sel, gpointer data)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	if (!gtk_tree_selection_get_selected (sel, &model, &iter))
	{
		g_warning ("Invalid chanview selection");
		return;
	}

	g_autoptr(IrcContext) ctx;
	gtk_tree_model_get (model, &iter, COL_CTX, &ctx, -1);

	IrcContextManager *mgr = irc_context_manager_get_default ();
	irc_context_manager_set_front_context (mgr, ctx);
}

struct changed_foreach_data
{
	GtkTreeSelection *sel;
	IrcContext *ctx;
};

static gboolean
front_changed_foreach (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
  	GtkTreeSelection *sel = ((struct changed_foreach_data*)data)->sel;
	IrcContext *ctx = ((struct changed_foreach_data*)data)->ctx;
	g_autoptr(IrcContext) row_ctx;
	gtk_tree_model_get (model, iter, COL_CTX, &row_ctx, -1);
	if (row_ctx == ctx)
	{
		gtk_tree_store_set (GTK_TREE_STORE(model), iter, COL_ACT, 0, COL_HIGHLIGHT, FALSE, -1);
		g_signal_handlers_block_by_func (sel, on_selection_changed, NULL);
		gtk_tree_selection_select_iter (sel, iter);
		g_signal_handlers_unblock_by_func (sel, on_selection_changed, NULL);
		return TRUE;
	}

	return FALSE;
}

static void
on_front_context_changed (IrcContextManager *mgr, IrcContext *ctx, gpointer data)
{
	GtkTreeView *view = GTK_TREE_VIEW(data);
	GtkTreeModel *model = gtk_tree_view_get_model (view);
	GtkTreeSelection *sel = gtk_tree_view_get_selection (view);
	struct changed_foreach_data foreach_data = { sel, ctx };

	gtk_tree_model_foreach (model, front_changed_foreach, &foreach_data);
}

struct foreach_data
{
	IrcContext *ctx;
	gboolean bool_data;
};

static gboolean
set_active (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	IrcContext *active_ctx = IRC_CONTEXT(((struct foreach_data*)data)->ctx);
	const gboolean is_active = ((struct foreach_data*)data)->bool_data;
	g_autoptr(IrcContext) row_ctx;
	gtk_tree_model_get (model, iter, COL_CTX, &row_ctx, -1);
	if (row_ctx == active_ctx)
	{
		gtk_tree_store_set (GTK_TREE_STORE(model), iter, COL_ACTIVE, is_active ? NULL : "grey", -1);
		return TRUE;
	}
	return FALSE;
}

static void
on_context_active (IrcContext *ctx, GParamSpec *pspec, gpointer data)
{
	GtkTreeModel *model = GTK_TREE_MODEL (data);
	struct foreach_data for_data = { .ctx = ctx };

	g_object_get (ctx, "active", &for_data.bool_data, NULL);
	//g_debug ("Context active %d", for_data.bool_data);
	gtk_tree_model_foreach (model, set_active, &for_data);
}


static gboolean
add_activity (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	IrcContext *activity_ctx = IRC_CONTEXT(((struct foreach_data*)data)->ctx);
	const gboolean is_highlight = ((struct foreach_data*)data)->bool_data;
	g_autoptr(IrcContext) row_ctx;
	gtk_tree_model_get (model, iter, COL_CTX, &row_ctx, -1);
	if (row_ctx == activity_ctx)
	{
		guint32 last_act;
		gboolean had_highlight;
		gtk_tree_model_get (model, iter, COL_ACT, &last_act, COL_HIGHLIGHT, &had_highlight, -1);
		gtk_tree_store_set (GTK_TREE_STORE(model), iter, COL_ACT, ++last_act, COL_HIGHLIGHT,
							had_highlight || is_highlight, -1);
		return TRUE;
	}
	return FALSE;
}

static void
on_context_activity (IrcContext *ctx, gboolean is_highlight, gpointer data)
{
	GtkTreeView *view = GTK_TREE_VIEW(data);
	GtkTreeModel *model = gtk_tree_view_get_model (view);
  	IrcContextManager *mgr = irc_context_manager_get_default ();

	if (ctx != irc_context_manager_get_front_context (mgr))
	{
		struct foreach_data for_data = { ctx, is_highlight };
		gtk_tree_model_foreach (model, add_activity, &for_data);
	}
}

static void
on_context_added (IrcContextManager *mgr, IrcContext *ctx, gpointer data)
{
	GtkTreeView *view = GTK_TREE_VIEW(data);
	GtkTreeModel *model = gtk_tree_view_get_model (view);
	GtkTreeStore *store = GTK_TREE_STORE(model);

	IrcContext *parent = irc_context_get_parent(ctx);
	if (parent != NULL)
	{
		GtkTreeIter iter;
		if (!gtk_tree_model_get_iter_first (model, &iter))
		{
			g_warning ("No toplevel iters for child..");
			return;
		}
		do {
			g_autoptr(IrcContext) row_ctx;
			gtk_tree_model_get (model, &iter, COL_CTX, &row_ctx, -1);
			if (row_ctx == parent)
			{
				GtkTreeIter new_iter;
				gtk_tree_store_append (store, &new_iter, &iter);
				gtk_tree_store_set (store, &new_iter, COL_NAME, irc_context_get_name (ctx), COL_CTX, ctx, -1);

				GtkTreePath *path = gtk_tree_model_get_path (model, &iter);
				gtk_tree_view_expand_to_path (view, path);
				gtk_tree_path_free (path);
				break;
			}
		}
		while (gtk_tree_model_iter_next (model, &iter));
	}
	else
	{
		GtkTreeIter new_iter;
		gtk_tree_store_append (store, &new_iter, NULL);
		gtk_tree_store_set (store, &new_iter, COL_NAME, irc_context_get_name (ctx), COL_CTX, ctx, -1);

		irc_context_manager_set_front_context (mgr, ctx);
		//gtk_tree_selection_select_iter (gtk_tree_view_get_selection (view), &new_iter);
	}

	g_signal_connect (ctx, "activity", G_CALLBACK(on_context_activity), data);
	g_signal_connect (ctx, "notify::active", G_CALLBACK(on_context_active), model);
}


static gboolean
irc_context_view_button_press_event (GtkWidget *wid, GdkEventButton *event)
{
	GtkTreeView *view = GTK_TREE_VIEW (wid);
	GtkTreeModel *model;
  	GtkTreePath *path;
	GtkTreeIter iter;

	if (event->button != GDK_BUTTON_SECONDARY ||
		gtk_tree_view_get_bin_window (view) != event->window ||
		!gtk_tree_view_get_path_at_pos (view, (int)event->x, (int)event->y, &path, NULL, NULL, NULL))
	{
		return GTK_WIDGET_CLASS(irc_contextview_parent_class)->button_press_event(wid, event);
	}

	model = gtk_tree_view_get_model (view);
	if (gtk_tree_model_get_iter (model, &iter, path))
	{
		g_autoptr(IrcContext) ctx;

		gtk_tree_model_get (model, &iter, COL_CTX, &ctx, -1);

		GMenuModel *menu = irc_context_get_menu (ctx);
		GtkMenu *gtk_menu = GTK_MENU(gtk_menu_new_from_model (menu));
		gtk_menu_attach_to_widget (gtk_menu, wid, NULL);
		gtk_menu_popup_at_pointer (gtk_menu, (GdkEvent*)event);
	}

	gtk_tree_path_free (path);
	return GDK_EVENT_STOP;
}

IrcContextview *
irc_contextview_new (void)
{
	g_autoptr(IrcChanstore) store = irc_chanstore_new ();
	IrcContextview *view = g_object_new (IRC_TYPE_CONTEXTVIEW,
										"model", GTK_TREE_MODEL(store),
										"headers-visible", FALSE,
										"reorderable", TRUE,
										"enable-search", FALSE,
										"can-focus", FALSE, NULL);

	GtkTreeViewColumn *name_column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_sizing (name_column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

	GtkCellRenderer *name_render = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (name_column, name_render, TRUE);
	gtk_tree_view_column_add_attribute (name_column, name_render, "text", COL_NAME);
  	gtk_tree_view_column_add_attribute (name_column, name_render, "foreground", COL_ACTIVE);
	g_object_set (name_render, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	gtk_cell_renderer_text_set_fixed_height_from_font (GTK_CELL_RENDERER_TEXT(name_render), 1);


	GtkCellRenderer *act_render = irc_cellrenderer_bubble_new ();
	gtk_tree_view_column_pack_end (name_column, act_render, FALSE);
	gtk_tree_view_column_add_attribute (name_column, act_render, "activity", COL_ACT);
	gtk_tree_view_column_add_attribute (name_column, act_render, "highlight", COL_HIGHLIGHT);

	gtk_tree_view_insert_column (GTK_TREE_VIEW(view), name_column, 0);

	GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW(view));
	g_signal_connect (sel, "changed", G_CALLBACK(on_selection_changed), NULL);

	return view;
}

static void
irc_contextview_finalize (GObject *object)
{
	G_OBJECT_CLASS (irc_contextview_parent_class)->finalize (object);
}

static void
irc_contextview_class_init (IrcContextviewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);

	object_class->finalize = irc_contextview_finalize;

	wid_class->button_press_event = irc_context_view_button_press_event;
}

static void
irc_contextview_init (IrcContextview *self)
{
	IrcContextManager *mgr = irc_context_manager_get_default ();

  	g_signal_connect (mgr, "context-added", G_CALLBACK(on_context_added), self);
	g_signal_connect (mgr, "context-removed", G_CALLBACK(on_context_removed), self);
	g_signal_connect (mgr, "front-context-changed", G_CALLBACK(on_front_context_changed), self);
}
