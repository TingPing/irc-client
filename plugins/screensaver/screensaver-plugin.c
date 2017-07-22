/* screensaver-plugin.c
 *
 * Copyright (C) 2017 Patrick Griffis <tingping@tingping.se>
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

#include "irc.h"
#include "screensaver-plugin.h"
#include "screensaver-gnome.h"
#include "screensaver-fdo.h"

struct _ScreensaverPlugin
{
	PeasExtensionBase parent_instance;
	GObject *application;
	GCancellable *cancel;
	ScreensaverGnomeOrgGnomeScreenSaver *gnome;
	ScreensaverFdoOrgFreedesktopScreenSaver *fdo;
};

static void peas_activatable_iface_init (PeasActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (ScreensaverPlugin, screensaver_plugin, PEAS_TYPE_EXTENSION_BASE, 0,
                                G_IMPLEMENT_INTERFACE(PEAS_TYPE_ACTIVATABLE,
                                                      peas_activatable_iface_init))

static void
on_active_changed (GObject *object, gboolean new_value)
{
	IrcContextManager *mgr = irc_context_manager_get_default ();
	IrcContext *ctx = irc_context_manager_get_front_context (mgr);

	if (ctx == NULL)
		return;

	if (new_value)
		irc_context_run_command (ctx, "allserv AWAY :Auto-away");
	else
		irc_context_run_command (ctx, "allserv AWAY");
}

static void
on_gnome_proxy_ready (GObject *source, GAsyncResult *res, gpointer user_data)
{
	g_autoptr(GError) err = NULL;
	ScreensaverPlugin *self = user_data;

	self->gnome = screensaver_gnome_org_gnome_screen_saver_proxy_new_for_bus_finish (res, &err);
	if (err == NULL)
	{
		g_signal_connect (self->gnome, "active_changed", G_CALLBACK(on_active_changed), NULL);
		g_info ("screensaver: Watching gnome screensaver");
	}
	else
		g_debug ("screensaver: %s", err->message);
}

static void
on_freedesktop_proxy_ready (GObject *source, GAsyncResult *res, gpointer user_data)
{
	g_autoptr(GError) err = NULL;
	ScreensaverPlugin *self = user_data;

	self->fdo = screensaver_fdo_org_freedesktop_screen_saver_proxy_new_for_bus_finish (res, &err);
	if (err == NULL)
	{
		g_signal_connect (self->fdo, "active_changed", G_CALLBACK(on_active_changed), NULL);
		g_info ("screensaver: Watching freedesktop screensaver");
	}
	else
		g_debug ("screensaver: %s", err->message);
}

static void
screensaver_plugin_activate (PeasActivatable *activatable)
{
	g_info ("Activating screensaver plugin");
	ScreensaverPlugin *self = SCREENSAVER_PLUGIN(activatable);

	self->cancel = g_cancellable_new ();

	screensaver_gnome_org_gnome_screen_saver_proxy_new_for_bus (G_BUS_TYPE_SESSION,
		G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START|G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START_AT_CONSTRUCTION|G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
		"org.gnome.ScreenSaver", "/org/gnome/ScreenSaver", self->cancel, on_gnome_proxy_ready, self
	);

	screensaver_fdo_org_freedesktop_screen_saver_proxy_new_for_bus (G_BUS_TYPE_SESSION,
		G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START|G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START_AT_CONSTRUCTION|G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
		"org.freedesktop.ScreenSaver", "/org/freedesktop/ScreenSaver", self->cancel, on_freedesktop_proxy_ready, self
	);
}

static void
screensaver_plugin_deactivate (PeasActivatable *activatable)
{
	g_info ("Deactivating screensaver plugin");
	ScreensaverPlugin *self = SCREENSAVER_PLUGIN(activatable);

	if (self->cancel)
	{
		g_cancellable_cancel (self->cancel);
		g_clear_object (&self->cancel);
	}
	g_clear_object (&self->gnome);
	g_clear_object (&self->fdo);
}

enum {
	PROP_0,
	PROP_OBJECT
};

static void
screensaver_plugin_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  ScreensaverPlugin *self = SCREENSAVER_PLUGIN(object);

  switch (prop_id)
    {
    case PROP_OBJECT:
      self->application = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
    }
}

static void
screensaver_plugin_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  ScreensaverPlugin *self = SCREENSAVER_PLUGIN(object);

  switch (prop_id)
    {
    case PROP_OBJECT:
      g_value_set_object (value, self->application);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
    }
}

static void
screensaver_plugin_finalize (GObject *object)
{
	ScreensaverPlugin *self = (ScreensaverPlugin *)object;

	if (self->cancel)
	{
		g_cancellable_cancel (self->cancel);
		g_clear_object (&self->cancel);
	}

	g_clear_object (&self->application);
	g_clear_object (&self->gnome);
	g_clear_object (&self->fdo);

	G_OBJECT_CLASS (screensaver_plugin_parent_class)->finalize (object);
}

static void
screensaver_plugin_class_init (ScreensaverPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = screensaver_plugin_finalize;
	object_class->get_property = screensaver_plugin_get_property;
	object_class->set_property = screensaver_plugin_set_property;

	g_object_class_override_property (object_class, PROP_OBJECT, "object");
}

static void
screensaver_plugin_class_finalize (ScreensaverPluginClass *klass)
{
}

static void
screensaver_plugin_init (ScreensaverPlugin *self)
{
}

static void
peas_activatable_iface_init (PeasActivatableInterface *iface)
{
  iface->activate = screensaver_plugin_activate;
  iface->deactivate = screensaver_plugin_deactivate;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	screensaver_plugin_register_type (G_TYPE_MODULE (module));

	peas_object_module_register_extension_type (module,
	                                            PEAS_TYPE_ACTIVATABLE,
	                                            SCREENSAVER_TYPE_PLUGIN);
}
