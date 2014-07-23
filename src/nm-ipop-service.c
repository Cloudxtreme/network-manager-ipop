/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* nm-ipop-service - ipop integration with NetworkManager
 *
 * Copyright (C) 2005 - 2008 Tim Niemueller <tim@niemueller.de>
 * Copyright (C) 2005 - 2010 Dan Williams <dcbw@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * $Id: nm-ipop-service.c 4232 2008-10-29 09:13:40Z tambeti $
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-glib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>

#include <NetworkManager.h>
#include <NetworkManagerVPN.h>
#include <nm-setting-vpn.h>

#include "nm-ipop-service.h"
#include "nm-utils.h"
#include "common/utils.h"

#if !defined(DIST_VERSION)
# define DIST_VERSION VERSION
#endif

static gboolean debug = FALSE;
static GMainLoop *loop = NULL;

#define NM_IPOP_HELPER_PATH		LIBEXECDIR"/nm-ipop-service-ipop-helper"

G_DEFINE_TYPE (NMIPOPPlugin, nm_ipop_plugin, NM_TYPE_VPN_PLUGIN)

#define NM_IPOP_PLUGIN_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), NM_TYPE_IPOP_PLUGIN, NMIPOPPluginPrivate))

typedef struct {
	char *username;
	char *password;
	char *priv_key_pass;
	char *proxy_username;
	char *proxy_password;
	GIOChannel *socket_channel;
	guint socket_channel_eventid;
} NMIPOPPluginIOData;

typedef struct {
	GPid	pid;
	guint connect_timer;
	guint connect_count;
	NMIPOPPluginIOData *io_data;
} NMIPOPPluginPrivate;

typedef struct {
	const char *name;
	GType type;
	gint int_min;
	gint int_max;
	gboolean address;
} ValidProperty;

static ValidProperty valid_properties[] = {
	{ NM_IPOP_KEY_AUTH,                 G_TYPE_STRING, 0, 0, FALSE },
	{ NM_IPOP_KEY_CA,                   G_TYPE_STRING, 0, 0, FALSE },
	{ NM_IPOP_KEY_CERT,                 G_TYPE_STRING, 0, 0, FALSE },
	{ NM_IPOP_KEY_CIPHER,               G_TYPE_STRING, 0, 0, FALSE },
	{ NM_IPOP_KEY_COMP_LZO,             G_TYPE_BOOLEAN, 0, 0, FALSE },
	{ NM_IPOP_KEY_CONNECTION_TYPE,      G_TYPE_STRING, 0, 0, FALSE },
	{ NM_IPOP_KEY_FRAGMENT_SIZE,        G_TYPE_INT, 0, G_MAXINT, FALSE },
	{ NM_IPOP_KEY_KEY,                  G_TYPE_STRING, 0, 0, FALSE },
	{ NM_IPOP_KEY_LOCAL_IP,             G_TYPE_STRING, 0, 0, TRUE },
	{ NM_IPOP_KEY_MSSFIX,               G_TYPE_BOOLEAN, 0, 0, FALSE },
	{ NM_IPOP_KEY_PROTO_TCP,            G_TYPE_BOOLEAN, 0, 0, FALSE },
	{ NM_IPOP_KEY_PORT,                 G_TYPE_INT, 1, 65535, FALSE },
	{ NM_IPOP_KEY_PROXY_TYPE,           G_TYPE_STRING, 0, 0, FALSE },
	{ NM_IPOP_KEY_PROXY_SERVER,         G_TYPE_STRING, 0, 0, FALSE },
	{ NM_IPOP_KEY_PROXY_PORT,           G_TYPE_INT, 1, 65535, FALSE },
	{ NM_IPOP_KEY_PROXY_RETRY,          G_TYPE_BOOLEAN, 0, 0, FALSE },
	{ NM_IPOP_KEY_HTTP_PROXY_USERNAME,  G_TYPE_STRING, 0, 0, FALSE },
	{ NM_IPOP_KEY_REMOTE,               G_TYPE_STRING, 0, 0, FALSE },
	{ NM_IPOP_KEY_REMOTE_RANDOM,        G_TYPE_BOOLEAN, 0, 0, FALSE },
	{ NM_IPOP_KEY_REMOTE_IP,            G_TYPE_STRING, 0, 0, TRUE },
	{ NM_IPOP_KEY_RENEG_SECONDS,        G_TYPE_INT, 0, G_MAXINT, FALSE },
	{ NM_IPOP_KEY_STATIC_KEY,           G_TYPE_STRING, 0, 0, FALSE },
	{ NM_IPOP_KEY_STATIC_KEY_DIRECTION, G_TYPE_INT, 0, 1, FALSE },
	{ NM_IPOP_KEY_TA,                   G_TYPE_STRING, 0, 0, FALSE },
	{ NM_IPOP_KEY_TA_DIR,               G_TYPE_INT, 0, 1, FALSE },
	{ NM_IPOP_KEY_TAP_DEV,              G_TYPE_BOOLEAN, 0, 0, FALSE },
	{ NM_IPOP_KEY_TLS_REMOTE,	       G_TYPE_STRING, 0, 0, FALSE },
	{ NM_IPOP_KEY_TUNNEL_MTU,           G_TYPE_INT, 0, G_MAXINT, FALSE },
	{ NM_IPOP_KEY_USERNAME,             G_TYPE_STRING, 0, 0, FALSE },
	{ NM_IPOP_KEY_PASSWORD"-flags",     G_TYPE_STRING, 0, 0, FALSE },
	{ NM_IPOP_KEY_CERTPASS"-flags",     G_TYPE_STRING, 0, 0, FALSE },
	{ NM_IPOP_KEY_NOSECRET,             G_TYPE_STRING, 0, 0, FALSE },
	{ NM_IPOP_KEY_HTTP_PROXY_PASSWORD"-flags", G_TYPE_STRING, 0, 0, FALSE },
	{ NULL,                                G_TYPE_NONE, FALSE }
};

static ValidProperty valid_secrets[] = {
	{ NM_IPOP_KEY_PASSWORD,             G_TYPE_STRING, 0, 0, FALSE },
	{ NM_IPOP_KEY_CERTPASS,             G_TYPE_STRING, 0, 0, FALSE },
	{ NM_IPOP_KEY_NOSECRET,             G_TYPE_STRING, 0, 0, FALSE },
	{ NM_IPOP_KEY_HTTP_PROXY_PASSWORD,  G_TYPE_STRING, 0, 0, FALSE },
	{ NULL,                                G_TYPE_NONE, FALSE }
};

static gboolean
validate_address (const char *address)
{
	const char *p = address;

	if (!address || !strlen (address))
		return FALSE;

	/* Ensure it's a valid DNS name or IP address */
	while (*p) {
		if (!isalnum (*p) && (*p != '-') && (*p != '.'))
			return FALSE;
		p++;
	}
	return TRUE;
}

typedef struct ValidateInfo {
	ValidProperty *table;
	GError **error;
	gboolean have_items;
} ValidateInfo;

static void
validate_one_property (const char *key, const char *value, gpointer user_data)
{
	ValidateInfo *info = (ValidateInfo *) user_data;
	int i;

	if (*(info->error))
		return;

	info->have_items = TRUE;

	/* 'name' is the setting name; always allowed but unused */
	if (!strcmp (key, NM_SETTING_NAME))
		return;

	for (i = 0; info->table[i].name; i++) {
		ValidProperty prop = info->table[i];
		long int tmp;

		if (strcmp (prop.name, key))
			continue;

		switch (prop.type) {
		case G_TYPE_STRING:
			if (!prop.address || validate_address (value))
				return; /* valid */

			g_set_error (info->error,
			             NM_VPN_PLUGIN_ERROR,
			             NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
			             _("invalid address '%s'"),
			             key);
			break;
		case G_TYPE_INT:
			errno = 0;
			tmp = strtol (value, NULL, 10);
			if (errno == 0 && tmp >= prop.int_min && tmp <= prop.int_max)
				return; /* valid */

			g_set_error (info->error,
			             NM_VPN_PLUGIN_ERROR,
			             NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
			             _("invalid integer property '%s' or out of range [%d -> %d]"),
			             key, prop.int_min, prop.int_max);
			break;
		case G_TYPE_BOOLEAN:
			if (!strcmp (value, "yes") || !strcmp (value, "no"))
				return; /* valid */

			g_set_error (info->error,
			             NM_VPN_PLUGIN_ERROR,
			             NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
			             /* Translators: keep "yes" and "no" untranslated! */
			             _("invalid boolean property '%s' (not yes or no)"),
			             key);
			break;
		default:
			g_set_error (info->error,
			             NM_VPN_PLUGIN_ERROR,
			             NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
			             _("unhandled property '%s' type %s"),
			             key, g_type_name (prop.type));
			break;
		}
	}

	/* Did not find the property from valid_properties or the type did not match */
	if (!info->table[i].name) {
		g_set_error (info->error,
		             NM_VPN_PLUGIN_ERROR,
		             NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
		             _("property '%s' invalid or not supported"),
		             key);
	}
}

static gboolean
nm_ipop_properties_validate (NMSettingVPN *s_vpn, GError **error)
{
	GError *validate_error = NULL;
	ValidateInfo info = { &valid_properties[0], &validate_error, FALSE };

	nm_setting_vpn_foreach_data_item (s_vpn, validate_one_property, &info);
	if (!info.have_items) {
		g_set_error (error,
		             NM_VPN_PLUGIN_ERROR,
		             NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
		             "%s",
		             _("No VPN configuration options."));
		return FALSE;
	}

	if (validate_error) {
		*error = validate_error;
		return FALSE;
	}
	return TRUE;
}

static gboolean
nm_ipop_secrets_validate (NMSettingVPN *s_vpn, GError **error)
{
	GError *validate_error = NULL;
	ValidateInfo info = { &valid_secrets[0], &validate_error, FALSE };

	nm_setting_vpn_foreach_secret (s_vpn, validate_one_property, &info);
	if (!info.have_items) {
		g_set_error (error,
		             NM_VPN_PLUGIN_ERROR,
		             NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
		             "%s",
		             _("No VPN secrets!"));
		return FALSE;
	}

	if (validate_error) {
		*error = validate_error;
		return FALSE;
	}
	return TRUE;
}

static void
nm_ipop_disconnect_management_socket (NMIPOPPlugin *plugin)
{
	NMIPOPPluginPrivate *priv = NM_IPOP_PLUGIN_GET_PRIVATE (plugin);
	NMIPOPPluginIOData *io_data = priv->io_data;

	/* This should not throw a warning since this can happen in
	   non-password modes */
	if (!io_data)
		return;

	if (io_data->socket_channel_eventid)
		g_source_remove (io_data->socket_channel_eventid);
	if (io_data->socket_channel) {
		g_io_channel_shutdown (io_data->socket_channel, FALSE, NULL);
		g_io_channel_unref (io_data->socket_channel);
	}

	g_free (io_data->username);
	g_free (io_data->proxy_username);

	if (io_data->password)
		memset (io_data->password, 0, strlen (io_data->password));
	g_free (io_data->password);

	if (io_data->priv_key_pass)
		memset (io_data->priv_key_pass, 0, strlen (io_data->priv_key_pass));
	g_free (io_data->priv_key_pass);

	if (io_data->proxy_password)
		memset (io_data->proxy_password, 0, strlen (io_data->proxy_password));
	g_free (io_data->proxy_password);

	g_free (priv->io_data);
	priv->io_data = NULL;
}

static char *
ovpn_quote_string (const char *unquoted)
{
	char *quoted = NULL, *q;
	char *u = (char *) unquoted;

	g_return_val_if_fail (unquoted != NULL, NULL);

	/* FIXME: use unpaged memory */
	quoted = q = g_malloc0 (strlen (unquoted) * 2);
	while (*u) {
		/* Escape certain characters */
		if (*u == ' ' || *u == '\\' || *u == '"')
			*q++ = '\\';
		*q++ = *u++;
	}

	return quoted;
}

/* sscanf is evil, and since we can't use glib regexp stuff since it's still
 * too new for some distros, do a simple match here.
 */
static char *
get_detail (const char *input, const char *prefix)
{
	char *ret = NULL;
	guint32 i = 0;
	const char *p, *start;

	g_return_val_if_fail (prefix != NULL, NULL);

	if (!g_str_has_prefix (input, prefix))
		return NULL;

	/* Grab characters until the next ' */
	p = start = input + strlen (prefix);
	while (*p) {
		if (*p == '\'') {
			ret = g_malloc0 (i + 1);
			strncpy (ret, start, i);
			break;
		}
		p++, i++;
	}

	return ret;
}

static void
write_user_pass (GIOChannel *channel,
                 const char *authtype,
                 const char *user,
                 const char *pass)
{
	char *quser, *qpass, *buf;

	/* Quote strings passed back to ipop */
	quser = ovpn_quote_string (user);
	qpass = ovpn_quote_string (pass);
	buf = g_strdup_printf ("username \"%s\" \"%s\"\n"
	                       "password \"%s\" \"%s\"\n",
	                       authtype, quser,
	                       authtype, qpass);
	memset (qpass, 0, strlen (qpass));
	g_free (qpass);
	g_free (quser);

	/* Will always write everything in blocking channels (on success) */
	g_io_channel_write_chars (channel, buf, strlen (buf), NULL, NULL);
	g_io_channel_flush (channel, NULL);

	memset (buf, 0, strlen (buf));
	g_free (buf);
}

static gboolean
handle_management_socket (NMVPNPlugin *plugin,
                          GIOChannel *source,
                          GIOCondition condition,
                          NMVPNPluginFailure *out_failure)
{
	NMIPOPPluginIOData *io_data = NM_IPOP_PLUGIN_GET_PRIVATE (plugin)->io_data;
	gboolean again = TRUE;
	char *str = NULL, *auth = NULL, *buf;

	if (!(condition & G_IO_IN))
		return TRUE;

	if (g_io_channel_read_line (source, &str, NULL, NULL, NULL) != G_IO_STATUS_NORMAL)
		return TRUE;

	if (strlen (str) < 1)
		goto out;

	auth = get_detail (str, ">PASSWORD:Need '");
	if (auth) {
		if (strcmp (auth, "Auth") == 0) {
			if (io_data->username != NULL && io_data->password != NULL)
				write_user_pass (source, auth, io_data->username, io_data->password);
			else
				g_warning ("Auth requested but one of username or password is missing");
		} else if (!strcmp (auth, "Private Key")) {
			if (io_data->priv_key_pass) {
				char *qpass;

				/* Quote strings passed back to ipop */
				qpass = ovpn_quote_string (io_data->priv_key_pass);
				buf = g_strdup_printf ("password \"%s\" \"%s\"\n", auth, qpass);
				memset (qpass, 0, strlen (qpass));
				g_free (qpass);

				/* Will always write everything in blocking channels (on success) */
				g_io_channel_write_chars (source, buf, strlen (buf), NULL, NULL);
				g_io_channel_flush (source, NULL);
				g_free (buf);
			} else
				g_warning ("Certificate password requested but private key password == NULL");
		} else if (strcmp (auth, "HTTP Proxy") == 0) {
			if (io_data->proxy_username != NULL && io_data->proxy_password != NULL)
				write_user_pass (source, auth, io_data->proxy_username, io_data->proxy_password);
			else
				g_warning ("HTTP Proxy auth requested but either proxy username or password is missing");
		} else {
			g_warning ("No clue what to send for username/password request for '%s'", auth);
			if (out_failure)
				*out_failure = NM_VPN_PLUGIN_FAILURE_CONNECT_FAILED;
			again = FALSE;
		}
		g_free (auth);
	}

	auth = get_detail (str, ">PASSWORD:Verification Failed: '");
	if (auth) {
		if (!strcmp (auth, "Auth"))
			g_warning ("Password verification failed");
		else if (!strcmp (auth, "Private Key"))
			g_warning ("Private key verification failed");
		else
			g_warning ("Unknown verification failed: %s", auth);

		g_free (auth);

		if (out_failure)
			*out_failure = NM_VPN_PLUGIN_FAILURE_LOGIN_FAILED;
		again = FALSE;
	}

out:
	g_free (str);
	return again;
}

static gboolean
nm_ipop_socket_data_cb (GIOChannel *source, GIOCondition condition, gpointer user_data)
{
	NMVPNPlugin *plugin = NM_VPN_PLUGIN (user_data);
	NMVPNPluginFailure failure = NM_VPN_PLUGIN_FAILURE_CONNECT_FAILED;

	if (!handle_management_socket (plugin, source, condition, &failure)) {
		nm_vpn_plugin_failure (plugin, failure);
		nm_vpn_plugin_set_state (plugin, NM_VPN_SERVICE_STATE_STOPPED);
		return FALSE;
	}

	return TRUE;
}

static gboolean
nm_ipop_connect_timer_cb (gpointer data)
{
	NMIPOPPlugin *plugin = NM_IPOP_PLUGIN (data);
	NMIPOPPluginPrivate *priv = NM_IPOP_PLUGIN_GET_PRIVATE (plugin);
	struct sockaddr_in     serv_addr;
	gboolean               connected = FALSE;
	gint                   socket_fd = -1;
	NMIPOPPluginIOData *io_data = priv->io_data;

	priv->connect_count++;

	/* open socket and start listener */
	socket_fd = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (socket_fd < 0)
		return FALSE;

	serv_addr.sin_family = AF_INET;
	if (inet_pton (AF_INET, "127.0.0.1", &(serv_addr.sin_addr)) <= 0)
		g_warning ("%s: could not convert 127.0.0.1", __func__);
	serv_addr.sin_port = htons (1194);
 
	connected = (connect (socket_fd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) == 0);
	if (!connected) {
		close (socket_fd);
		if (priv->connect_count <= 30)
			return TRUE;

		priv->connect_timer = 0;

		g_warning ("Could not open management socket");
		nm_vpn_plugin_failure (NM_VPN_PLUGIN (plugin), NM_VPN_PLUGIN_FAILURE_CONNECT_FAILED);
		nm_vpn_plugin_set_state (NM_VPN_PLUGIN (plugin), NM_VPN_SERVICE_STATE_STOPPED);
	} else {
		GIOChannel *ipop_socket_channel;
		guint ipop_socket_channel_eventid;

		ipop_socket_channel = g_io_channel_unix_new (socket_fd);
		ipop_socket_channel_eventid = g_io_add_watch (ipop_socket_channel,
		                                                 G_IO_IN,
		                                                 nm_ipop_socket_data_cb,
		                                                 plugin);

		g_io_channel_set_encoding (ipop_socket_channel, NULL, NULL);
		io_data->socket_channel = ipop_socket_channel;
		io_data->socket_channel_eventid = ipop_socket_channel_eventid;
	}

	priv->connect_timer = 0;
	return FALSE;
}

static void
nm_ipop_schedule_connect_timer (NMIPOPPlugin *plugin)
{
	NMIPOPPluginPrivate *priv = NM_IPOP_PLUGIN_GET_PRIVATE (plugin);

	if (priv->connect_timer == 0)
		priv->connect_timer = g_timeout_add (200, nm_ipop_connect_timer_cb, plugin);
}

static void
ipop_watch_cb (GPid pid, gint status, gpointer user_data)
{
	NMVPNPlugin *plugin = NM_VPN_PLUGIN (user_data);
	NMIPOPPluginPrivate *priv = NM_IPOP_PLUGIN_GET_PRIVATE (plugin);
	NMVPNPluginFailure failure = NM_VPN_PLUGIN_FAILURE_CONNECT_FAILED;
	guint error = 0;
	gboolean good_exit = FALSE;

	if (WIFEXITED (status)) {
		error = WEXITSTATUS (status);
		if (error != 0)
			g_warning ("ipop exited with error code %d", error);
    }
	else if (WIFSTOPPED (status))
		g_warning ("ipop stopped unexpectedly with signal %d", WSTOPSIG (status));
	else if (WIFSIGNALED (status))
		g_warning ("ipop died with signal %d", WTERMSIG (status));
	else
		g_warning ("ipop died from an unknown cause");
  
	/* Reap child if needed. */
	waitpid (priv->pid, NULL, WNOHANG);
	priv->pid = 0;

	/* IPOP doesn't supply useful exit codes :( */
	switch (error) {
	case 0:
		good_exit = TRUE;
		break;
	default:
		failure = NM_VPN_PLUGIN_FAILURE_CONNECT_FAILED;
		break;
	}

	/* Try to get the last bits of data from ipop */
	if (priv->io_data && priv->io_data->socket_channel) {
		GIOChannel *channel = priv->io_data->socket_channel;
		GIOCondition condition;

		while ((condition = g_io_channel_get_buffer_condition (channel)) & G_IO_IN) {
			if (!handle_management_socket (plugin, channel, condition, &failure)) {
				good_exit = FALSE;
				break;
			}
		}
	}

	if (!good_exit)
		nm_vpn_plugin_failure (plugin, failure);

	nm_vpn_plugin_set_state (plugin, NM_VPN_SERVICE_STATE_STOPPED);
}

static gboolean
validate_auth (const char *auth)
{
	if (auth) {
		if (   !strcmp (auth, NM_IPOP_AUTH_NONE)
		    || !strcmp (auth, NM_IPOP_AUTH_RSA_MD4)
		    || !strcmp (auth, NM_IPOP_AUTH_MD5)
		    || !strcmp (auth, NM_IPOP_AUTH_SHA1)
		    || !strcmp (auth, NM_IPOP_AUTH_SHA224)
		    || !strcmp (auth, NM_IPOP_AUTH_SHA256)
		    || !strcmp (auth, NM_IPOP_AUTH_SHA384)
		    || !strcmp (auth, NM_IPOP_AUTH_SHA512)
		    || !strcmp (auth, NM_IPOP_AUTH_RIPEMD160))
			return TRUE;
	}
	return FALSE;
}

static const char *
validate_connection_type (const char *ctype)
{
	if (ctype) {
		if (   !strcmp (ctype, NM_IPOP_CONTYPE_TLS)
		    || !strcmp (ctype, NM_IPOP_CONTYPE_STATIC_KEY)
		    || !strcmp (ctype, NM_IPOP_CONTYPE_PASSWORD)
		    || !strcmp (ctype, NM_IPOP_CONTYPE_PASSWORD_TLS))
			return ctype;
	}
	return NULL;
}

static const char *
nm_find_ipop (void)
{
	static const char *ipop_binary_paths[] = {
		"/usr/sbin/ipop",
		"/sbin/ipop",
		"/usr/local/sbin/ipop",
		NULL
	};
	const char  **ipop_binary = ipop_binary_paths;

	while (*ipop_binary != NULL) {
		if (g_file_test (*ipop_binary, G_FILE_TEST_EXISTS))
			break;
		ipop_binary++;
	}

	return *ipop_binary;
}

static void
free_ipop_args (GPtrArray *args)
{
	g_ptr_array_foreach (args, (GFunc) g_free, NULL);
	g_ptr_array_free (args, TRUE);
}

static void
add_ipop_arg (GPtrArray *args, const char *arg)
{
	g_return_if_fail (args != NULL);
	g_return_if_fail (arg != NULL);

	g_ptr_array_add (args, (gpointer) g_strdup (arg));
}

static gboolean
add_ipop_arg_int (GPtrArray *args, const char *arg)
{
	long int tmp_int;

	g_return_val_if_fail (args != NULL, FALSE);
	g_return_val_if_fail (arg != NULL, FALSE);

	/* Convert -> int and back to string for security's sake since
	 * strtol() ignores some leading and trailing characters.
	 */
	errno = 0;
	tmp_int = strtol (arg, NULL, 10);
	if (errno != 0)
		return FALSE;
	g_ptr_array_add (args, (gpointer) g_strdup_printf ("%d", (guint32) tmp_int));
	return TRUE;
}

static void
add_cert_args (GPtrArray *args, NMSettingVPN *s_vpn)
{
	const char *ca, *cert, *key;

	g_return_if_fail (args != NULL);
	g_return_if_fail (s_vpn != NULL);

	ca = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_CA);
	cert = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_CERT);
	key = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_KEY);

	if (   ca && strlen (ca)
	    && cert && strlen (cert)
	    && key && strlen (key)
	    && !strcmp (ca, cert)
	    && !strcmp (ca, key)) {
		add_ipop_arg (args, "--pkcs12");
		add_ipop_arg (args, ca);
	} else {
		if (ca && strlen (ca)) {
			add_ipop_arg (args, "--ca");
			add_ipop_arg (args, ca);
		}

		if (cert && strlen (cert)) {
			add_ipop_arg (args, "--cert");
			add_ipop_arg (args, cert);
		}

		if (key && strlen (key)) {
			add_ipop_arg (args, "--key");
			add_ipop_arg (args, key);
		}
	}
}

static gboolean
nm_ipop_start_ipop_binary (NMIPOPPlugin *plugin,
                                 NMSettingVPN *s_vpn,
                                 const char *default_username,
                                 GError **error)
{
	NMIPOPPluginPrivate *priv = NM_IPOP_PLUGIN_GET_PRIVATE (plugin);
	const char *ipop_binary, *auth, *connection_type, *tmp, *tmp2, *tmp3, *tmp4;
	GPtrArray *args;
	GSource *ipop_watch;
	GPid pid;

	/* Find ipop */
	ipop_binary = nm_find_ipop ();
	if (!ipop_binary) {
		g_set_error (error,
		             NM_VPN_PLUGIN_ERROR,
		             NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
		             "%s",
		             _("Could not find the ipop binary."));
		return FALSE;
	}
  
 	auth = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_AUTH);
 	if (auth) {
 		if (!validate_auth(auth)) {
 			g_set_error (error,
 			             NM_VPN_PLUGIN_ERROR,
 			             NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
 			             "%s",
 			             _("Invalid HMAC auth."));
 			return FALSE;
 		}
 	}

	tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_CONNECTION_TYPE);
	connection_type = validate_connection_type (tmp);
	if (!connection_type) {
		g_set_error (error,
		             NM_VPN_PLUGIN_ERROR,
		             NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
		             "%s",
		             _("Invalid connection type."));
		return FALSE;
	}

	args = g_ptr_array_new ();
	add_ipop_arg (args, ipop_binary);

	tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_REMOTE);
	if (tmp && strlen (tmp)) {
		char *tok;
		while ((tok = strsep((char**)&tmp, " ,")) != NULL) {
			if (strlen(tok)) {
				add_ipop_arg (args, "--remote");
				add_ipop_arg (args, tok);
			}
		}
	}

	/* Remote random */
	tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_REMOTE_RANDOM);
	if (tmp && !strcmp (tmp, "yes"))
		add_ipop_arg (args, "--remote-random");

	tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_PROXY_TYPE);
	tmp2 = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_PROXY_SERVER);
	tmp3 = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_PROXY_PORT);
	tmp4 = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_PROXY_RETRY);
	if (tmp && strlen (tmp) && tmp2 && strlen (tmp2)) {
		if (!strcmp (tmp, "http")) {
			add_ipop_arg (args, "--http-proxy");
			add_ipop_arg (args, tmp2);
			add_ipop_arg (args, tmp3 ? tmp3 : "8080");
			add_ipop_arg (args, "auto");  /* Automatic proxy auth method detection */
			if (tmp4)
				add_ipop_arg (args, "--http-proxy-retry");
		} else if (!strcmp (tmp, "socks")) {
			add_ipop_arg (args, "--socks-proxy");
			add_ipop_arg (args, tmp2);
			add_ipop_arg (args, tmp3 ? tmp3 : "1080");
			if (tmp4)
				add_ipop_arg (args, "--socks-proxy-retry");
		} else {
			g_set_error (error,
				         NM_VPN_PLUGIN_ERROR,
				         NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
				         _("Invalid proxy type '%s'."),
				         tmp);
			return FALSE;
		}
	}

	tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_COMP_LZO);
	if (tmp && !strcmp (tmp, "yes"))
		add_ipop_arg (args, "--comp-lzo");

	add_ipop_arg (args, "--nobind");

	/* Device, either tun or tap */
	add_ipop_arg (args, "--dev");
	tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_TAP_DEV);
	if (tmp && !strcmp (tmp, "yes"))
		add_ipop_arg (args, "tap");
	else
		add_ipop_arg (args, "tun");

	/* Protocol, either tcp or udp */
	add_ipop_arg (args, "--proto");
	tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_PROTO_TCP);
	if (tmp && !strcmp (tmp, "yes"))
		add_ipop_arg (args, "tcp-client");
	else
		add_ipop_arg (args, "udp");

	/* Port */
	add_ipop_arg (args, "--port");
	tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_PORT);
	if (tmp && strlen (tmp)) {
		if (!add_ipop_arg_int (args, tmp)) {
			g_set_error (error,
			             NM_VPN_PLUGIN_ERROR,
			             NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
			             _("Invalid port number '%s'."),
			             tmp);
			free_ipop_args (args);
			return FALSE;
		}
	} else {
		/* Default to IANA assigned port 1194 */
		add_ipop_arg (args, "1194");
	}

	/* Cipher */
	tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_CIPHER);
	if (tmp && strlen (tmp)) {
		add_ipop_arg (args, "--cipher");
		add_ipop_arg (args, tmp);
	}

	/* Auth */
	if (auth) {
		add_ipop_arg (args, "--auth");
		add_ipop_arg (args, auth);
	}
	add_ipop_arg (args, "--auth-nocache");

	/* TA */
	tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_TA);
	if (tmp && strlen (tmp)) {
		add_ipop_arg (args, "--tls-auth");
		add_ipop_arg (args, tmp);

		tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_TA_DIR);
		if (tmp && strlen (tmp))
			add_ipop_arg (args, tmp);
	}

	/* tls-remote */
	tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_TLS_REMOTE);
	if (tmp && strlen (tmp)) {
                add_ipop_arg (args, "--tls-remote");
                add_ipop_arg (args, tmp);
	}

	/* Reneg seconds */
	tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_RENEG_SECONDS);
	if (tmp && strlen (tmp)) {
		add_ipop_arg (args, "--reneg-sec");
		if (!add_ipop_arg_int (args, tmp)) {
			g_set_error (error,
			             NM_VPN_PLUGIN_ERROR,
			             NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
			             _("Invalid reneg seconds '%s'."),
			             tmp);
			free_ipop_args (args);
			return FALSE;
		}
	}

	if (debug) {
		add_ipop_arg (args, "--verb");
		add_ipop_arg (args, "10");
	} else {
		/* Syslog */
		add_ipop_arg (args, "--syslog");
		add_ipop_arg (args, "nm-ipop");
	}

	/* TUN MTU size */
	tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_TUNNEL_MTU);
	if (tmp && strlen (tmp)) {
		add_ipop_arg (args, "--tun-mtu");
		if (!add_ipop_arg_int (args, tmp)) {
			g_set_error (error,
			             NM_VPN_PLUGIN_ERROR,
			             NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
			             _("Invalid TUN MTU size '%s'."),
			             tmp);
			free_ipop_args (args);
			return FALSE;
		}
	}

	/* fragment size */
	tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_FRAGMENT_SIZE);
	if (tmp && strlen (tmp)) {
		add_ipop_arg (args, "--fragment");
		if (!add_ipop_arg_int (args, tmp)) {
			g_set_error (error,
			             NM_VPN_PLUGIN_ERROR,
			             NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
			             _("Invalid fragment size '%s'."),
			             tmp);
			free_ipop_args (args);
			return FALSE;
		}
	}

	/* mssfix */
	tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_MSSFIX);
	if (tmp && !strcmp (tmp, "yes")) {
		add_ipop_arg (args, "--mssfix");
	}

	/* Punch script security in the face; this option was added to IPOP 2.1-rc9
	 * and defaults to disallowing any scripts, a behavior change from previous
	 * versions.
	 */
	add_ipop_arg (args, "--script-security");
	add_ipop_arg (args, "2");

	/* Up script, called when connection has been established or has been restarted */
	add_ipop_arg (args, "--up");
	if (debug)
		add_ipop_arg (args, NM_IPOP_HELPER_PATH " --helper-debug");
	else
		add_ipop_arg (args, NM_IPOP_HELPER_PATH);
	add_ipop_arg (args, "--up-restart");

	/* Keep key and tun if restart is needed */
	add_ipop_arg (args, "--persist-key");
	add_ipop_arg (args, "--persist-tun");

	/* Management socket for localhost access to supply username and password */
	add_ipop_arg (args, "--management");
	add_ipop_arg (args, "127.0.0.1");
	/* with have nobind, thus 1194 should be free, it is the IANA assigned port */
	add_ipop_arg (args, "1194");
	/* Query on the management socket for user/pass */
	add_ipop_arg (args, "--management-query-passwords");

	/* do not let ipop setup routes or addresses, NM will handle it */
	add_ipop_arg (args, "--route-noexec");
	add_ipop_arg (args, "--ifconfig-noexec");

	/* Now append configuration options which are dependent on the configuration type */
	if (!strcmp (connection_type, NM_IPOP_CONTYPE_TLS)) {
		add_ipop_arg (args, "--client");
		add_cert_args (args, s_vpn);
	} else if (!strcmp (connection_type, NM_IPOP_CONTYPE_STATIC_KEY)) {
		tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_STATIC_KEY);
		if (tmp && strlen (tmp)) {
			add_ipop_arg (args, "--secret");
			add_ipop_arg (args, tmp);

			tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_STATIC_KEY_DIRECTION);
			if (tmp && strlen (tmp))
				add_ipop_arg (args, tmp);
		}

		add_ipop_arg (args, "--ifconfig");

		tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_LOCAL_IP);
		if (!tmp) {
			/* Insufficient data (FIXME: this should really be detected when validating the properties */
			g_set_error (error,
			             NM_VPN_PLUGIN_ERROR,
			             NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
			             "%s",
			             _("Missing required local IP address for static key mode."));
			free_ipop_args (args);
			return FALSE;
		}
		add_ipop_arg (args, tmp);

		tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_REMOTE_IP);
		if (!tmp) {
			/* Insufficient data (FIXME: this should really be detected when validating the properties */
			g_set_error (error,
			             NM_VPN_PLUGIN_ERROR,
			             NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
			             "%s",
			             _("Missing required remote IP address for static key mode."));
			free_ipop_args (args);
			return FALSE;
		}
		add_ipop_arg (args, tmp);
	} else if (!strcmp (connection_type, NM_IPOP_CONTYPE_PASSWORD)) {
		/* Client mode */
		add_ipop_arg (args, "--client");
		/* Use user/path authentication */
		add_ipop_arg (args, "--auth-user-pass");

		tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_CA);
		if (tmp && strlen (tmp)) {
			add_ipop_arg (args, "--ca");
			add_ipop_arg (args, tmp);
		}
	} else if (!strcmp (connection_type, NM_IPOP_CONTYPE_PASSWORD_TLS)) {
		add_ipop_arg (args, "--client");
		add_cert_args (args, s_vpn);
		/* Use user/path authentication */
		add_ipop_arg (args, "--auth-user-pass");
	} else {
		g_set_error (error,
		             NM_VPN_PLUGIN_ERROR,
		             NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
		             _("Unknown connection type '%s'."),
		             connection_type);
		free_ipop_args (args);
		return FALSE;
	}

	g_ptr_array_add (args, NULL);

	if (!g_spawn_async (NULL, (char **) args->pdata, NULL,
	                    G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid, error)) {
		free_ipop_args (args);
		return FALSE;
	}
	free_ipop_args (args);

	g_message ("ipop started with pid %d", pid);

	priv->pid = pid;
	ipop_watch = g_child_watch_source_new (pid);
	g_source_set_callback (ipop_watch, (GSourceFunc) ipop_watch_cb, plugin, NULL);
	g_source_attach (ipop_watch, NULL);
	g_source_unref (ipop_watch);

	/* Listen to the management socket for a few connection types:
	   PASSWORD: Will require username and password
	   X509USERPASS: Will require username and password and maybe certificate password
	   X509: May require certificate password
	*/
	if (   !strcmp (connection_type, NM_IPOP_CONTYPE_TLS)
	    || !strcmp (connection_type, NM_IPOP_CONTYPE_PASSWORD)
	    || !strcmp (connection_type, NM_IPOP_CONTYPE_PASSWORD_TLS)
	    || nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_HTTP_PROXY_USERNAME)) {

		priv->io_data = g_malloc0 (sizeof (NMIPOPPluginIOData));

		tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_USERNAME);
		priv->io_data->username = tmp ? g_strdup (tmp) : NULL;
		/* Use the default username if it wasn't overridden by the user */
		if (!priv->io_data->username && default_username)
			priv->io_data->username = g_strdup (default_username);

		tmp = nm_setting_vpn_get_secret (s_vpn, NM_IPOP_KEY_PASSWORD);
		priv->io_data->password = tmp ? g_strdup (tmp) : NULL;

		tmp = nm_setting_vpn_get_secret (s_vpn, NM_IPOP_KEY_CERTPASS);
		priv->io_data->priv_key_pass = tmp ? g_strdup (tmp) : NULL;

		tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_HTTP_PROXY_USERNAME);
		priv->io_data->proxy_username = tmp ? g_strdup (tmp) : NULL;

		tmp = nm_setting_vpn_get_secret (s_vpn, NM_IPOP_KEY_HTTP_PROXY_PASSWORD);
		priv->io_data->proxy_password = tmp ? g_strdup (tmp) : NULL;

		nm_ipop_schedule_connect_timer (plugin);
	}

	return TRUE;
}

static const char *
check_need_secrets (NMSettingVPN *s_vpn, gboolean *need_secrets)
{
	const char *tmp, *key, *ctype;

	g_return_val_if_fail (s_vpn != NULL, FALSE);
	g_return_val_if_fail (need_secrets != NULL, FALSE);

	*need_secrets = FALSE;

	tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_CONNECTION_TYPE);
	ctype = validate_connection_type (tmp);
	if (!ctype)
		return NULL;

	if (!strcmp (ctype, NM_IPOP_CONTYPE_PASSWORD_TLS)) {
		/* Will require a password and maybe private key password */
		key = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_KEY);
		if (is_encrypted (key) && !nm_setting_vpn_get_secret (s_vpn, NM_IPOP_KEY_CERTPASS))
			*need_secrets = TRUE;

		if (!nm_setting_vpn_get_secret (s_vpn, NM_IPOP_KEY_PASSWORD))
			*need_secrets = TRUE;
	} else if (!strcmp (ctype, NM_IPOP_CONTYPE_PASSWORD)) {
		/* Will require a password */
		if (!nm_setting_vpn_get_secret (s_vpn, NM_IPOP_KEY_PASSWORD))
			*need_secrets = TRUE;
	} else if (!strcmp (ctype, NM_IPOP_CONTYPE_TLS)) {
		/* May require private key password */
		key = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_KEY);
		if (is_encrypted (key) && !nm_setting_vpn_get_secret (s_vpn, NM_IPOP_KEY_CERTPASS))
			*need_secrets = TRUE;
	} else {
		/* Static key doesn't need passwords */
	}

	/* HTTP Proxy might require a password; assume so if there's an HTTP proxy username */
	tmp = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_HTTP_PROXY_USERNAME);
	if (tmp && !nm_setting_vpn_get_secret (s_vpn, NM_IPOP_KEY_HTTP_PROXY_PASSWORD))
		*need_secrets = TRUE;

	return ctype;
}

static gboolean
real_connect (NMVPNPlugin   *plugin,
              NMConnection  *connection,
              GError       **error)
{
	NMSettingVPN *s_vpn;
	const char *connection_type;
	const char *user_name;
	gboolean need_secrets;

	s_vpn = NM_SETTING_VPN (nm_connection_get_setting (connection, NM_TYPE_SETTING_VPN));
	if (!s_vpn) {
		g_set_error (error,
		             NM_VPN_PLUGIN_ERROR,
		             NM_VPN_PLUGIN_ERROR_CONNECTION_INVALID,
		             "%s",
		             _("Could not process the request because the VPN connection settings were invalid."));
		return FALSE;
	}

	/* Check if we need secrets and validate the connection type */
	connection_type = check_need_secrets (s_vpn, &need_secrets);
	if (!connection_type) {
		g_set_error (error,
		             NM_VPN_PLUGIN_ERROR,
		             NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
		             "%s",
		             _("Invalid connection type."));
		return FALSE;
	}

	user_name = nm_setting_vpn_get_user_name (s_vpn);

	/* Need a username for any password-based connection types */
	if (   !strcmp (connection_type, NM_IPOP_CONTYPE_PASSWORD_TLS)
	    || !strcmp (connection_type, NM_IPOP_CONTYPE_PASSWORD)) {
		if (!user_name && !nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_USERNAME)) {
			g_set_error (error,
			             NM_VPN_PLUGIN_ERROR,
			             NM_VPN_PLUGIN_ERROR_CONNECTION_INVALID,
			             "%s",
			             _("Could not process the request because no username was provided."));
			return FALSE;
		}
	}

	/* Validate the properties */
	if (!nm_ipop_properties_validate (s_vpn, error))
		return FALSE;

	/* Validate secrets */
	if (need_secrets) {
		if (!nm_ipop_secrets_validate (s_vpn, error))
			return FALSE;
	}

	/* Finally try to start IPOP */
	if (!nm_ipop_start_ipop_binary (NM_IPOP_PLUGIN (plugin), s_vpn, user_name, error))
		return FALSE;

	return TRUE;
}

static gboolean
real_need_secrets (NMVPNPlugin *plugin,
                   NMConnection *connection,
                   char **setting_name,
                   GError **error)
{
	NMSettingVPN *s_vpn;
	const char *connection_type;
	gboolean need_secrets = FALSE;

	g_return_val_if_fail (NM_IS_VPN_PLUGIN (plugin), FALSE);
	g_return_val_if_fail (NM_IS_CONNECTION (connection), FALSE);

	if (debug) {
		g_message ("%s: connection -------------------------------------", __func__);
		nm_connection_dump (connection);
	}

	s_vpn = NM_SETTING_VPN (nm_connection_get_setting (connection, NM_TYPE_SETTING_VPN));
	if (!s_vpn) {
		g_set_error (error,
		             NM_VPN_PLUGIN_ERROR,
		             NM_VPN_PLUGIN_ERROR_CONNECTION_INVALID,
		             "%s",
		             _("Could not process the request because the VPN connection settings were invalid."));
		return FALSE;
	}

	connection_type = check_need_secrets (s_vpn, &need_secrets);
	if (!connection_type) {
		g_set_error (error,
		             NM_VPN_PLUGIN_ERROR,
		             NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
		             "%s",
		             _("Invalid connection type."));
		return FALSE;
	}

	if (need_secrets)
		*setting_name = NM_SETTING_VPN_SETTING_NAME;

	return need_secrets;
}

static gboolean
ensure_killed (gpointer data)
{
	int pid = GPOINTER_TO_INT (data);

	if (kill (pid, 0) == 0)
		kill (pid, SIGKILL);

	return FALSE;
}

static gboolean
real_disconnect (NMVPNPlugin	 *plugin,
			  GError		**err)
{
	NMIPOPPluginPrivate *priv = NM_IPOP_PLUGIN_GET_PRIVATE (plugin);

	if (priv->pid) {
		if (kill (priv->pid, SIGTERM) == 0)
			g_timeout_add (2000, ensure_killed, GINT_TO_POINTER (priv->pid));
		else
			kill (priv->pid, SIGKILL);

		g_message ("Terminated ipop daemon with PID %d.", priv->pid);
		priv->pid = 0;
	}

	return TRUE;
}

static void
nm_ipop_plugin_init (NMIPOPPlugin *plugin)
{
}

static void
nm_ipop_plugin_class_init (NMIPOPPluginClass *plugin_class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (plugin_class);
	NMVPNPluginClass *parent_class = NM_VPN_PLUGIN_CLASS (plugin_class);

	g_type_class_add_private (object_class, sizeof (NMIPOPPluginPrivate));

	/* virtual methods */
	parent_class->connect      = real_connect;
	parent_class->need_secrets = real_need_secrets;
	parent_class->disconnect   = real_disconnect;
}

static void
plugin_state_changed (NMIPOPPlugin *plugin,
                      NMVPNServiceState state,
                      gpointer user_data)
{
	NMIPOPPluginPrivate *priv = NM_IPOP_PLUGIN_GET_PRIVATE (plugin);

	switch (state) {
	case NM_VPN_SERVICE_STATE_UNKNOWN:
	case NM_VPN_SERVICE_STATE_INIT:
	case NM_VPN_SERVICE_STATE_SHUTDOWN:
	case NM_VPN_SERVICE_STATE_STOPPING:
	case NM_VPN_SERVICE_STATE_STOPPED:
		/* Cleanup on failure */
		if (priv->connect_timer) {
			g_source_remove (priv->connect_timer);
			priv->connect_timer = 0;
		}
		nm_ipop_disconnect_management_socket (plugin);
		break;
	default:
		break;
	}
}

NMIPOPPlugin *
nm_ipop_plugin_new (void)
{
	NMIPOPPlugin *plugin;

	plugin =  (NMIPOPPlugin *) g_object_new (NM_TYPE_IPOP_PLUGIN,
	                                            NM_VPN_PLUGIN_DBUS_SERVICE_NAME,
	                                            NM_DBUS_SERVICE_IPOP,
	                                            NULL);
	if (plugin)
		g_signal_connect (G_OBJECT (plugin), "state-changed", G_CALLBACK (plugin_state_changed), NULL);

	return plugin;
}

static void
signal_handler (int signo)
{
	if (signo == SIGINT || signo == SIGTERM)
		g_main_loop_quit (loop);
}

static void
setup_signals (void)
{
	struct sigaction action;
	sigset_t mask;

	sigemptyset (&mask);
	action.sa_handler = signal_handler;
	action.sa_mask = mask;
	action.sa_flags = 0;
	sigaction (SIGTERM,  &action, NULL);
	sigaction (SIGINT,  &action, NULL);
}

static void
quit_mainloop (NMVPNPlugin *plugin, gpointer user_data)
{
	g_main_loop_quit ((GMainLoop *) user_data);
}

int
main (int argc, char *argv[])
{
	NMIPOPPlugin *plugin;
	gboolean persist = FALSE;
	GOptionContext *opt_ctx = NULL;

	GOptionEntry options[] = {
		{ "persist", 0, 0, G_OPTION_ARG_NONE, &persist, N_("Don't quit when VPN connection terminates"), NULL },
		{ "debug", 0, 0, G_OPTION_ARG_NONE, &debug, N_("Enable verbose debug logging (may expose passwords)"), NULL },
		{NULL}
	};

	g_type_init ();

	/* Parse options */
	opt_ctx = g_option_context_new ("");
	g_option_context_set_translation_domain (opt_ctx, "UTF-8");
	g_option_context_set_ignore_unknown_options (opt_ctx, FALSE);
	g_option_context_set_help_enabled (opt_ctx, TRUE);
	g_option_context_add_main_entries (opt_ctx, options, NULL);

	g_option_context_set_summary (opt_ctx,
		_("nm-vpnc-service provides integrated IPOP capability to NetworkManager."));

	g_option_context_parse (opt_ctx, &argc, &argv, NULL);
	g_option_context_free (opt_ctx);

	if (getenv ("IPOP_DEBUG"))
		debug = TRUE;

	if (debug)
		g_message ("nm-ipop-service (version " DIST_VERSION ") starting...");

	if (system ("/sbin/modprobe tun") == -1)
		exit (EXIT_FAILURE);

	plugin = nm_ipop_plugin_new ();
	if (!plugin)
		exit (EXIT_FAILURE);

	loop = g_main_loop_new (NULL, FALSE);

	if (!persist)
		g_signal_connect (plugin, "quit", G_CALLBACK (quit_mainloop), loop);

	setup_signals ();
	g_main_loop_run (loop);

	g_main_loop_unref (loop);
	g_object_unref (plugin);

	exit (EXIT_SUCCESS);
}
