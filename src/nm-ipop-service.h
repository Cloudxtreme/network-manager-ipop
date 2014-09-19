/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* nm-ipop-service - ipop integration with NetworkManager
 *
 * Copyright (C) 2005 - 2008 Tim Niemueller <tim@niemueller.de>
 * Copyright (C) 2005 - 2008 Dan Williams <dcbw@redhat.com>
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
 */

#ifndef NM_IPOP_SERVICE_H
#define NM_IPOP_SERVICE_H

#include <glib.h>
#include <glib-object.h>
#include <nm-vpn-plugin.h>

#define NM_TYPE_IPOP_PLUGIN            (nm_ipop_plugin_get_type ())
#define NM_IPOP_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), NM_TYPE_IPOP_PLUGIN, NMIPOPPlugin))
#define NM_IPOP_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), NM_TYPE_IPOP_PLUGIN, NMIPOPPluginClass))
#define NM_IS_IPOP_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), NM_TYPE_IPOP_PLUGIN))
#define NM_IS_IPOP_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), NM_TYPE_IPOP_PLUGIN))
#define NM_IPOP_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), NM_TYPE_IPOP_PLUGIN, NMIPOPPluginClass))

#define NM_DBUS_SERVICE_IPOP    "org.freedesktop.NetworkManager.ipop"
#define NM_DBUS_INTERFACE_IPOP  "org.freedesktop.NetworkManager.ipop"
#define NM_DBUS_PATH_IPOP       "/org/freedesktop/NetworkManager/ipop"

#define NM_IPOP_KEY_LOCAL_IP "local-ip" /* ??? */
#define NM_IPOP_KEY_PORT "port"
#define NM_IPOP_KEY_XMPP_HOST "xmpp-host"
#define NM_IPOP_KEY_XMPP_USERNAME "xmpp-username"
#define NM_IPOP_KEY_XMPP_PASSWORD "xmpp-password"


typedef struct {
    NMVPNPlugin parent;
} NMIPOPPlugin;

typedef struct {
    NMVPNPluginClass parent;
} NMIPOPPluginClass;

GType nm_ipop_plugin_get_type (void);

NMIPOPPlugin *nm_ipop_plugin_new (void);

gboolean handle_options (int argc, char *argv[]);

#endif /* NM_IPOP_SERVICE_H */
