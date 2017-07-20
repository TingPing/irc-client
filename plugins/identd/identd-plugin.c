/* identd-plugin.c
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
#include "identd-plugin.h"
#include "identd-service.h"

struct _IdentdPlugin
{
	PeasExtensionBase parent_instance;
	GObject *application;
	GHashTable *signals;
	gulong mgr_signal;
};

static IdentdService *identd;

static void peas_activatable_iface_init (PeasActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (IdentdPlugin, identd_plugin, PEAS_TYPE_EXTENSION_BASE, 0,
                                G_IMPLEMENT_INTERFACE(PEAS_TYPE_ACTIVATABLE,
                                                      peas_activatable_iface_init))

static void
on_socket_client_event (GSocketClient *client, GSocketClientEvent  event, GSocketConnectable *connectable,
                        GIOStream *connection, gpointer user_data)
{
	if (event == G_SOCKET_CLIENT_CONNECTED)
	{
		g_autoptr(GError) err = NULL;
		g_autoptr(GInetSocketAddress) addr = G_INET_SOCKET_ADDRESS(
		                                     g_socket_connection_get_local_address (G_SOCKET_CONNECTION(connection), &err));
		if (err != NULL)
		{
			g_warning ("Failed to get local address of connection: %s", err->message);
			return;
		}

		GInetAddress *inet_addr = g_inet_socket_address_get_address (addr);
		g_autofree char *addr_str = g_inet_address_to_string (inet_addr);
		identd_service_add_address (identd, addr_str);

		IrcServer *server = user_data;
		g_autoptr(GSettings) settings;
		g_object_get (server, "settings", &settings, NULL);
		g_autofree char *username = g_settings_get_string (settings, "server-username");

		guint16 local_port = g_inet_socket_address_get_port (addr);
		identd_service_add_user (identd, username, local_port);
	}
}

static void
on_client_deleted (gpointer data, GObject *client)
{
	IdentdPlugin *self = data;
	g_hash_table_remove (self->signals, client);
}

static void
connect_socket_signal (IdentdPlugin *self, IrcContext *server)
{
	g_autoptr(GSocketClient) client;
	g_object_get (server, "socket", &client, NULL);

	gulong signal_id = g_signal_connect (client, "event", G_CALLBACK(on_socket_client_event), server);
	g_hash_table_insert (self->signals, client, GSIZE_TO_POINTER(signal_id));
	g_object_weak_ref (G_OBJECT(client), on_client_deleted, self);
}

static gboolean
on_remove (gpointer key, gpointer value, gpointer data)
{
	g_signal_handler_disconnect (key, GPOINTER_TO_SIZE(value));
	return TRUE;
}

static void
disconnect_socket_signals (IdentdPlugin *self)
{
	g_hash_table_foreach_remove (self->signals, on_remove, NULL);

	if (self->mgr_signal)
	{
		IrcContextManager *mgr = irc_context_manager_get_default ();
		g_signal_handler_disconnect (mgr, self->mgr_signal);
		self->mgr_signal = 0;
	}
}

static void
foreach_parent (GNode *node, gpointer data)
{
	if (IRC_IS_SERVER(node->data))
		connect_socket_signal (data, node->data);
}

static void
on_context_added (IrcContextManager *mgr, IrcContext *ctx, gpointer data)
{
	if (IRC_IS_SERVER(ctx))
		connect_socket_signal (data, ctx);
}

static void
identd_plugin_activate (PeasActivatable *activatable)
{
	g_info ("Activating identd plugin");
	IdentdPlugin *self = IDENTD_PLUGIN(activatable);

	if ((identd = identd_service_new ()))
	{
		IrcContextManager *mgr = irc_context_manager_get_default();
		irc_context_manager_foreach_parent (mgr, foreach_parent, self);
		self->mgr_signal = g_signal_connect (mgr, "context-added", G_CALLBACK(on_context_added), self);
	}
}

static void
identd_plugin_deactivate (PeasActivatable *activatable)
{
	g_info ("Deactivating identd plugin");
	IdentdPlugin *self = IDENTD_PLUGIN(activatable);

	g_clear_pointer (&identd, identd_service_destroy);
	disconnect_socket_signals (self);
}

enum {
	PROP_0,
	PROP_OBJECT
};

static void
identd_plugin_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  IdentdPlugin *self = IDENTD_PLUGIN(object);

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
identd_plugin_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  IdentdPlugin *self = IDENTD_PLUGIN(object);

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
identd_plugin_finalize (GObject *object)
{
	IdentdPlugin *self = (IdentdPlugin *)object;

	g_clear_object (&self->application);
	disconnect_socket_signals (self);
	g_hash_table_destroy (self->signals);

	G_OBJECT_CLASS (identd_plugin_parent_class)->finalize (object);
}

static void
identd_plugin_class_init (IdentdPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = identd_plugin_finalize;
	object_class->get_property = identd_plugin_get_property;
	object_class->set_property = identd_plugin_set_property;

	g_object_class_override_property (object_class, PROP_OBJECT, "object");
}

static void
identd_plugin_class_finalize (IdentdPluginClass *klass)
{
}

static void
identd_plugin_init (IdentdPlugin *self)
{
	self->signals = g_hash_table_new (NULL, NULL);
}

static void
peas_activatable_iface_init (PeasActivatableInterface *iface)
{
  iface->activate = identd_plugin_activate;
  iface->deactivate = identd_plugin_deactivate;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	identd_plugin_register_type (G_TYPE_MODULE (module));

	peas_object_module_register_extension_type (module,
	                                            PEAS_TYPE_ACTIVATABLE,
	                                            IDENTD_TYPE_PLUGIN);
}
