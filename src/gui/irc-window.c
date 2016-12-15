/* irc-window.c
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

#include <gspell/gspell.h>

#include "irc-utils.h"
#include "irc-user.h"
#include "irc-userlist.h"
#include "irc-server.h"
#include "irc-channel.h"
#include "irc-context.h"
#include "irc-context-manager.h"
#include "irc-textview.h"
#include "irc-chatview.h"
#include "irc-window.h"
#include "irc-chanstore.h"
#include "irc-contextview.h"
#include "irc-entry.h"
#include "irc-entrybuffer.h"

typedef struct
{
	IrcChatview *tab;
	IrcTextview *view;
	GtkWidget *popover;
	IrcEntrybuffer *entrybuffer;
} IrcContextUI;

static void
irc_ctx_ui_free (IrcContextUI *ctx_ui)
{
	if (ctx_ui != NULL)
	{
		// View will be destroyed by tab
		if (ctx_ui->popover != NULL)
			g_object_unref (ctx_ui->popover);
		if (ctx_ui->tab)
			gtk_widget_destroy (GTK_WIDGET(ctx_ui->tab));
		g_object_unref (ctx_ui->entrybuffer);
		g_free (ctx_ui);
	}
}


struct _IrcWindow
{
	GtkApplicationWindow parent_instance;
};

typedef struct
{
	GtkStack *viewstack;
	GtkMenuButton *usersbutton;
	GtkHeaderBar *headerbar;
	GtkScrolledWindow *sw_cv;
	IrcEntry *entry;
	GtkFrame *entry_frame;
	GtkPaned *paned;
	GtkRevealer *search_revealer;
	GtkSearchEntry *search_entry;
} IrcWindowPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (IrcWindow, irc_window, GTK_TYPE_APPLICATION_WINDOW)

static void
on_context_print (IrcContext *ctx, const char *line, gint64 stamp, gpointer data)
{
	IrcContextUI *ctx_ui = g_object_get_data (G_OBJECT(ctx), "ctx-ui");
	if (ctx_ui == NULL)
	{
		g_warning ("No tab for given context");
		return;
	}

	irc_textview_append_text (ctx_ui->view, line, (time_t)stamp);

  	/*IrcContextManager *mgr = irc_context_manager_get_default ();
	if (ctx == irc_context_manager_get_front_context (mgr))
		return;*/
}

static void
on_context_added (IrcContextManager *mgr, IrcContext *ctx, gpointer data)
{
	IrcWindow *self = IRC_WINDOW(data);
  	IrcWindowPrivate *priv = irc_window_get_instance_private (self);
	IrcContextUI *ctx_ui;

	ctx_ui = g_new (IrcContextUI, 1);
	ctx_ui->tab = irc_chatview_new ();
	ctx_ui->view = irc_textview_new ();
	ctx_ui->entrybuffer = irc_entrybuffer_new ();
	ctx_ui->popover = NULL;

	gtk_container_add (GTK_CONTAINER(ctx_ui->tab), GTK_WIDGET(ctx_ui->view));
	gtk_widget_show_all (GTK_WIDGET(ctx_ui->tab));
	gtk_container_add (GTK_CONTAINER(priv->viewstack), GTK_WIDGET(ctx_ui->tab));
	g_object_set_data_full (G_OBJECT(ctx), "ctx-ui", ctx_ui, (GDestroyNotify)irc_ctx_ui_free);

	g_signal_connect (ctx, "print", G_CALLBACK(on_context_print), NULL);

	if (IRC_IS_SERVER(ctx)) // Temp
	{
		irc_server_connect (IRC_SERVER(ctx));
	}
}

static void
update_topic (GObject *obj, GParamSpec *spec, gpointer data)
{
	IrcChannel *channel = IRC_CHANNEL(obj);
	GtkHeaderBar *header = GTK_HEADER_BAR(data);
	char *topic;

	g_object_get (channel, "topic", &topic, NULL);
	if (topic != NULL)
	{
		g_autofree char *stripped = irc_strip_attributes (topic);
		gtk_header_bar_set_subtitle (header, stripped);
		//gtk_widget_set_tooltip_text (GTK_WIDGET(header), stripped);
	}
}

static void
on_front_context_changed (IrcContextManager *mgr, IrcContext *ctx, gpointer data)
{
  	IrcWindow *self = IRC_WINDOW(data);
  	IrcWindowPrivate *priv = irc_window_get_instance_private (self);
	IrcContextUI *ctx_ui = g_object_get_data (G_OBJECT(ctx), "ctx-ui");

	// TODO: Change selection on front changed.
	if (ctx_ui == NULL)
	{
		g_warning ("Front changed to invalid context");
	}
	else
	{
		gtk_stack_set_visible_child (priv->viewstack, GTK_WIDGET(ctx_ui->tab));
		gtk_entry_set_buffer (GTK_ENTRY(priv->entry), GTK_ENTRY_BUFFER(ctx_ui->entrybuffer));
		if (IRC_IS_CHANNEL (ctx) && ctx_ui->popover == NULL)
		{
			ctx_ui->popover = GTK_WIDGET(g_object_ref (
									irc_userlist_new (G_LIST_MODEL(irc_channel_get_users (IRC_CHANNEL(ctx))))));
		}
		gtk_menu_button_set_popover (priv->usersbutton, ctx_ui->popover);
		gtk_header_bar_set_title (priv->headerbar, irc_context_get_name (ctx));
		if (IRC_IS_CHANNEL(ctx))
		{
			//irc_entry_set_completion_model (priv->entry, GTK_TREE_MODEL(irc_channel_get_users (IRC_CHANNEL(ctx))));
			// FIXME: The topic changes
			update_topic (G_OBJECT(ctx), NULL, priv->headerbar);
		}
		else
		{
			gtk_header_bar_set_subtitle (priv->headerbar, NULL);
		}
	}
}

IrcWindow *
irc_window_new (GApplication *app)
{
	return g_object_new (IRC_TYPE_WINDOW, "application", app, NULL);
}

static void
restore_window (IrcWindow *self)
{
	g_autoptr(GSettings) settings = g_settings_new ("se.tingping.IrcClient");
	int width, height, x, y;
	g_autoptr(GVariant) size = g_settings_get_value (settings, "window-size");
	g_autoptr(GVariant) pos = g_settings_get_value (settings, "window-pos");

	g_variant_get (size, "(ii)", &width, &height);
	g_variant_get (pos, "(ii)", &x, &y);

	if (width && height)
		gtk_window_resize (GTK_WINDOW(self), width, height);
	if (x && y)
		gtk_window_move (GTK_WINDOW(self), x, y);
}

static gboolean
irc_window_configure_event (GtkWidget *widget, GdkEventConfigure *event)
{
	g_autoptr(GSettings) settings = g_settings_new ("se.tingping.IrcClient");
	GVariant * const size[] = {
		g_variant_new_int32 (event->width),
		g_variant_new_int32 (event->height),
	};
	GVariant * const pos[] = {
		g_variant_new_int32 (event->x),
		g_variant_new_int32 (event->y),
	};

	g_settings_set_value (settings, "window-size", g_variant_new_tuple (size, G_N_ELEMENTS(size)));
	g_settings_set_value (settings, "window-pos", g_variant_new_tuple (pos, G_N_ELEMENTS(pos)));
	return GTK_WIDGET_CLASS(irc_window_parent_class)->configure_event (widget, event);
}

static IrcTextview *
get_front_view (IrcWindow *self)
{
	IrcContextManager *mgr = irc_context_manager_get_default ();
	IrcContext *ctx = irc_context_manager_get_front_context (mgr);

	if (ctx == NULL)
	{
		g_warning ("Search on invalid context");
		return NULL;
	}

	IrcContextUI *ui = g_object_get_data (G_OBJECT(ctx), "ctx-ui");
	if (ui == NULL)
	{
		g_warning ("Search on invalid context ui");
		return NULL;
	}

	return ui->view;
}

static void
on_search_next (GtkWidget *wid, gpointer data)
{
	IrcWindow *self = IRC_WINDOW(data);
	IrcTextview *view = get_front_view (self);
	if (!view)
		return;

	GActionGroup *group = gtk_widget_get_action_group (GTK_WIDGET(view), "textview");
	g_action_group_activate_action (group, "search-next", NULL);
}

static void
on_search_previous (GtkWidget *wid, gpointer data)
{
	IrcWindow *self = IRC_WINDOW(data);
	IrcTextview *view = get_front_view (self);
	if (!view)
		return;

	GActionGroup *group = gtk_widget_get_action_group (GTK_WIDGET(view), "textview");
	g_action_group_activate_action (group, "search-previous", NULL);
}

static void
on_search_changed (GtkSearchEntry *entry, gpointer data)
{
	IrcWindow *self = IRC_WINDOW(data);
	IrcTextview *view = get_front_view (self);
	if (!view)
		return;

	irc_textview_set_search (view, gtk_entry_get_text (GTK_ENTRY(entry)));
}

static void
on_search_state_changed (GSimpleAction *action, GVariant *param, gpointer data)
{
	IrcWindow *self = IRC_WINDOW(data);
	IrcWindowPrivate *priv = irc_window_get_instance_private (self);
	gboolean search = g_variant_get_boolean (param);
	g_simple_action_set_state (action, param);

	gtk_revealer_set_reveal_child (priv->search_revealer, search);
	if (search)
		gtk_widget_grab_focus (GTK_WIDGET(priv->search_entry));
	else
	{
		IrcTextview *view = get_front_view (self);
		if (!view)
			return;
		irc_textview_set_search (view, NULL);
	}
}

static void
irc_window_finalize (GObject *object)
{
	//IrcWindow *self = IRC_WINDOW(object);
	//IrcWindowPrivate *priv = irc_window_get_instance_private (self);

	G_OBJECT_CLASS (irc_window_parent_class)->finalize (object);
}

static void
irc_window_class_init (IrcWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);

	object_class->finalize = irc_window_finalize;
	wid_class->configure_event = irc_window_configure_event;

	gtk_widget_class_set_template_from_resource (wid_class, "/se/tingping/IrcClient/ui/window.ui");
	gtk_widget_class_bind_template_child_private (wid_class, IrcWindow, viewstack);
	gtk_widget_class_bind_template_child_private (wid_class, IrcWindow, usersbutton);
  	gtk_widget_class_bind_template_child_private (wid_class, IrcWindow, headerbar);
	gtk_widget_class_bind_template_child_private (wid_class, IrcWindow, sw_cv);
  	gtk_widget_class_bind_template_child_private (wid_class, IrcWindow, search_revealer);
  	gtk_widget_class_bind_template_child_private (wid_class, IrcWindow, search_entry);
	gtk_widget_class_bind_template_child_private (wid_class, IrcWindow, entry_frame);
	gtk_widget_class_bind_template_child_private (wid_class, IrcWindow, paned);
	gtk_widget_class_bind_template_callback (wid_class, on_search_next);
	gtk_widget_class_bind_template_callback (wid_class, on_search_previous);
  	gtk_widget_class_bind_template_callback (wid_class, on_search_changed);
}

static void
irc_window_init (IrcWindow *self)
{
	gtk_widget_init_template (GTK_WIDGET(self));

	// Hard coding is a little ugly..
	gtk_widget_insert_action_group (GTK_WIDGET(self), "context", irc_context_get_action_group ());
	gtk_widget_insert_action_group (GTK_WIDGET(self), "channel", irc_channel_get_action_group ());
	gtk_widget_insert_action_group (GTK_WIDGET(self), "server", irc_server_get_action_group ());
	//gtk_widget_insert_action_group (GTK_WIDGET(self), "query", irc_channel_get_action_group ());

	GSimpleAction *action = g_simple_action_new_stateful ("search", NULL, g_variant_new_boolean (FALSE));
	g_signal_connect (action, "change-state", G_CALLBACK(on_search_state_changed), self);
	g_action_map_add_action (G_ACTION_MAP(self), G_ACTION(action));
	const char * const search_accels[] = { "<Primary>f", NULL };
	GtkApplication *app = GTK_APPLICATION(g_application_get_default ());
	gtk_application_set_accels_for_action (app, "win.search", search_accels);

	IrcWindowPrivate *priv = irc_window_get_instance_private (self);
	GtkWidget *chanview = GTK_WIDGET (irc_contextview_new ());
	gtk_container_add (GTK_CONTAINER(priv->sw_cv), chanview);
	gtk_widget_show_all (GTK_WIDGET(priv->sw_cv));

	priv->entry = irc_entry_new ();
	GspellEntry *spell_entry = gspell_entry_get_from_gtk_entry (GTK_ENTRY(priv->entry));
	gspell_entry_basic_setup (spell_entry);
	g_object_set (priv->entry, "margin", 5, NULL);
	gtk_container_add (GTK_CONTAINER(priv->entry_frame), GTK_WIDGET(priv->entry));
	gtk_widget_show (GTK_WIDGET(priv->entry));

	IrcContextManager *mgr = irc_context_manager_get_default ();
	g_signal_connect (mgr, "context-added", G_CALLBACK(on_context_added), self);
	g_signal_connect (mgr, "front-context-changed", G_CALLBACK(on_front_context_changed), self);

	g_autoptr(GSettings) settings = g_settings_new ("se.tingping.IrcClient");
	g_settings_bind (settings, "contextview-width", priv->paned, "position", G_SETTINGS_BIND_DEFAULT);

	restore_window (self);
}
