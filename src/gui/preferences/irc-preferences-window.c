/* irc-preferences-window.c
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

#include <libpeas-gtk/peas-gtk.h>
#include <glib/gi18n.h>

#include "irc-preferences-window.h"
#include "irc-network-properties.h"
#include "irc-utils.h"

struct _IrcPreferencesWindow
{
	GtkWindow parent_instance;
};

typedef struct
{
	GtkSwitch *joinpart_switch, *stripcolor_switch;
	GtkTreeSelection *network_selection;
	GtkListStore *networklist;
	GtkFontButton *fontbutton;
	GtkColorButton *colorbutton00, *colorbutton01, *colorbutton02, *colorbutton03, *colorbutton04,
		*colorbutton05, *colorbutton06, *colorbutton07, *colorbutton08, *colorbutton09, *colorbutton10,
		*colorbutton11, *colorbutton12, *colorbutton13, *colorbutton14, *colorbutton15;
	PeasGtkPluginManager *pluginmanager;
} IrcPreferencesWindowPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (IrcPreferencesWindow, irc_preferences_window, GTK_TYPE_WINDOW)

GtkWidget *
irc_preferences_window_new (void)
{
  return g_object_new (IRC_TYPE_PREFERENCES_WINDOW, NULL);
}

static gboolean
get_color (GValue *value, GVariant *variant, gpointer user_data)
{
	GdkRGBA color;

	if (!gdk_rgba_parse (&color, g_variant_get_string (variant, NULL)))
		return FALSE;

	g_value_set_boxed (value, &color);
	return TRUE;
}

static GVariant *
set_color (const GValue *value, const GVariantType *expected_type, gpointer user_data)
{
	GdkRGBA *color = g_value_get_boxed (value);
	g_autofree char *color_str = gdk_rgba_to_string (color);
	return g_variant_new_string (color_str);
}

static char *
get_selected_network (IrcPreferencesWindow *self, GtkTreeIter *out_iter)
{
	IrcPreferencesWindowPrivate *priv = irc_preferences_window_get_instance_private (self);
	GtkTreeIter iter;

	if (!gtk_tree_selection_get_selected (priv->network_selection, NULL, &iter))
		return NULL;

	if (out_iter)
		*out_iter = iter;
	gchar *network;
	gtk_tree_model_get (GTK_TREE_MODEL(priv->networklist), &iter, 0, &network, -1);
	return network;
}

static void
configurebutton_clicked_cb (GtkToolButton *btn, gpointer data)
{
	IrcPreferencesWindow *self = IRC_PREFERENCES_WINDOW(data);
	g_autofree char *network = get_selected_network (self, NULL);

	if (!network)
		return;

	GtkDialog *dlg = irc_network_properties_new (network);
	gtk_window_set_transient_for (GTK_WINDOW(dlg), GTK_WINDOW(self));
	gtk_window_present (GTK_WINDOW(dlg));
}

static void
addbutton_clicked_cb (GtkToolButton *btn, gpointer data)
{
	IrcPreferencesWindow *self = IRC_PREFERENCES_WINDOW(data);
  	IrcPreferencesWindowPrivate *priv = irc_preferences_window_get_instance_private (self);
	gtk_list_store_insert_with_values (priv->networklist, NULL, -1, 0, "temp", -1);
	// TODO: Select and edit
}

static void
removebutton_clicked_cb (GtkToolButton *btn, gpointer data)
{
	IrcPreferencesWindow *self = IRC_PREFERENCES_WINDOW(data);
  	IrcPreferencesWindowPrivate *priv = irc_preferences_window_get_instance_private (self);
	GtkTreeIter iter;
	g_autofree char *network = get_selected_network (self, &iter);

	if (!network)
		return;

	gtk_list_store_remove (priv->networklist, &iter);

  	g_autofree char *path = g_strconcat ("/se/tingping/IrcClient/", network, "/", NULL);
	g_autoptr(GSettings) settings = g_settings_new_with_path ("se.tingping.network", path);
	g_autoptr(GSettingsSchema) schema;
	g_object_get (settings, "settings-schema", &schema, NULL);
	g_auto(GStrv) keys = g_settings_schema_list_keys (schema);
	for (gsize i = 0; keys[i]; ++i)
	{
		g_settings_reset (settings, keys[i]);
	}
}

static void
network_name_edited_cb (GtkCellRendererText *render, char *pathstr, char *new_text, gpointer data)
{
	GtkTreeModel *model = GTK_TREE_MODEL(data);
	GtkTreePath *path = gtk_tree_path_new_from_string (pathstr);
	GtkTreeIter iter;

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_list_store_set (GTK_LIST_STORE(model), &iter, 0, new_text, -1);
	// TODO: Don't allow duplicates
	// TODO: Move settings

	gtk_tree_path_free (path);
}

static void
load_networklist (IrcPreferencesWindow *self)
{
  	IrcPreferencesWindowPrivate *priv = irc_preferences_window_get_instance_private (self);
	g_autoptr(GSettings) settings = g_settings_new ("se.tingping.IrcClient");
	g_auto(GStrv) networks = g_settings_get_strv (settings, "networks");

	for (gsize i = 0; networks[i]; ++i)
	{
		gtk_list_store_insert_with_values (priv->networklist, NULL, -1, 0, networks[i], -1);
	}
}

static void
save_networklist (IrcPreferencesWindow *self)
{
  	IrcPreferencesWindowPrivate *priv = irc_preferences_window_get_instance_private (self);
	GtkTreeModel *model = GTK_TREE_MODEL(priv->networklist);
	GtkTreeIter iter;
	g_auto (GStrv) networks = NULL;

	if (!gtk_tree_model_get_iter_first (model, &iter))
		return;

	do {
		char *network;
		gtk_tree_model_get (model, &iter, 0, &network, -1);
		networks = irc_strv_append (networks, network); // TODO: Scales up poorly
	} while (gtk_tree_model_iter_next (model, &iter));

  	g_autoptr(GSettings) settings = g_settings_new ("se.tingping.IrcClient");
	g_settings_set_strv (settings, "networks", (const char* const *)networks);
}

static void
irc_preferences_window_destroy (GtkWidget *widget)
{
	IrcPreferencesWindow *self = IRC_PREFERENCES_WINDOW(widget);

	save_networklist (self);

	GTK_WIDGET_CLASS (irc_preferences_window_parent_class)->destroy (widget);
}

static void
irc_preferences_window_class_init (IrcPreferencesWindowClass *klass)
{
	//GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	widget_class->destroy = irc_preferences_window_destroy;

	gtk_widget_class_set_template_from_resource (widget_class, "/se/tingping/IrcClient/ui/preferences.ui");

	gtk_widget_class_bind_template_child_private (widget_class, IrcPreferencesWindow, joinpart_switch);
  	gtk_widget_class_bind_template_child_private (widget_class, IrcPreferencesWindow, stripcolor_switch);
	gtk_widget_class_bind_template_child_private (widget_class, IrcPreferencesWindow, networklist);
	gtk_widget_class_bind_template_child_private (widget_class, IrcPreferencesWindow, network_selection);
	gtk_widget_class_bind_template_child_private (widget_class, IrcPreferencesWindow, pluginmanager);
	gtk_widget_class_bind_template_child_private (widget_class, IrcPreferencesWindow, fontbutton);
  	gtk_widget_class_bind_template_callback (widget_class, configurebutton_clicked_cb);
	gtk_widget_class_bind_template_callback (widget_class, removebutton_clicked_cb);
	gtk_widget_class_bind_template_callback (widget_class, addbutton_clicked_cb);
	gtk_widget_class_bind_template_callback (widget_class, network_name_edited_cb);
	gtk_widget_class_bind_template_child_private (widget_class, IrcPreferencesWindow, colorbutton00);
	gtk_widget_class_bind_template_child_private (widget_class, IrcPreferencesWindow, colorbutton01);
	gtk_widget_class_bind_template_child_private (widget_class, IrcPreferencesWindow, colorbutton02);
	gtk_widget_class_bind_template_child_private (widget_class, IrcPreferencesWindow, colorbutton03);
	gtk_widget_class_bind_template_child_private (widget_class, IrcPreferencesWindow, colorbutton04);
	gtk_widget_class_bind_template_child_private (widget_class, IrcPreferencesWindow, colorbutton05);
	gtk_widget_class_bind_template_child_private (widget_class, IrcPreferencesWindow, colorbutton06);
	gtk_widget_class_bind_template_child_private (widget_class, IrcPreferencesWindow, colorbutton07);
	gtk_widget_class_bind_template_child_private (widget_class, IrcPreferencesWindow, colorbutton08);
	gtk_widget_class_bind_template_child_private (widget_class, IrcPreferencesWindow, colorbutton09);
	gtk_widget_class_bind_template_child_private (widget_class, IrcPreferencesWindow, colorbutton10);
	gtk_widget_class_bind_template_child_private (widget_class, IrcPreferencesWindow, colorbutton11);
	gtk_widget_class_bind_template_child_private (widget_class, IrcPreferencesWindow, colorbutton12);
	gtk_widget_class_bind_template_child_private (widget_class, IrcPreferencesWindow, colorbutton13);
	gtk_widget_class_bind_template_child_private (widget_class, IrcPreferencesWindow, colorbutton14);
	gtk_widget_class_bind_template_child_private (widget_class, IrcPreferencesWindow, colorbutton15);
}

static void
irc_preferences_window_init (IrcPreferencesWindow *self)
{
	IrcPreferencesWindowPrivate *priv = irc_preferences_window_get_instance_private (self);
	g_type_ensure (PEAS_GTK_TYPE_PLUGIN_MANAGER_VIEW);
	gtk_widget_init_template (GTK_WIDGET (self));

	g_autoptr(GSettings) settings = g_settings_new_with_path ("se.tingping.context", "/se/tingping/IrcClient/");
	g_settings_bind (settings, "hide-joinpart", priv->joinpart_switch, "active", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (settings, "stripcolor", priv->stripcolor_switch, "active", G_SETTINGS_BIND_DEFAULT);

	g_autoptr(GSettings) color_settings = g_settings_new_with_path ("se.tingping.theme", "/se/tingping/IrcClient/");
	g_settings_bind_with_mapping (color_settings, "color00", priv->colorbutton00, "rgba", G_SETTINGS_BIND_DEFAULT,
										get_color, set_color, NULL, NULL);
  	g_settings_bind_with_mapping (color_settings, "color01", priv->colorbutton01, "rgba", G_SETTINGS_BIND_DEFAULT,
										get_color, set_color, NULL, NULL);
  	g_settings_bind_with_mapping (color_settings, "color02", priv->colorbutton02, "rgba", G_SETTINGS_BIND_DEFAULT,
										get_color, set_color, NULL, NULL);
  	g_settings_bind_with_mapping (color_settings, "color03", priv->colorbutton03, "rgba", G_SETTINGS_BIND_DEFAULT,
										get_color, set_color, NULL, NULL);
  	g_settings_bind_with_mapping (color_settings, "color04", priv->colorbutton04, "rgba", G_SETTINGS_BIND_DEFAULT,
										get_color, set_color, NULL, NULL);
  	g_settings_bind_with_mapping (color_settings, "color05", priv->colorbutton05, "rgba", G_SETTINGS_BIND_DEFAULT,
										get_color, set_color, NULL, NULL);
  	g_settings_bind_with_mapping (color_settings, "color06", priv->colorbutton06, "rgba", G_SETTINGS_BIND_DEFAULT,
										get_color, set_color, NULL, NULL);
  	g_settings_bind_with_mapping (color_settings, "color07", priv->colorbutton07, "rgba", G_SETTINGS_BIND_DEFAULT,
										get_color, set_color, NULL, NULL);
  	g_settings_bind_with_mapping (color_settings, "color08", priv->colorbutton08, "rgba", G_SETTINGS_BIND_DEFAULT,
										get_color, set_color, NULL, NULL);
  	g_settings_bind_with_mapping (color_settings, "color09", priv->colorbutton09, "rgba", G_SETTINGS_BIND_DEFAULT,
										get_color, set_color, NULL, NULL);
  	g_settings_bind_with_mapping (color_settings, "color10", priv->colorbutton10, "rgba", G_SETTINGS_BIND_DEFAULT,
										get_color, set_color, NULL, NULL);
  	g_settings_bind_with_mapping (color_settings, "color11", priv->colorbutton11, "rgba", G_SETTINGS_BIND_DEFAULT,
										get_color, set_color, NULL, NULL);
  	g_settings_bind_with_mapping (color_settings, "color12", priv->colorbutton12, "rgba", G_SETTINGS_BIND_DEFAULT,
										get_color, set_color, NULL, NULL);
	g_settings_bind_with_mapping (color_settings, "color13", priv->colorbutton13, "rgba", G_SETTINGS_BIND_DEFAULT,
										get_color, set_color, NULL, NULL);
  	g_settings_bind_with_mapping (color_settings, "color14", priv->colorbutton14, "rgba", G_SETTINGS_BIND_DEFAULT,
										get_color, set_color, NULL, NULL);
  	g_settings_bind_with_mapping (color_settings, "color15", priv->colorbutton15, "rgba", G_SETTINGS_BIND_DEFAULT,
										get_color, set_color, NULL, NULL);

	gtk_widget_show_all (GTK_WIDGET(priv->pluginmanager));

	g_autoptr(GSettings) global_settings = g_settings_new ("se.tingping.IrcClient");
	g_settings_bind (global_settings, "font", priv->fontbutton, "font-name", G_SETTINGS_BIND_DEFAULT);

	load_networklist (self);
}
