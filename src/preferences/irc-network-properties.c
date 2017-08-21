/* irc-network-properties.c
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

#include "irc-network-properties.h"
#include "irc-server.h"
#include "irc-context-manager.h"

struct _IrcNetworkProperties
{
	GtkDialog parent_instance;
};

typedef struct
{
	char *network;
	GtkEntry *hostnameentry, *usernameentry, *nicknameentry, *realnameentry, *saslusernameentry,
		*saslpasswordentry, *passwordentry, *encodingentry;
	GtkSwitch *tlsswitch;
} IrcNetworkPropertiesPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (IrcNetworkProperties, irc_network_properties, GTK_TYPE_DIALOG)

GtkDialog *
irc_network_properties_new (const char *network)
{
	GtkDialog *dlg = g_object_new (IRC_TYPE_NETWORK_PROPERTIES, "title", network,
													"use-header-bar", 1, NULL);
  	IrcNetworkPropertiesPrivate *priv = irc_network_properties_get_instance_private (IRC_NETWORK_PROPERTIES(dlg));
	priv->network = g_strdup (network);

  	g_autofree char *path = g_strconcat ("/se/tingping/IrcClient/", priv->network, "/", NULL);

	g_autoptr(GSettings) settings = g_settings_new_with_path ("se.tingping.network", path);
	g_settings_bind (settings, "hostname", priv->hostnameentry, "text", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (settings, "server-username", priv->usernameentry, "text", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (settings, "server-password", priv->passwordentry, "text", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (settings, "nickname", priv->nicknameentry, "text", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (settings, "realname", priv->realnameentry, "text", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (settings, "sasl-username", priv->saslusernameentry, "text", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (settings, "sasl-password", priv->saslpasswordentry, "text", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (settings, "tls", priv->tlsswitch, "active", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (settings, "encoding", priv->encodingentry, "text", G_SETTINGS_BIND_DEFAULT);

	gtk_dialog_add_button (dlg, "Connect", 1);
	return dlg;
}

static void
irc_network_properties_response (GtkDialog *dialog, int response_id)
{
	if (response_id == 1)
	{
	  	IrcNetworkPropertiesPrivate *priv = irc_network_properties_get_instance_private (IRC_NETWORK_PROPERTIES(dialog));
		IrcContextManager *mgr = irc_context_manager_get_default ();
		g_autoptr(IrcServer) serv = irc_server_new_from_network (priv->network);
		irc_context_manager_add (mgr, IRC_CONTEXT(serv));
	}
}

static void
irc_network_properties_finalize (GObject *object)
{
	IrcNetworkProperties *self = IRC_NETWORK_PROPERTIES(object);
	IrcNetworkPropertiesPrivate *priv = irc_network_properties_get_instance_private (self);

	g_free (priv->network);

	G_OBJECT_CLASS (irc_network_properties_parent_class)->finalize (object);
}

static void
irc_network_properties_class_init (IrcNetworkPropertiesClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *wid_class = GTK_WIDGET_CLASS (klass);
	GtkDialogClass *dlg_class = GTK_DIALOG_CLASS (klass);

	object_class->finalize = irc_network_properties_finalize;
	dlg_class->response = irc_network_properties_response;

	gtk_widget_class_set_template_from_resource (wid_class, "/se/tingping/IrcClient/preferences/irc-network-properties.ui");
  	gtk_widget_class_bind_template_child_private (wid_class, IrcNetworkProperties, hostnameentry);
	gtk_widget_class_bind_template_child_private (wid_class, IrcNetworkProperties, nicknameentry);
	gtk_widget_class_bind_template_child_private (wid_class, IrcNetworkProperties, usernameentry);
	gtk_widget_class_bind_template_child_private (wid_class, IrcNetworkProperties, passwordentry);
	gtk_widget_class_bind_template_child_private (wid_class, IrcNetworkProperties, realnameentry);
	gtk_widget_class_bind_template_child_private (wid_class, IrcNetworkProperties, saslusernameentry);
	gtk_widget_class_bind_template_child_private (wid_class, IrcNetworkProperties, saslpasswordentry);
	gtk_widget_class_bind_template_child_private (wid_class, IrcNetworkProperties, encodingentry);
	gtk_widget_class_bind_template_child_private (wid_class, IrcNetworkProperties, tlsswitch);
}

static void
irc_network_properties_init (IrcNetworkProperties *self)
{
	gtk_widget_init_template (GTK_WIDGET (self));
}
