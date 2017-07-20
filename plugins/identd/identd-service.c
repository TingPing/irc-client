/* -identd-service.c
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

#include <string.h>
#include <gio/gio.h>
#include <libgupnp-igd/gupnp-simple-igd.h>
#include "identd-service.h"

typedef struct
{
	IdentdService *service;
	GSocketConnection *conn;
	gchar *username;
} ident_info;

typedef struct
{
	GHashTable *responses;
	guint16 port;
} callback_tuple;

struct IdentdService {
	GSocketService *service;
	GUPnPSimpleIgd *upnp;
	GHashTable *responses;
	guint16 port;
};

static void
stream_close_ready (GObject *source, GAsyncResult *res, gpointer userdata)
{
	GError *err = NULL;

	if (!g_io_stream_close_finish (G_IO_STREAM(source), res, &err))
	{
		g_warning ("%s", err->message);
		g_error_free (err);
	}

	g_object_unref (source);
}

static void
ident_info_free (ident_info *info)
{
	g_return_if_fail (info != NULL);

	g_io_stream_close_async (G_IO_STREAM(info->conn), G_PRIORITY_DEFAULT,
							 NULL, stream_close_ready, NULL);
	g_free (info->username);
	g_free (info);
}

static void
callback_tuple_free (callback_tuple *t)
{
	g_return_if_fail (t != NULL);

	g_hash_table_unref (t->responses);
	g_free (t);
}

static gboolean
identd_cleanup_response_cb (gpointer userdata)
{
	g_return_val_if_fail (userdata != NULL, G_SOURCE_REMOVE);

	callback_tuple *t = userdata;
	g_hash_table_remove (t->responses, GINT_TO_POINTER(t->port));

	return G_SOURCE_REMOVE;
}

void
identd_service_add_user (IdentdService *service, const char *username, const guint16 port)
{
	g_return_if_fail (username != NULL);
	g_return_if_fail (port != 0);
	g_return_if_fail (service != NULL);

	callback_tuple *info = g_new(callback_tuple, 1);
	info->responses = g_hash_table_ref (service->responses);
	info->port = port;

	g_hash_table_insert (service->responses, GINT_TO_POINTER (port), g_strdup (username));
	g_timeout_add_seconds_full (G_PRIORITY_DEFAULT, 30, identd_cleanup_response_cb, info, (GDestroyNotify)callback_tuple_free);

	g_info ("identd: Adding %s (%" G_GUINT16_FORMAT ") to service", username, port);
}

void
identd_service_add_address (IdentdService *service, const char *address)
{
	g_return_if_fail (service != NULL);
	g_return_if_fail (address != NULL);

	// Would set a short lease but my router only supports permanent leases...
	gupnp_simple_igd_add_port (service->upnp, "TCP", 113, address, service->port, 0, "Identd server for  Client");
}

static void
identd_write_ready (GOutputStream *stream, GAsyncResult *res, ident_info *info)
{
	g_output_stream_write_finish (stream, res, NULL);

	ident_info_free (info);
}

static void
identd_read_ready (GDataInputStream *in_stream, GAsyncResult *res, ident_info *info)
{
	GOutputStream *out_stream;
	guint64 local, remote;
	gchar *read_buf, buf[512], *p;

	if ((read_buf = g_data_input_stream_read_line_finish (in_stream, res, NULL, NULL)))
	{
		local = g_ascii_strtoull (read_buf, NULL, 0);
		p = strchr (read_buf, ',');
		if (!p)
		{
			g_free (read_buf);
			goto cleanup;
		}

		remote = g_ascii_strtoull (p + 1, NULL, 0);
		g_free (read_buf);

		g_snprintf (buf, sizeof (buf), "%"G_GUINT16_FORMAT", %"G_GUINT16_FORMAT" : ",
					(guint16)MIN(local, G_MAXUINT16), (guint16)MIN(remote, G_MAXUINT16));

		if (!local || !remote || local > G_MAXUINT16 || remote > G_MAXUINT16)
		{
			g_strlcat (buf, "ERROR : INVALID-PORT\r\n", sizeof (buf));
			g_info ("identd: Received invalid port");
		}
		else
		{
			info->username = g_hash_table_lookup (info->service->responses, GINT_TO_POINTER (local));
			if (!info->username)
			{
				g_strlcat (buf, "ERROR : NO-USER\r\n", sizeof (buf));
				g_info ("identd: Received unknown local port");
			}
			else
			{
				const gsize len = strlen (buf);
				g_autoptr(GSocketAddress) sok_addr;

				g_hash_table_steal (info->service->responses, GINT_TO_POINTER (local));

				g_snprintf (buf + len, sizeof (buf) - len, "USERID : UNIX : %s\r\n", info->username);

				if ((sok_addr = g_socket_connection_get_remote_address (info->conn, NULL)))
				{
					GInetAddress *inet_addr = g_inet_socket_address_get_address (G_INET_SOCKET_ADDRESS (sok_addr));
					g_autofree char *addr = g_inet_address_to_string (inet_addr);

					g_info ("identd: Servicing ident request from %s as %s", addr, info->username);
				}
			}
		}

		out_stream = g_io_stream_get_output_stream (G_IO_STREAM (info->conn));
		g_output_stream_write_async (out_stream, buf, strlen (buf), G_PRIORITY_DEFAULT,
									NULL, (GAsyncReadyCallback)identd_write_ready, info);
	}
	else
	{
		g_debug ("identd: Failed to read incoming line");
		goto cleanup;
	}

	return;

cleanup:
	ident_info_free (info);
}

static gboolean
identd_incoming_cb (GSocketService *service, GSocketConnection *conn,
					GObject *source, gpointer userdata)
{
	IdentdService *identd = userdata;
	GDataInputStream *data_stream;
	GInputStream *stream;
	ident_info *info;

	g_debug ("identd: Incoming");

	info = g_new0 (ident_info, 1);

	info->conn = g_object_ref (conn);
	info->service = identd;

	stream = g_io_stream_get_input_stream (G_IO_STREAM (conn));
	data_stream = g_data_input_stream_new (stream);
	g_data_input_stream_set_newline_type (data_stream, G_DATA_STREAM_NEWLINE_TYPE_CR_LF);
	g_data_input_stream_read_line_async (data_stream, G_PRIORITY_DEFAULT,
										NULL, (GAsyncReadyCallback)identd_read_ready, info);

	return TRUE;
}

static void
mapped_port_cb (GUPnPSimpleIgd *self, char *proto, char *external_ip, char *replaced_ip, guint external_port,
                char *local_ip, guint local_port, char *description, gpointer user_data)
{
	g_info ("identd: Mapped port %s:%u to %s:%u", external_ip, external_port, local_ip, local_port);
}


static void
error_mapping_port_cb (GUPnPSimpleIgd *self, GError *error, char *proto, guint external_port,
                       char *local_ip, guint local_port, char *description, gpointer user_data)
{
	g_warning ("identd: Error mapping port: %s", error->message);
}

void
identd_service_destroy (IdentdService *service)
{
	g_return_if_fail (service != NULL);

	g_socket_service_stop (service->service);
	g_clear_object (&service->service);
	gupnp_simple_igd_remove_port (service->upnp, "TCP", 113);
	g_clear_object (&service->upnp);
	g_hash_table_unref (service->responses);
	g_free (service);
}

IdentdService *
identd_service_new (void)
{
	GError *error = NULL;
	GSocketService *service = g_socket_service_new ();

	guint16 listen_port = g_socket_listener_add_any_inet_port (G_SOCKET_LISTENER (service), NULL, &error);
	if (error)
	{
		g_warning ("Error starting identd server: %s", error->message);

		g_error_free (error);
		g_object_unref (service);
		return NULL;
	}
	g_info ("identd: Listening on port: %" G_GUINT16_FORMAT, listen_port);

	IdentdService *identd = g_new (IdentdService, 1);
	identd->service = service;
	identd->responses = g_hash_table_new_full (NULL, NULL, NULL, g_free);
	identd->upnp = gupnp_simple_igd_new ();
	identd->port = listen_port;

	g_signal_connect(identd->upnp, "error-mapping-port", G_CALLBACK(error_mapping_port_cb), NULL);
	g_signal_connect(identd->upnp, "mapped-external-port", G_CALLBACK(mapped_port_cb), NULL);
	g_signal_connect (service, "incoming", G_CALLBACK(identd_incoming_cb), identd);

	g_socket_service_start (service);

	return identd;
}
