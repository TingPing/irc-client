/* irc-application.c
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
#include <libpeas/peas.h>

#include "irc-application.h"
//#include "irc-plugin-engine.h"
#include "irc-window.h"
#include "irc-resources.h"
#include "irc-preferences-window.h"
#include "irc-utils.h"

struct _IrcApplication
{
	GtkApplication parent_instance;
};

typedef struct
{
	GtkWindow *main_window;
	PeasExtensionSet *extensions;
	GSettings *settings;
	GtkCssProvider *css_provider;
} IrcApplicationPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (IrcApplication, irc_application, GTK_TYPE_APPLICATION)


/**
 * irc_application_new:
 * @id: Application id
 *
 * Returns: (transfer full): New #IrcApplication
 */
IrcApplication *
irc_application_new (const char *id)
{
	return g_object_new (IRC_TYPE_APPLICATION, "application-id", id,
						"flags", G_APPLICATION_HANDLES_OPEN, NULL);
}

static void
on_extension_added (PeasExtensionSet *set, PeasPluginInfo *info, PeasExtension *ext, gpointer data)
{
	peas_activatable_activate (PEAS_ACTIVATABLE(ext));
}

static void
on_extension_removed (PeasExtensionSet *set, PeasPluginInfo *info, PeasExtension *ext, gpointer data)
{
	peas_activatable_deactivate (PEAS_ACTIVATABLE(ext));
}

static inline char *
get_custom_plugin_dir (void)
{
	const char *env = g_getenv ("IRC_PLUGIN_DIR");
	if (!env)
		return NULL;

	return g_filename_to_utf8 (env, -1, NULL, NULL, NULL);
}

static void
irc_application_load_plugins (IrcApplication *self)
{
	IrcApplicationPrivate *priv = irc_application_get_instance_private (IRC_APPLICATION(self));
	PeasEngine *engine = peas_engine_get_default();
	g_autofree char *custom_path = get_custom_plugin_dir ();
	if (custom_path)
	{
		peas_engine_add_search_path (engine, custom_path, NULL);
	}
	else
	{
		g_autofree char *plugin_path = g_build_filename (LIBDIR, "irc-client", NULL);
		g_autofree char *user_plugin_path = g_build_filename (g_get_home_dir (), ".local", "lib", "irc-client", NULL);

		peas_engine_add_search_path (engine, user_plugin_path, NULL);
		peas_engine_add_search_path (engine, plugin_path, NULL);
	}
	priv->extensions = peas_extension_set_new (engine, PEAS_TYPE_ACTIVATABLE,
												"object", self, NULL);

  	g_settings_bind (priv->settings, "enabled-plugins", engine, "loaded-plugins", G_SETTINGS_BIND_DEFAULT);
	peas_extension_set_foreach (priv->extensions, on_extension_added, self);
	g_signal_connect (priv->extensions, "extension-added", G_CALLBACK(on_extension_added), self);
	g_signal_connect (priv->extensions, "extension-removed", G_CALLBACK(on_extension_removed), self);
}

static void
irc_application_about (GSimpleAction *action, GVariant *param, gpointer data)
{
  	IrcApplication *self = IRC_APPLICATION(data);
	IrcApplicationPrivate *priv = irc_application_get_instance_private (IRC_APPLICATION(self));

	gtk_show_about_dialog (priv->main_window,
		"program-name", PACKAGE_NAME,
		"version", PACKAGE_VERSION,
		"license-type", GTK_LICENSE_GPL_3_0,
		NULL);
}

static void
irc_application_quit (GSimpleAction *action, GVariant *param, gpointer data)
{
	g_application_quit (G_APPLICATION(data));
}

static void
irc_application_focus_context (GSimpleAction *action, GVariant *param, gpointer data)
{
	IrcApplication *self = IRC_APPLICATION(data);
	IrcApplicationPrivate *priv = irc_application_get_instance_private (IRC_APPLICATION(self));
	GActionGroup *actions = gtk_widget_get_action_group (GTK_WIDGET(priv->main_window), "context");
	g_assert (actions != NULL);
	g_action_group_activate_action (actions, "focus", param);
}

static char *
css_from_pango_description (const char *description)
{
	g_autofree char *family = NULL;
	guint64 size = 0;
	char *p;

	// TODO: Probably should handle weight also?
	g_assert (description != NULL);
	if ((p = strrchr (description, ' ')))
	{
		size = g_ascii_strtoull (p + 1, NULL, 0);
		if (size)
			family = g_strndup (description, (gsize)(p - description));
	}
	if (!family)
		family = g_strdup (description);

	if (size)
		return g_strdup_printf (".irc-textview { font-family: %s; font-size: %"G_GUINT64_FORMAT "px }",
								family, size);
	else
		return g_strdup_printf (".irc-textview { font-family: %s }", family);
}

static void
font_changed (GObject *obj, GParamSpec *spec, gpointer data)
{
  	IrcApplication *self = IRC_APPLICATION(data);
	IrcApplicationPrivate *priv = irc_application_get_instance_private (self);
	g_autofree char *font = g_settings_get_string (priv->settings, "font");
	g_autofree char *css = css_from_pango_description (font);

	if (!gtk_css_provider_load_from_data (priv->css_provider, css, -1, NULL))
		g_warning ("Failed loading font css \"%s\"", css);
}

static void
irc_application_preferences (GSimpleAction *action, GVariant *param, gpointer data)
{
  	IrcApplication *self = IRC_APPLICATION(data);
	IrcApplicationPrivate *priv = irc_application_get_instance_private (self);
	GtkWidget *wid = irc_preferences_window_new ();
	gtk_window_set_transient_for (GTK_WINDOW(wid), priv->main_window);

	// This doesn't actually make this window modal
	// but it makes the child dialog of the color chooser modal...
	gtk_window_set_modal (GTK_WINDOW(wid), TRUE);
	gtk_window_present (GTK_WINDOW(wid));
}

static GActionEntry app_entries[] = {
	{ .name = "about", .activate = irc_application_about },
	{ .name = "quit", .activate = irc_application_quit },
	{ .name = "preferences", .activate = irc_application_preferences },
	{ .name = "focus-context", .activate = irc_application_focus_context, .parameter_type = "s" },
};

static void
irc_application_activate (GApplication *self)
{
  	IrcApplicationPrivate *priv = irc_application_get_instance_private (IRC_APPLICATION(self));

	if (priv->main_window == NULL)
	{
		priv->main_window = GTK_WINDOW(irc_window_new (self));
		gtk_style_context_add_provider_for_screen (gtk_window_get_screen (priv->main_window),
											 GTK_STYLE_PROVIDER(priv->css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		irc_application_load_plugins (IRC_APPLICATION(self)); // In-case plugins modify UI
	}

	gtk_window_present (priv->main_window);
}

static void
irc_application_open (GApplication *self, GFile **files, int n_files, const char *hint)
{
	for (int i = 0; i < n_files; ++i)
	{
		g_autofree char *uri = g_file_get_uri_scheme (files[i]);

		if (!g_str_equal (uri, "irc") && !g_str_equal (uri, "ircs"))
		{
			g_warning ("Recived invalid uri to open");
			continue;
		}

		g_info ("Opening");
	}
}

static void
irc_application_startup (GApplication *self)
{
	g_resources_register (irc_get_resource ());
	g_application_set_resource_base_path (self, "/se/tingping/IrcClient");

    G_APPLICATION_CLASS (irc_application_parent_class)->startup (self);
}

static void
irc_application_finalize (GObject *object)
{
	IrcApplication *self = IRC_APPLICATION(object);
	IrcApplicationPrivate *priv = irc_application_get_instance_private (self);

	g_clear_object (&priv->extensions);
	g_clear_object (&priv->settings);
	g_clear_object (&priv->css_provider);

	G_OBJECT_CLASS (irc_application_parent_class)->finalize (object);
}

static void
irc_application_class_init (IrcApplicationClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

	object_class->finalize = irc_application_finalize;
	app_class->activate = irc_application_activate;
	app_class->open = irc_application_open;
	app_class->startup = irc_application_startup;
}

static void
irc_application_init (IrcApplication *self)
{
	IrcApplicationPrivate *priv = irc_application_get_instance_private (IRC_APPLICATION(self));
	//priv->engine = irc_plugin_engine_get_default ();
	priv->settings = g_settings_new ("se.tingping.IrcClient");

	g_action_map_add_action_entries (G_ACTION_MAP(self), app_entries, G_N_ELEMENTS(app_entries), self);

	priv->css_provider = gtk_css_provider_new ();
	g_signal_connect (priv->settings, "changed::font", G_CALLBACK(font_changed), self);
	font_changed (G_OBJECT(priv->settings), NULL, self);
}
