/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/***************************************************************************
 *
 * Copyright (C) 2008 - 2010 Dan Williams, <dcbw@redhat.com>
 * Copyright (C) 2008 - 2012 Red Hat, Inc.
 * Copyright (C) 2008 Tambet Ingo, <tambet@gmail.com>
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
 **************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <glib/gi18n-lib.h>
#include <nm-setting-connection.h>
#include <nm-setting-8021x.h>

#include "auth-helpers.h"
#include "nm-ipop.h"
#include "src/nm-ipop-service.h"
#include "common/utils.h"

#define PW_TYPE_SAVE   0
#define PW_TYPE_ASK    1
#define PW_TYPE_UNUSED 2

//static void
//show_password (GtkToggleButton *togglebutton, GtkEntry *password_entry)
//{
//	gtk_entry_set_visibility (password_entry, gtk_toggle_button_get_active (togglebutton));
//}

//static GtkWidget *
//setup_secret_widget (GtkBuilder *builder,
//                     const char *widget_name,
//                     NMSettingVPN *s_vpn,
//                     const char *secret_key)
//{
//	NMSettingSecretFlags pw_flags = NM_SETTING_SECRET_FLAG_NONE;
//	GtkWidget *widget;
//	GtkWidget *show_passwords;
//	const char *tmp;

//	widget = GTK_WIDGET (gtk_builder_get_object (builder, widget_name));
//	g_assert (widget);

//	show_passwords = GTK_WIDGET (gtk_builder_get_object (builder, "show_passwords"));
//	g_signal_connect (show_passwords, "toggled", G_CALLBACK (show_password), widget);

//	if (s_vpn) {
//		tmp = nm_setting_vpn_get_secret (s_vpn, secret_key);
//		if (tmp)
//			gtk_entry_set_text (GTK_ENTRY (widget), tmp);

//		nm_setting_get_secret_flags (NM_SETTING (s_vpn), secret_key, &pw_flags, NULL);
//		g_object_set_data (G_OBJECT (widget), "flags", GUINT_TO_POINTER (pw_flags));
//	}

//	return widget;
//}

//static void
//tls_cert_changed_cb (GtkWidget *widget, GtkWidget *next_widget)
//{
//	GtkFileChooser *this, *next;
//	char *fname, *next_fname;

//	/* If the just-changed file chooser is a PKCS#12 file, then all of the
//	 * TLS filechoosers have to be PKCS#12.  But if it just changed to something
//	 * other than a PKCS#12 file, then clear out the other file choosers.
//	 *
//	 * Basically, all the choosers have to contain PKCS#12 files, or none of
//	 * them can, because PKCS#12 files contain everything required for the TLS
//	 * connection (CA, client cert, private key).
//	 */

//	this = GTK_FILE_CHOOSER (widget);
//	next = GTK_FILE_CHOOSER (next_widget);

//	fname = gtk_file_chooser_get_filename (this);
//	if (is_pkcs12 (fname)) {
//		/* Make sure all choosers have this PKCS#12 file */
//		next_fname = gtk_file_chooser_get_filename (next);
//		if (!next_fname || strcmp (fname, next_fname)) {
//			/* Next chooser was different, make it the same as the first */
//			gtk_file_chooser_set_filename (next, fname);
//		}
//		g_free (fname);
//		g_free (next_fname);
//		return;
//	}
//	g_free (fname);

//	/* Just-chosen file isn't PKCS#12 or no file was chosen, so clear out other
//	 * file selectors that have PKCS#12 files in them.
//	 */
//	next_fname = gtk_file_chooser_get_filename (next);
//	if (is_pkcs12 (next_fname))
//		gtk_file_chooser_set_filename (next, NULL);
//	g_free (next_fname);
//}

//static void
//tls_setup (GtkBuilder *builder,
//           GtkSizeGroup *group,
//           NMSettingVPN *s_vpn,
//           const char *prefix,
//           GtkWidget *ca_chooser,
//           ChangedCallback changed_cb,
//           gpointer user_data)
//{
//	GtkWidget *widget, *cert, *key;
//	const char *value;
//	char *tmp;
//	GtkFileFilter *filter;

//	tmp = g_strdup_printf ("%s_user_cert_chooser", prefix);
//	cert = GTK_WIDGET (gtk_builder_get_object (builder, tmp));
//	g_free (tmp);

//	gtk_size_group_add_widget (group, cert);
//	filter = tls_file_chooser_filter_new (TRUE);
//	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (cert), filter);
//	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (cert), TRUE);
//	gtk_file_chooser_button_set_title (GTK_FILE_CHOOSER_BUTTON (cert),
//	                                   _("Choose your personal certificate..."));
//	g_signal_connect (G_OBJECT (cert), "selection-changed", G_CALLBACK (changed_cb), user_data);

//	if (s_vpn) {
//		value = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_CERT);
//		if (value && strlen (value))
//			gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (cert), value);
//	}

//	tmp = g_strdup_printf ("%s_private_key_chooser", prefix);
//	key = GTK_WIDGET (gtk_builder_get_object (builder, tmp));
//	g_free (tmp);

//	gtk_size_group_add_widget (group, key);
//	filter = tls_file_chooser_filter_new (TRUE);
//	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (key), filter);
//	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (key), TRUE);
//	gtk_file_chooser_button_set_title (GTK_FILE_CHOOSER_BUTTON (key),
//	                                   _("Choose your private key..."));
//	g_signal_connect (G_OBJECT (key), "selection-changed", G_CALLBACK (changed_cb), user_data);

//	if (s_vpn) {
//		value = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_KEY);
//		if (value && strlen (value))
//			gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (key), value);
//	}

//	/* Link choosers to the PKCS#12 changer callback */
//	g_signal_connect (ca_chooser, "selection-changed", G_CALLBACK (tls_cert_changed_cb), cert);
//	g_signal_connect (cert, "selection-changed", G_CALLBACK (tls_cert_changed_cb), key);
//	g_signal_connect (key, "selection-changed", G_CALLBACK (tls_cert_changed_cb), ca_chooser);

//	/* Fill in the private key password */
//	tmp = g_strdup_printf ("%s_private_key_password_entry", prefix);
//	widget = setup_secret_widget (builder, tmp, s_vpn, NM_IPOP_KEY_CERTPASS);
//	g_free (tmp);
//	gtk_size_group_add_widget (group, widget);
//	g_signal_connect (widget, "changed", G_CALLBACK (changed_cb), user_data);
//}

//static void
//pw_type_combo_changed_cb (GtkWidget *combo, gpointer user_data)
//{
//	GtkWidget *entry = user_data;

//	/* If the user chose "Not required", desensitize and clear the correct
//	 * password entry.
//	 */
//	switch (gtk_combo_box_get_active (GTK_COMBO_BOX (combo))) {
//	case PW_TYPE_ASK:
//	case PW_TYPE_UNUSED:
//		gtk_entry_set_text (GTK_ENTRY (entry), "");
//		gtk_widget_set_sensitive (entry, FALSE);
//		break;
//	default:
//		gtk_widget_set_sensitive (entry, TRUE);
//		break;
//	}
//}

//static void
//init_one_pw_combo (GtkBuilder *builder,
//                   NMSettingVPN *s_vpn,
//                   const char *prefix,
//                   const char *secret_key,
//                   GtkWidget *entry_widget,
//                   ChangedCallback changed_cb,
//                   gpointer user_data)
//{
//	int active = -1;
//	GtkWidget *widget;
//	GtkListStore *store;
//	GtkTreeIter iter;
//	const char *value = NULL;
//	char *tmp;
//	guint32 default_idx = 1;
//	NMSettingSecretFlags pw_flags = NM_SETTING_SECRET_FLAG_NONE;

//	/* If there's already a password and the password type can't be found in
//	 * the VPN settings, default to saving it.  Otherwise, always ask for it.
//	 */
//	value = gtk_entry_get_text (GTK_ENTRY (entry_widget));
//	if (value && strlen (value))
//		default_idx = 0;

//	store = gtk_list_store_new (1, G_TYPE_STRING);
//	if (s_vpn)
//		nm_setting_get_secret_flags (NM_SETTING (s_vpn), secret_key, &pw_flags, NULL);

//	gtk_list_store_append (store, &iter);
//	gtk_list_store_set (store, &iter, 0, _("Saved"), -1);
//	if (   (active < 0)
//	    && !(pw_flags & NM_SETTING_SECRET_FLAG_NOT_SAVED)
//	    && !(pw_flags & NM_SETTING_SECRET_FLAG_NOT_REQUIRED)) {
//		active = PW_TYPE_SAVE;
//	}

//	gtk_list_store_append (store, &iter);
//	gtk_list_store_set (store, &iter, 0, _("Always Ask"), -1);
//	if ((active < 0) && (pw_flags & NM_SETTING_SECRET_FLAG_NOT_SAVED))
//		active = PW_TYPE_ASK;

//	gtk_list_store_append (store, &iter);
//	gtk_list_store_set (store, &iter, 0, _("Not Required"), -1);
//	if ((active < 0) && (pw_flags & NM_SETTING_SECRET_FLAG_NOT_REQUIRED))
//		active = PW_TYPE_UNUSED;

//	tmp = g_strdup_printf ("%s_pass_type_combo", prefix);
//	widget = GTK_WIDGET (gtk_builder_get_object (builder, tmp));
//	g_assert (widget);
//	g_free (tmp);

//	gtk_combo_box_set_model (GTK_COMBO_BOX (widget), GTK_TREE_MODEL (store));
//	g_object_unref (store);
//	gtk_combo_box_set_active (GTK_COMBO_BOX (widget), active < 0 ? default_idx : active);
//	pw_type_combo_changed_cb (widget, entry_widget);

//	g_signal_connect (G_OBJECT (widget), "changed", G_CALLBACK (pw_type_combo_changed_cb), entry_widget);
//	g_signal_connect (G_OBJECT (widget), "changed", G_CALLBACK (changed_cb), user_data);
//}

//static void
//pw_setup (GtkBuilder *builder,
//          GtkSizeGroup *group,
//          NMSettingVPN *s_vpn,
//          const char *prefix,
//          ChangedCallback changed_cb,
//          gpointer user_data)
//{
//	GtkWidget *widget;
//	const char *value;
//	char *tmp;

//	tmp = g_strdup_printf ("%s_username_entry", prefix);
//	widget = GTK_WIDGET (gtk_builder_get_object (builder, tmp));
//	g_free (tmp);
//	gtk_size_group_add_widget (group, widget);

//	if (s_vpn) {
//        value = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_XMPP_USERNAME);
//		if (value && strlen (value))
//			gtk_entry_set_text (GTK_ENTRY (widget), value);
//	}
//	g_signal_connect (G_OBJECT (widget), "changed", G_CALLBACK (changed_cb), user_data);

//	/* Fill in the user password */
//	tmp = g_strdup_printf ("%s_password_entry", prefix);
//    widget = setup_secret_widget (builder, tmp, s_vpn, NM_IPOP_KEY_XMPP_PASSWORD);
//	g_free (tmp);
//	gtk_size_group_add_widget (group, widget);
//	g_signal_connect (widget, "changed", G_CALLBACK (changed_cb), user_data);

//    init_one_pw_combo (builder, s_vpn, prefix, NM_IPOP_KEY_XMPP_PASSWORD, widget, changed_cb, user_data);
//}

//void
//tls_pw_init_auth_widget (GtkBuilder *builder,
//                         GtkSizeGroup *group,
//                         NMSettingVPN *s_vpn,
//                         const char *contype,
//                         const char *prefix,
//                         ChangedCallback changed_cb,
//                         gpointer user_data)
//{
//	GtkWidget *ca;
//	const char *value;
//	char *tmp;
//	GtkFileFilter *filter;
//	gboolean tls = FALSE, pw = FALSE;

//	g_return_if_fail (builder != NULL);
//	g_return_if_fail (group != NULL);
//	g_return_if_fail (changed_cb != NULL);
//	g_return_if_fail (prefix != NULL);

//	tmp = g_strdup_printf ("%s_ca_cert_chooser", prefix);
//	ca = GTK_WIDGET (gtk_builder_get_object (builder, tmp));
//	g_free (tmp);
//	gtk_size_group_add_widget (group, ca);

//	/* Three major connection types here: TLS-only, PW-only, and TLS + PW */
//	if (!strcmp (contype, NM_IPOP_CONTYPE_TLS) || !strcmp (contype, NM_IPOP_CONTYPE_PASSWORD_TLS))
//		tls = TRUE;
//	if (!strcmp (contype, NM_IPOP_CONTYPE_PASSWORD) || !strcmp (contype, NM_IPOP_CONTYPE_PASSWORD_TLS))
//		pw = TRUE;

//	/* Only TLS types can use PKCS#12 */
//	filter = tls_file_chooser_filter_new (tls);

//	/* Set up CA cert file picker which all connection types support */
//	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (ca), filter);
//	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (ca), TRUE);
//	gtk_file_chooser_button_set_title (GTK_FILE_CHOOSER_BUTTON (ca),
//	                                   _("Choose a Certificate Authority certificate..."));
//	g_signal_connect (G_OBJECT (ca), "selection-changed", G_CALLBACK (changed_cb), user_data);

//	if (s_vpn) {
//		value = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_CA);
//		if (value && strlen (value))
//			gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (ca), value);
//	}

//	/* Set up the rest of the options */
//	if (tls)
//		tls_setup (builder, group, s_vpn, prefix, ca, changed_cb, user_data);
//	if (pw)
//		pw_setup (builder, group, s_vpn, prefix, changed_cb, user_data);
//}

//#define SK_DIR_COL_NAME 0
//#define SK_DIR_COL_NUM  1

//void
//sk_init_auth_widget (GtkBuilder *builder,
//                     GtkSizeGroup *group,
//                     NMSettingVPN *s_vpn,
//                     ChangedCallback changed_cb,
//                     gpointer user_data)
//{
//	GtkWidget *widget;
//	const char *value = NULL;
//	GtkListStore *store;
//	GtkTreeIter iter;
//	gint active = -1;
//	gint direction = -1;
//	GtkFileFilter *filter;

//	g_return_if_fail (builder != NULL);
//	g_return_if_fail (group != NULL);
//	g_return_if_fail (changed_cb != NULL);

//	widget = GTK_WIDGET (gtk_builder_get_object (builder, "sk_key_chooser"));
//	gtk_size_group_add_widget (group, widget);
//	filter = sk_file_chooser_filter_new ();
//	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (widget), filter);
//	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (widget), TRUE);
//	gtk_file_chooser_button_set_title (GTK_FILE_CHOOSER_BUTTON (widget),
//	                                   _("Choose an IPOP static key..."));
//	g_signal_connect (G_OBJECT (widget), "selection-changed", G_CALLBACK (changed_cb), user_data);

//	if (s_vpn) {
//		value = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_STATIC_KEY);
//		if (value && strlen (value))
//			gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (widget), value);
//	}

//	store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);

//	if (s_vpn) {
//		value = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_STATIC_KEY_DIRECTION);
//		if (value && strlen (value)) {
//			long int tmp;

//			errno = 0;
//			tmp = strtol (value, NULL, 10);
//			if (errno == 0 && (tmp == 0 || tmp == 1))
//				direction = (guint32) tmp;
//		}
//	}

//	gtk_list_store_append (store, &iter);
//	gtk_list_store_set (store, &iter, SK_DIR_COL_NAME, _("None"), SK_DIR_COL_NUM, -1, -1);

//	gtk_list_store_append (store, &iter);
//	gtk_list_store_set (store, &iter, SK_DIR_COL_NAME, "0", SK_DIR_COL_NUM, 0, -1);
//	if (direction == 0)
//		active = 1;

//	gtk_list_store_append (store, &iter);
//	gtk_list_store_set (store, &iter, SK_DIR_COL_NAME, "1", SK_DIR_COL_NUM, 1, -1);
//	if (direction == 1)
//		active = 2;

//	widget = GTK_WIDGET (gtk_builder_get_object (builder, "sk_direction_combo"));
//	gtk_size_group_add_widget (group, widget);

//	gtk_combo_box_set_model (GTK_COMBO_BOX (widget), GTK_TREE_MODEL (store));
//	g_object_unref (store);
//	gtk_combo_box_set_active (GTK_COMBO_BOX (widget), active < 0 ? 0 : active);

//	widget = GTK_WIDGET (gtk_builder_get_object (builder, "sk_dir_help_label"));
//	gtk_size_group_add_widget (group, widget);

//	widget = GTK_WIDGET (gtk_builder_get_object (builder, "sk_local_address_entry"));
//	gtk_size_group_add_widget (group, widget);
//	g_signal_connect (G_OBJECT (widget), "changed", G_CALLBACK (changed_cb), user_data);
//	if (s_vpn) {
//		value = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_LOCAL_IP);
//		if (value && strlen (value))
//			gtk_entry_set_text (GTK_ENTRY (widget), value);
//	}

//	widget = GTK_WIDGET (gtk_builder_get_object (builder, "sk_remote_address_entry"));
//	gtk_size_group_add_widget (group, widget);
//	g_signal_connect (G_OBJECT (widget), "changed", G_CALLBACK (changed_cb), user_data);
//	if (s_vpn) {
//		value = nm_setting_vpn_get_data_item (s_vpn, NM_IPOP_KEY_REMOTE_IP);
//		if (value && strlen (value))
//			gtk_entry_set_text (GTK_ENTRY (widget), value);
//	}
//}

//static gboolean
//validate_file_chooser (GtkBuilder *builder, const char *name)
//{
//	GtkWidget *widget;
//	char *str;
//	gboolean valid = FALSE;

//	widget = GTK_WIDGET (gtk_builder_get_object (builder, name));
//	str = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));
//	if (str && strlen (str))
//		valid = TRUE;
//	g_free (str);
//	return valid;
//}

//static gboolean
//validate_tls (GtkBuilder *builder, const char *prefix, GError **error)
//{
//	char *tmp;
//	gboolean valid, encrypted = FALSE;
//	GtkWidget *widget;
//	char *str;

//	tmp = g_strdup_printf ("%s_ca_cert_chooser", prefix);
//	valid = validate_file_chooser (builder, tmp);
//	g_free (tmp);
//	if (!valid) {
//		g_set_error (error,
//		             IPOP_PLUGIN_UI_ERROR,
//		             IPOP_PLUGIN_UI_ERROR_INVALID_PROPERTY,
//		             NM_IPOP_KEY_CA);
//		return FALSE;
//	}

//	tmp = g_strdup_printf ("%s_user_cert_chooser", prefix);
//	valid = validate_file_chooser (builder, tmp);
//	g_free (tmp);
//	if (!valid) {
//		g_set_error (error,
//		             IPOP_PLUGIN_UI_ERROR,
//		             IPOP_PLUGIN_UI_ERROR_INVALID_PROPERTY,
//		             NM_IPOP_KEY_CERT);
//		return FALSE;
//	}

//	tmp = g_strdup_printf ("%s_private_key_chooser", prefix);
//	widget = GTK_WIDGET (gtk_builder_get_object (builder, tmp));
//	valid = validate_file_chooser (builder, tmp);
//	g_free (tmp);
//	if (!valid) {
//		g_set_error (error,
//		             IPOP_PLUGIN_UI_ERROR,
//		             IPOP_PLUGIN_UI_ERROR_INVALID_PROPERTY,
//		             NM_IPOP_KEY_KEY);
//		return FALSE;
//	}

//	/* Encrypted certificates require a password */
//	str = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));
//	encrypted = is_encrypted (str);
//	g_free (str);
//	if (encrypted) {
//		tmp = g_strdup_printf ("%s_private_key_password_entry", prefix);
//		widget = GTK_WIDGET (gtk_builder_get_object (builder, tmp));
//		g_free (tmp);

//		if (!gtk_entry_get_text_length (GTK_ENTRY (widget))) {
//			g_set_error (error,
//			             IPOP_PLUGIN_UI_ERROR,
//			             IPOP_PLUGIN_UI_ERROR_INVALID_PROPERTY,
//			             NM_IPOP_KEY_CERTPASS);
//			return FALSE;
//		}
//	}

//	return TRUE;
//}

//gboolean
//auth_widget_check_validity (GtkBuilder *builder, const char *contype, GError **error)
//{
//	GtkWidget *widget;
//	const char *str;

//	if (!strcmp (contype, NM_IPOP_CONTYPE_TLS)) {
//		if (!validate_tls (builder, "tls", error))
//			return FALSE;
//	} else if (!strcmp (contype, NM_IPOP_CONTYPE_PASSWORD_TLS)) {
//		if (!validate_tls (builder, "pw_tls", error))
//			return FALSE;

//		widget = GTK_WIDGET (gtk_builder_get_object (builder, "pw_tls_username_entry"));
//		str = gtk_entry_get_text (GTK_ENTRY (widget));
//		if (!str || !strlen (str)) {
//			g_set_error (error,
//			             IPOP_PLUGIN_UI_ERROR,
//			             IPOP_PLUGIN_UI_ERROR_INVALID_PROPERTY,
//			             NM_IPOP_KEY_USERNAME);
//			return FALSE;
//		}
//	} else if (!strcmp (contype, NM_IPOP_CONTYPE_PASSWORD)) {
//		if (!validate_file_chooser (builder, "pw_ca_cert_chooser")) {
//			g_set_error (error,
//			             IPOP_PLUGIN_UI_ERROR,
//			             IPOP_PLUGIN_UI_ERROR_INVALID_PROPERTY,
//			             NM_IPOP_KEY_CA);
//			return FALSE;
//		}
//		widget = GTK_WIDGET (gtk_builder_get_object (builder, "pw_username_entry"));
//		str = gtk_entry_get_text (GTK_ENTRY (widget));
//		if (!str || !strlen (str)) {
//			g_set_error (error,
//			             IPOP_PLUGIN_UI_ERROR,
//			             IPOP_PLUGIN_UI_ERROR_INVALID_PROPERTY,
//			             NM_IPOP_KEY_USERNAME);
//			return FALSE;
//		}
//	} else if (!strcmp (contype, NM_IPOP_CONTYPE_STATIC_KEY)) {
//		if (!validate_file_chooser (builder, "sk_key_chooser")) {
//			g_set_error (error,
//			             IPOP_PLUGIN_UI_ERROR,
//			             IPOP_PLUGIN_UI_ERROR_INVALID_PROPERTY,
//			             NM_IPOP_KEY_STATIC_KEY);
//			return FALSE;
//		}

//		widget = GTK_WIDGET (gtk_builder_get_object (builder, "sk_local_address_entry"));
//		str = gtk_entry_get_text (GTK_ENTRY (widget));
//		if (!str || !strlen (str)) {
//			g_set_error (error,
//			             IPOP_PLUGIN_UI_ERROR,
//			             IPOP_PLUGIN_UI_ERROR_INVALID_PROPERTY,
//			             NM_IPOP_KEY_LOCAL_IP);
//			return FALSE;
//		}

//		widget = GTK_WIDGET (gtk_builder_get_object (builder, "sk_remote_address_entry"));
//		str = gtk_entry_get_text (GTK_ENTRY (widget));
//		if (!str || !strlen (str)) {
//			g_set_error (error,
//			             IPOP_PLUGIN_UI_ERROR,
//			             IPOP_PLUGIN_UI_ERROR_INVALID_PROPERTY,
//			             NM_IPOP_KEY_REMOTE_IP);
//			return FALSE;
//		}
//	} else
//		g_assert_not_reached ();

//	return TRUE;
//}

//static void
//update_from_filechooser (GtkBuilder *builder,
//                         const char *key,
//                         const char *prefix,
//                         const char *widget_name,
//                         NMSettingVPN *s_vpn)
//{
//	GtkWidget *widget;
//	char *tmp, *filename;

//	g_return_if_fail (builder != NULL);
//	g_return_if_fail (key != NULL);
//	g_return_if_fail (prefix != NULL);
//	g_return_if_fail (widget_name != NULL);
//	g_return_if_fail (s_vpn != NULL);

//	tmp = g_strdup_printf ("%s_%s", prefix, widget_name);
//	widget = GTK_WIDGET (gtk_builder_get_object (builder, tmp));
//	g_free (tmp);

//	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));
//	if (filename && strlen (filename))
//		nm_setting_vpn_add_data_item (s_vpn, key, filename);
//	g_free (filename);
//}

//static void
//update_tls (GtkBuilder *builder, const char *prefix, NMSettingVPN *s_vpn)
//{
//	GtkWidget *widget;
//	NMSettingSecretFlags pw_flags;
//	char *tmp;
//	const char *str;

//	update_from_filechooser (builder, NM_IPOP_KEY_CA, prefix, "ca_cert_chooser", s_vpn);
//	update_from_filechooser (builder, NM_IPOP_KEY_CERT, prefix, "user_cert_chooser", s_vpn);
//	update_from_filechooser (builder, NM_IPOP_KEY_KEY, prefix, "private_key_chooser", s_vpn);

//	/* Password */
//	tmp = g_strdup_printf ("%s_private_key_password_entry", prefix);
//	widget = (GtkWidget *) gtk_builder_get_object (builder, tmp);
//	g_assert (widget);
//	g_free (tmp);

//	str = gtk_entry_get_text (GTK_ENTRY (widget));
//	if (str && strlen (str))
//		nm_setting_vpn_add_secret (s_vpn, NM_IPOP_KEY_CERTPASS, str);

//	pw_flags = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (widget), "flags"));
//	nm_setting_set_secret_flags (NM_SETTING (s_vpn), NM_IPOP_KEY_CERTPASS, pw_flags, NULL);
//}

//static void
//update_pw (GtkBuilder *builder, const char *prefix, NMSettingVPN *s_vpn)
//{
//	GtkWidget *widget;
//	NMSettingSecretFlags pw_flags;
//	char *tmp;
//	const char *str;

//	g_return_if_fail (builder != NULL);
//	g_return_if_fail (prefix != NULL);
//	g_return_if_fail (s_vpn != NULL);

//	tmp = g_strdup_printf ("%s_username_entry", prefix);
//	widget = GTK_WIDGET (gtk_builder_get_object (builder, tmp));
//	g_free (tmp);

//	str = gtk_entry_get_text (GTK_ENTRY (widget));
//	if (str && strlen (str))
//        nm_setting_vpn_add_data_item (s_vpn, NM_IPOP_KEY_XMPP_USERNAME, str);

//	/* Password */
//	tmp = g_strdup_printf ("%s_password_entry", prefix);
//	widget = (GtkWidget *) gtk_builder_get_object (builder, tmp);
//	g_assert (widget);
//	g_free (tmp);

//	str = gtk_entry_get_text (GTK_ENTRY (widget));
//	if (str && strlen (str))
//        nm_setting_vpn_add_secret (s_vpn, NM_IPOP_KEY_XMPP_PASSWORD, str);

//	/* Update password flags */
//	pw_flags = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (widget), "flags"));
//	pw_flags &= ~(NM_SETTING_SECRET_FLAG_NOT_SAVED | NM_SETTING_SECRET_FLAG_NOT_REQUIRED);

//	tmp = g_strdup_printf ("%s_pass_type_combo", prefix);
//	widget = GTK_WIDGET (gtk_builder_get_object (builder, tmp));
//	g_free (tmp);

//	switch (gtk_combo_box_get_active (GTK_COMBO_BOX (widget))) {
//	case PW_TYPE_SAVE:
//		break;
//	case PW_TYPE_UNUSED:
//		pw_flags |= NM_SETTING_SECRET_FLAG_NOT_REQUIRED;
//		break;
//	case PW_TYPE_ASK:
//	default:
//		pw_flags |= NM_SETTING_SECRET_FLAG_NOT_SAVED;
//		break;
//	}

//    nm_setting_set_secret_flags (NM_SETTING (s_vpn), NM_IPOP_KEY_XMPP_PASSWORD, pw_flags, NULL);
//}

//gboolean
//auth_widget_update_connection (GtkBuilder *builder,
//                               const char *contype,
//                               NMSettingVPN *s_vpn)
//{
//	GtkTreeModel *model;
//	GtkTreeIter iter;
//	GtkWidget *widget;
//	const char *str;

//	if (!strcmp (contype, NM_IPOP_CONTYPE_TLS)) {
//		update_tls (builder, "tls", s_vpn);
//	} else if (!strcmp (contype, NM_IPOP_CONTYPE_PASSWORD)) {
//		update_from_filechooser (builder, NM_IPOP_KEY_CA, "pw", "ca_cert_chooser", s_vpn);
//		update_pw (builder, "pw", s_vpn);
//	} else if (!strcmp (contype, NM_IPOP_CONTYPE_PASSWORD_TLS)) {
//		update_tls (builder, "pw_tls", s_vpn);
//		update_pw (builder, "pw_tls", s_vpn);
//	} else if (!strcmp (contype, NM_IPOP_CONTYPE_STATIC_KEY)) {
//		/* Update static key */
//		update_from_filechooser (builder, NM_IPOP_KEY_STATIC_KEY, "sk", "key_chooser", s_vpn);

//		/* Update direction */
//		widget = GTK_WIDGET (gtk_builder_get_object (builder, "sk_direction_combo"));
//		g_assert (widget);
//		model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));
//		if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter)) {
//			int direction = -1;

//			gtk_tree_model_get (model, &iter, SK_DIR_COL_NUM, &direction, -1);
//			if (direction > -1) {
//				char *tmp = g_strdup_printf ("%d", direction);
//				nm_setting_vpn_add_data_item (s_vpn, NM_IPOP_KEY_STATIC_KEY_DIRECTION, tmp);
//				g_free (tmp);
//			}
//		}

//		/* Update local address */
//		widget = GTK_WIDGET (gtk_builder_get_object (builder, "sk_local_address_entry"));
//		g_assert (widget);
//		str = gtk_entry_get_text (GTK_ENTRY (widget));
//		if (str && strlen (str))
//			nm_setting_vpn_add_data_item (s_vpn, NM_IPOP_KEY_LOCAL_IP, str);

//		/* Update remote address */
//		widget = GTK_WIDGET (gtk_builder_get_object (builder, "sk_remote_address_entry"));
//		g_assert (widget);
//		str = gtk_entry_get_text (GTK_ENTRY (widget));
//		if (str && strlen (str))
//			nm_setting_vpn_add_data_item (s_vpn, NM_IPOP_KEY_REMOTE_IP, str);
//	} else
//		g_assert_not_reached ();

//	return TRUE;
//}

static const char *
find_tag (const char *tag, const char *buf, gsize len)
{
	gsize i, taglen;

	taglen = strlen (tag);
	if (len < taglen)
		return NULL;

	for (i = 0; i < len - taglen + 1; i++) {
		if (memcmp (buf + i, tag, taglen) == 0)
			return buf + i;
	}
	return NULL;
}

static const char *pem_rsa_key_begin = "-----BEGIN RSA PRIVATE KEY-----";
static const char *pem_dsa_key_begin = "-----BEGIN DSA PRIVATE KEY-----";
static const char *pem_pkcs8_key_begin = "-----BEGIN ENCRYPTED PRIVATE KEY-----";
static const char *pem_cert_begin = "-----BEGIN CERTIFICATE-----";
static const char *pem_unenc_key_begin = "-----BEGIN PRIVATE KEY-----";

static gboolean
tls_default_filter (const GtkFileFilterInfo *filter_info, gpointer data)
{
	char *contents = NULL, *p, *ext;
	gsize bytes_read = 0;
	gboolean show = FALSE;
	gboolean pkcs_allowed = GPOINTER_TO_UINT (data);
	struct stat statbuf;

	if (!filter_info->filename)
		return FALSE;

	p = strrchr (filter_info->filename, '.');
	if (!p)
		return FALSE;

	ext = g_ascii_strdown (p, -1);
	if (!ext)
		return FALSE;

	if (pkcs_allowed && !strcmp (ext, ".p12") && is_pkcs12 (filter_info->filename)) {
		g_free (ext);
		return TRUE;
	}

	if (strcmp (ext, ".pem") && strcmp (ext, ".crt") && strcmp (ext, ".key") && strcmp (ext, ".cer")) {
		g_free (ext);
		return FALSE;
	}
	g_free (ext);

	/* Ignore files that are really large */
	if (!stat (filter_info->filename, &statbuf)) {
		if (statbuf.st_size > 500000)
			return FALSE;
	}

	if (!g_file_get_contents (filter_info->filename, &contents, &bytes_read, NULL))
		return FALSE;

	if (bytes_read < 400)  /* needs to be lower? */
		goto out;

	/* Check for PEM signatures */
	if (find_tag (pem_rsa_key_begin, (const char *) contents, bytes_read)) {
		show = TRUE;
		goto out;
	}

	if (find_tag (pem_dsa_key_begin, (const char *) contents, bytes_read)) {
		show = TRUE;
		goto out;
	}

	if (find_tag (pem_cert_begin, (const char *) contents, bytes_read)) {
		show = TRUE;
		goto out;
	}

	if (find_tag (pem_pkcs8_key_begin, (const char *) contents, bytes_read)) {
		show = TRUE;
		goto out;
	}

	if (find_tag (pem_unenc_key_begin, (const char *) contents, bytes_read)) {
		show = TRUE;
		goto out;
	}

out:
	g_free (contents);
	return show;
}

GtkFileFilter *
tls_file_chooser_filter_new (gboolean pkcs_allowed)
{
	GtkFileFilter *filter;

	filter = gtk_file_filter_new ();
	gtk_file_filter_add_custom (filter, GTK_FILE_FILTER_FILENAME, tls_default_filter, GUINT_TO_POINTER (pkcs_allowed), NULL);
	gtk_file_filter_set_name (filter, pkcs_allowed ? _("PEM or PKCS#12 certificates (*.pem, *.crt, *.key, *.cer, *.p12)")
	                                               : _("PEM certificates (*.pem, *.crt, *.key, *.cer)"));
	return filter;
}


static const char *sk_key_begin = "-----BEGIN IPOP Static key V1-----";

static gboolean
sk_default_filter (const GtkFileFilterInfo *filter_info, gpointer data)
{
	int fd;
	unsigned char buffer[1024];
	ssize_t bytes_read;
	gboolean show = FALSE;
	char *p;
	char *ext;

	if (!filter_info->filename)
		return FALSE;

	p = strrchr (filter_info->filename, '.');
	if (!p)
		return FALSE;

	ext = g_ascii_strdown (p, -1);
	if (!ext)
		return FALSE;
	if (strcmp (ext, ".key")) {
		g_free (ext);
		return FALSE;
	}
	g_free (ext);

	fd = open (filter_info->filename, O_RDONLY);
	if (fd < 0)
		return FALSE;

	bytes_read = read (fd, buffer, sizeof (buffer) - 1);
	if (bytes_read < 400)  /* needs to be lower? */
		goto out;
	buffer[bytes_read] = '\0';

	/* Check for PEM signatures */
	if (find_tag (sk_key_begin, (const char *) buffer, bytes_read)) {
		show = TRUE;
		goto out;
	}

out:
	close (fd);
	return show;
}

GtkFileFilter *
sk_file_chooser_filter_new (void)
{
	GtkFileFilter *filter;

	filter = gtk_file_filter_new ();
	gtk_file_filter_add_custom (filter, GTK_FILE_FILTER_FILENAME, sk_default_filter, NULL, NULL);
	gtk_file_filter_set_name (filter, _("IPOP Static Keys (*.key)"));
	return filter;
}

//static const char *advanced_keys[] = {
//	NM_IPOP_KEY_PORT,
//    NM_IPOP_KEY_LOCAL_IP,
//    NM_IPOP_KEY_XMPP_HOST,
//    NM_IPOP_KEY_XMPP_USERNAME,
//    NM_IPOP_KEY_XMPP_PASSWORD,
//	NULL
//};

//static void
//copy_values (const char *key, const char *value, gpointer user_data)
//{
//	GHashTable *hash = (GHashTable *) user_data;
//	const char **i;

//	for (i = &advanced_keys[0]; *i; i++) {
//		if (strcmp (key, *i))
//			continue;

//		g_hash_table_insert (hash, g_strdup (key), g_strdup (value));
//	}
//}

//GHashTable *
//advanced_dialog_new_hash_from_connection (NMConnection *connection,
//                                          GError **error)
//{
//	GHashTable *hash;
//	NMSettingVPN *s_vpn;
//	const char *secret;

//	hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

//	s_vpn = (NMSettingVPN *) nm_connection_get_setting (connection, NM_TYPE_SETTING_VPN);
//	nm_setting_vpn_foreach_data_item (s_vpn, copy_values, hash);

//	/* HTTP Proxy password is special */
//	secret = nm_setting_vpn_get_secret (s_vpn, NM_IPOP_KEY_HTTP_PROXY_PASSWORD);
//	if (secret) {
//		g_hash_table_insert (hash,
//		                     g_strdup (NM_IPOP_KEY_HTTP_PROXY_PASSWORD),
//		                     g_strdup (secret));
//	}

//	return hash;
//}

static void
port_toggled_cb (GtkWidget *check, gpointer user_data)
{
	GtkBuilder *builder = (GtkBuilder *) user_data;
	GtkWidget *widget;

	widget = GTK_WIDGET (gtk_builder_get_object (builder, "port_spinbutton"));
	gtk_widget_set_sensitive (widget, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check)));
}

//static void
//tunmtu_toggled_cb (GtkWidget *check, gpointer user_data)
//{
//	GtkBuilder *builder = (GtkBuilder *) user_data;
//	GtkWidget *widget;

//	widget = GTK_WIDGET (gtk_builder_get_object (builder, "tunmtu_spinbutton"));
//	gtk_widget_set_sensitive (widget, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check)));
//}

//static void
//fragment_toggled_cb (GtkWidget *check, gpointer user_data)
//{
//	GtkBuilder *builder = (GtkBuilder *) user_data;
//	GtkWidget *widget;

//	widget = GTK_WIDGET (gtk_builder_get_object (builder, "fragment_spinbutton"));
//	gtk_widget_set_sensitive (widget, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check)));
//}

//static void
//reneg_toggled_cb (GtkWidget *check, gpointer user_data)
//{
//	GtkBuilder *builder = (GtkBuilder *) user_data;
//	GtkWidget *widget;

//	widget = GTK_WIDGET (gtk_builder_get_object (builder, "reneg_spinbutton"));
//	gtk_widget_set_sensitive (widget, gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check)));
//}

//static const char *
//nm_find_ipop (void)
//{
//    static const char *ipop_binary_paths[] = {
//        "/usr/sbin/ipop/ipop-tincan",
//		"/sbin/ipop",
//		NULL
//	};
//	const char  **ipop_binary = ipop_binary_paths;

//	while (*ipop_binary != NULL) {
//		if (g_file_test (*ipop_binary, G_FILE_TEST_EXISTS))
//			break;
//		ipop_binary++;
//	}

//	return *ipop_binary;
//}

//#define TLS_CIPHER_COL_NAME 0
//#define TLS_CIPHER_COL_DEFAULT 1

//static void
//populate_cipher_combo (GtkComboBox *box, const char *user_cipher)
//{
//	GtkListStore *store;
//	GtkTreeIter iter;
//	const char *ipop_binary = NULL;
//	gchar *tmp, **items, **item;
//	gboolean user_added = FALSE;
//	char *argv[3];
//	GError *error = NULL;
//	gboolean success, found_blank = FALSE;

//	ipop_binary = nm_find_ipop ();
//	if (!ipop_binary)
//		return;

//	argv[0] = (char *) ipop_binary;
//	argv[1] = "--show-ciphers";
//	argv[2] = NULL;

//	success = g_spawn_sync ("/", argv, NULL, 0, NULL, NULL, &tmp, NULL, NULL, &error);
//	if (!success) {
//		g_warning ("%s: couldn't determine ciphers: %s", __func__, error->message);
//		g_error_free (error);
//		return;
//	}

//	store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);
//	gtk_combo_box_set_model (box, GTK_TREE_MODEL (store));

//	/* Add default option which won't pass --cipher to ipop */
//	gtk_list_store_append (store, &iter);
//	gtk_list_store_set (store, &iter,
//	                    TLS_CIPHER_COL_NAME, _("Default"),
//	                    TLS_CIPHER_COL_DEFAULT, TRUE, -1);

//	items = g_strsplit (tmp, "\n", 0);
//	g_free (tmp);

//	for (item = items; *item; item++) {
//		char *space = strchr (*item, ' ');

//		/* Don't add anything until after the first blank line */
//		if (!found_blank) {
//			if (!strlen (*item))
//				found_blank = TRUE;
//			continue;
//		}

//		if (space)
//			*space = '\0';

//		if (strlen (*item)) {
//			gtk_list_store_append (store, &iter);
//			gtk_list_store_set (store, &iter,
//			                    TLS_CIPHER_COL_NAME, *item,
//			                    TLS_CIPHER_COL_DEFAULT, FALSE, -1);
//			if (user_cipher && !strcmp (*item, user_cipher)) {
//				gtk_combo_box_set_active_iter (box, &iter);
//				user_added = TRUE;
//			}
//		}
//	}

//	/* Add the user-specified cipher if it exists wasn't found by ipop */
//	if (user_cipher && !user_added) {
//		gtk_list_store_insert (store, &iter, 1);
//		gtk_list_store_set (store, &iter,
//		                    TLS_CIPHER_COL_NAME, user_cipher,
//		                    TLS_CIPHER_COL_DEFAULT, FALSE -1);
//		gtk_combo_box_set_active_iter (box, &iter);
//	} else if (!user_added) {
//		gtk_combo_box_set_active (box, 0);
//	}

//	g_object_unref (G_OBJECT (store));
//	g_strfreev (items);
//}

//#define HMACAUTH_COL_NAME 0
//#define HMACAUTH_COL_VALUE 1
//#define HMACAUTH_COL_DEFAULT 2

//static void
//populate_hmacauth_combo (GtkComboBox *box, const char *hmacauth)
//{
//	GtkListStore *store;
//	GtkTreeIter iter;
//	gboolean active_initialized = FALSE;
//	const char **item;
//	static const char *items[] = {
//		NM_IPOP_AUTH_NONE,
//		NM_IPOP_AUTH_RSA_MD4,
//		NM_IPOP_AUTH_MD5,
//		NM_IPOP_AUTH_SHA1,
//		NM_IPOP_AUTH_SHA224,
//		NM_IPOP_AUTH_SHA256,
//		NM_IPOP_AUTH_SHA384,
//		NM_IPOP_AUTH_SHA512,
//		NM_IPOP_AUTH_RIPEMD160,
//		NULL
//	};

//	store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);
//	gtk_combo_box_set_model (box, GTK_TREE_MODEL (store));

//	/* Add default option which won't pass --auth to ipop */
//	gtk_list_store_append (store, &iter);
//	gtk_list_store_set (store, &iter,
//	                    HMACAUTH_COL_NAME, _("Default"),
//	                    HMACAUTH_COL_DEFAULT, TRUE, -1);

//	/* Add options */
//	for (item = items; *item; item++) {
//		const char *name = NULL;

//		if (!strcmp (*item, NM_IPOP_AUTH_NONE))
//			name = _("None");
//		else if (!strcmp (*item, NM_IPOP_AUTH_RSA_MD4))
//			name = _("RSA MD-4");
//		else if (!strcmp (*item, NM_IPOP_AUTH_MD5))
//			name = _("MD-5");
//		else if (!strcmp (*item, NM_IPOP_AUTH_SHA1))
//			name = _("SHA-1");
//		else if (!strcmp (*item, NM_IPOP_AUTH_SHA224))
//			name = _("SHA-224");
//		else if (!strcmp (*item, NM_IPOP_AUTH_SHA256))
//			name = _("SHA-256");
//		else if (!strcmp (*item, NM_IPOP_AUTH_SHA384))
//			name = _("SHA-384");
//		else if (!strcmp (*item, NM_IPOP_AUTH_SHA512))
//			name = _("SHA-512");
//		else if (!strcmp (*item, NM_IPOP_AUTH_RIPEMD160))
//			name = _("RIPEMD-160");
//		else
//			g_assert_not_reached ();

//		gtk_list_store_append (store, &iter);
//		gtk_list_store_set (store, &iter,
//		                    HMACAUTH_COL_NAME, name,
//		                    HMACAUTH_COL_VALUE, *item,
//		                    HMACAUTH_COL_DEFAULT, FALSE, -1);
//		if (hmacauth && !strcmp (*item, hmacauth)) {
//			gtk_combo_box_set_active_iter (box, &iter);
//			active_initialized = TRUE;
//		}
//	}

//	if (!active_initialized)
//		gtk_combo_box_set_active (box, 0);

//	g_object_unref (store);
//}

//static void
//tls_auth_toggled_cb (GtkWidget *widget, gpointer user_data)
//{
//	GtkBuilder *builder = (GtkBuilder *) user_data;
//	gboolean use_auth = FALSE;

//	widget = GTK_WIDGET (gtk_builder_get_object (builder, "tls_auth_checkbutton"));
//	use_auth = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

//	widget = GTK_WIDGET (gtk_builder_get_object (builder, "tls_dir_help_label"));
//	gtk_widget_set_sensitive (widget, use_auth);
//	widget = GTK_WIDGET (gtk_builder_get_object (builder, "direction_label"));
//	gtk_widget_set_sensitive (widget, use_auth);
//	widget = GTK_WIDGET (gtk_builder_get_object (builder, "tls_auth_label"));
//	gtk_widget_set_sensitive (widget, use_auth);
//	widget = GTK_WIDGET (gtk_builder_get_object (builder, "tls_auth_chooser"));
//	gtk_widget_set_sensitive (widget, use_auth);
//	widget = GTK_WIDGET (gtk_builder_get_object (builder, "direction_combo"));
//	gtk_widget_set_sensitive (widget, use_auth);
//}

//#define PROXY_TYPE_NONE  0
//#define PROXY_TYPE_HTTP  1
//#define PROXY_TYPE_SOCKS 2

//static void
//proxy_type_changed (GtkComboBox *combo, gpointer user_data)
//{
//	GtkBuilder *builder = GTK_BUILDER (user_data);
//	gboolean sensitive;
//	GtkWidget *widget;
//	guint32 i = 0;
//	int active;
//	const char *widgets[] = {
//		"proxy_desc_label", "proxy_server_label", "proxy_server_entry",
//		"proxy_port_label", "proxy_port_spinbutton", "proxy_retry_checkbutton",
//		"proxy_username_label", "proxy_password_label", "proxy_username_entry",
//		"proxy_password_entry", "show_proxy_password", NULL
//	};
//	const char *user_pass_widgets[] = {
//		"proxy_username_label", "proxy_password_label", "proxy_username_entry",
//		"proxy_password_entry", "show_proxy_password", NULL
//	};

//	active = gtk_combo_box_get_active (combo);
//	sensitive = (active > PROXY_TYPE_NONE);

//	while (widgets[i]) {
//		widget = GTK_WIDGET (gtk_builder_get_object (builder, widgets[i++]));
//		gtk_widget_set_sensitive (widget, sensitive);
//	}

//	/* Additionally user/pass widgets need to be disabled for SOCKS */
//	if (active == PROXY_TYPE_SOCKS) {
//		i = 0;
//		while (user_pass_widgets[i]) {
//			widget = GTK_WIDGET (gtk_builder_get_object (builder, user_pass_widgets[i++]));
//			gtk_widget_set_sensitive (widget, FALSE);
//		}
//	}

//	/* Proxy options require TCP; but don't reset the TCP checkbutton
//	 * to false when the user disables HTTP proxy; leave it checked.
//	 */
//	widget = GTK_WIDGET (gtk_builder_get_object (builder, "tcp_checkbutton"));
//	if (sensitive == TRUE)
//		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
//	gtk_widget_set_sensitive (widget, !sensitive);
//}

//static void
//show_proxy_password_toggled_cb (GtkCheckButton *button, gpointer user_data)
//{
//	GtkBuilder *builder = (GtkBuilder *) user_data;
//	GtkWidget *widget;
//	gboolean visible;

//	widget = GTK_WIDGET (gtk_builder_get_object (builder, "proxy_password_entry"));
//	g_assert (widget);

//	visible = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));
//	gtk_entry_set_visibility (GTK_ENTRY (widget), visible);
//}

#define TA_DIR_COL_NAME 0
#define TA_DIR_COL_NUM 1

GtkWidget *
advanced_dialog_new (GHashTable *hash, const char *contype)
{
	GtkBuilder *builder;
	GtkWidget *dialog = NULL;
	char *ui_file = NULL;
    GtkWidget *widget;
    const char *value;
	GError *error = NULL;

	g_return_val_if_fail (hash != NULL, NULL);

	ui_file = g_strdup_printf ("%s/%s", UIDIR, "nm-ipop-dialog.ui");
	builder = gtk_builder_new ();

	gtk_builder_set_translation_domain (builder, GETTEXT_PACKAGE);

	if (!gtk_builder_add_from_file (builder, ui_file, &error)) {
		g_warning ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
		g_object_unref (G_OBJECT (builder));
		goto out;
	}

	dialog = GTK_WIDGET (gtk_builder_get_object (builder, "ipop-advanced-dialog"));
	if (!dialog) {
		g_object_unref (G_OBJECT (builder));
		goto out;
	}
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

	g_object_set_data_full (G_OBJECT (dialog), "builder",
	                        builder, (GDestroyNotify) g_object_unref);
	g_object_set_data (G_OBJECT (dialog), "connection-type", GINT_TO_POINTER (contype));

    //widget = GTK_WIDGET (gtk_builder_get_object (builder, "reneg_checkbutton"));
    //g_signal_connect (G_OBJECT (widget), "toggled", G_CALLBACK (reneg_toggled_cb), builder);

//	value = g_hash_table_lookup (hash, NM_IPOP_KEY_RENEG_SECONDS);
//	if (value && strlen (value)) {
//		long int tmp;

//		errno = 0;
//		tmp = strtol (value, NULL, 10);
//		if (errno == 0 && tmp >= 0 && tmp <= 604800) {  /* up to a week? */
//			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

//			widget = GTK_WIDGET (gtk_builder_get_object (builder, "reneg_spinbutton"));
//			gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), (gdouble) tmp);
//		}
//		gtk_widget_set_sensitive (widget, TRUE);
//	} else {
//		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);

//		widget = GTK_WIDGET (gtk_builder_get_object (builder, "reneg_spinbutton"));
//		gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), 0.0);
//		gtk_widget_set_sensitive (widget, FALSE);
//	}

	widget = GTK_WIDGET (gtk_builder_get_object (builder, "port_checkbutton"));
	g_signal_connect (G_OBJECT (widget), "toggled", G_CALLBACK (port_toggled_cb), builder);

	value = g_hash_table_lookup (hash, NM_IPOP_KEY_PORT);
	if (value && strlen (value)) {
		long int tmp;

		errno = 0;
		tmp = strtol (value, NULL, 10);
		if (errno == 0 && tmp > 0 && tmp < 65536 && tmp != 1194) {
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

			widget = GTK_WIDGET (gtk_builder_get_object (builder, "port_spinbutton"));
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget),
			                           (gdouble) tmp);
		}
		gtk_widget_set_sensitive (widget, TRUE);
	} else {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);

		widget = GTK_WIDGET (gtk_builder_get_object (builder, "port_spinbutton"));
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), 1194.0);
		gtk_widget_set_sensitive (widget, FALSE);
	}

//	value = g_hash_table_lookup (hash, NM_IPOP_KEY_XMPP_HOST);
//	if (value && !strcmp (value, "yes")) {
//		widget = GTK_WIDGET (gtk_builder_get_object (builder, "mssfix_checkbutton"));
//		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
//	}

out:
	g_free (ui_file);
	return dialog;
}

GHashTable *
advanced_dialog_new_hash_from_dialog (GtkWidget *dialog, GError **error)
{
	GHashTable *hash;
	GtkWidget *widget;
    GtkBuilder *builder;

	g_return_val_if_fail (dialog != NULL, NULL);
	if (error)
		g_return_val_if_fail (*error == NULL, NULL);

	builder = g_object_get_data (G_OBJECT (dialog), "builder");
	g_return_val_if_fail (builder != NULL, NULL);

	hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

//	widget = GTK_WIDGET (gtk_builder_get_object (builder, "reneg_checkbutton"));
//	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
//		int reneg_seconds;

//		widget = GTK_WIDGET (gtk_builder_get_object (builder, "reneg_spinbutton"));
//		reneg_seconds = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget));
//		g_hash_table_insert (hash, g_strdup (NM_IPOP_KEY_RENEG_SECONDS), g_strdup_printf ("%d", reneg_seconds));
//	}

	widget = GTK_WIDGET (gtk_builder_get_object (builder, "port_checkbutton"));
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
		int port;

		widget = GTK_WIDGET (gtk_builder_get_object (builder, "port_spinbutton"));
		port = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget));
		g_hash_table_insert (hash, g_strdup (NM_IPOP_KEY_PORT), g_strdup_printf ("%d", port));
	}

//	widget = GTK_WIDGET (gtk_builder_get_object (builder, "lzo_checkbutton"));
//	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
//		g_hash_table_insert (hash, g_strdup (NM_IPOP_KEY_COMP_LZO), g_strdup ("yes"));


	return hash;
}

