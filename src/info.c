/* -*- mode:C; indent-tabs-mode:t; tab-width:8; c-basic-offset:8; -*- */
/* gnome-netinfo - A GUI Interface for network utilities
 * Copyright (C) 2002, 2003 by German Poo-Caaman~o
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_SYS_SOCKIO_H 
#  include <sys/sockio.h>
#endif


#include <netinet/in.h>
#include <sys/socket.h>	/* basic socket definitions */
#include <arpa/inet.h>	/* inet(3) functions */
#include <sys/un.h>	/* for Unix domain sockets */
#include <sys/ioctl.h>
#include <stdlib.h>
#include <net/if.h>

#include <glibtop.h>
#include <glibtop/netlist.h>
#include <glibtop/netload.h>

#include "info.h"
#include "utils.h"
#include "util-mii.h"

#ifndef IN6_IS_ADDR_GLOBAL
#define IN6_IS_ADDR_GLOBAL(a) \
   (((((__const uint8_t *) (a))[0] & 0xff) == 0x3f   \
     || (((__const uint8_t *) (a))[0] & 0xff) == 0x20))
#endif

static gboolean info_nic_update_stats (gpointer data);
static GList   *info_get_interfaces   (Netinfo *info);

static InfoInterfaceDescription info_iface_desc [] = {
	/*  Interface Name                 Interface Type           icon          Device prefix  Pixbuf  */
	{ N_("Other type"),              INFO_INTERFACE_OTHER,   "network.png",     "other_type", NULL },
	{ N_("Ethernet Interface"),      INFO_INTERFACE_ETH,     "16_ethernet.xpm", "eth",        NULL },
	{ N_("Wireless Interface"),      INFO_INTERFACE_WLAN,    "wavelan-16.png",  "wlan",       NULL },
	{ N_("Modem Interface"),         INFO_INTERFACE_PPP,     "16_ppp.xpm",      "ppp",        NULL },
	{ N_("Parallel Line Interface"), INFO_INTERFACE_PLIP,    "16_plip.xpm",     "plip",       NULL },
	{ N_("Infrared Interface"),      INFO_INTERFACE_IRLAN,   "irda-16.png",     "irlan",      NULL },
	{ N_("Loopback Interface"),      INFO_INTERFACE_LO,      "16_loopback.xpm", "lo",         NULL },
	{ N_("Unknown Interface"),       INFO_INTERFACE_UNKNOWN, "network.png",     "",         NULL },
	{ NULL,                          INFO_INTERFACE_UNKNOWN,  NULL,             NULL,         NULL }
};

void
info_do (const gchar * nic, Netinfo * info)
{

}

void
info_set_nic (Netinfo * netinfo, const gchar *nic)
{
	GtkTreeModel    *model;
	GtkTreeIter      iter;

	g_return_if_fail (netinfo != NULL);
	
	if (nic == NULL)
		return;

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (netinfo->combo));
	if (!gtk_tree_model_get_iter_first (model, &iter)) {
		g_warning ("No network devices found.");
		return;
	}

	do {
		char *text = NULL;

		gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 2, &text, -1);
		if (!text)
			continue;

		if (strcmp (text, nic) == 0) {
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (netinfo->combo), &iter);
			return;
		}
	} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &iter));
}

gchar *
info_get_nic (Netinfo * netinfo)
{
	GtkTreeModel    *model;
	GtkTreeIter      iter;
	gchar           *nic = NULL;

	g_return_val_if_fail (netinfo != NULL, NULL);
	
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (netinfo->combo));

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (netinfo->combo), &iter))
		gtk_tree_model_get (model, &iter, 2, &nic, -1);
	else {
		g_warning ("No network devices found.");
		return NULL;
	}

	return nic;
}

static void
info_get_interface_from_dev_name (const gchar *dev_name, gchar **iface, GdkPixbuf **pixbuf)
{
	gint i;
	gchar *path;
	
	for (i = 0; info_iface_desc[i].name; i++)
		if (strstr (dev_name, info_iface_desc[i].prefix) == dev_name) {
			(*iface) = g_strdup_printf ("%s (%s)", _(info_iface_desc[i].name), dev_name);
			if (info_iface_desc[i].pixbuf == NULL) {
				path = g_build_filename (PIXMAPS_DIR, info_iface_desc[i].icon, NULL);
				info_iface_desc[i].pixbuf = gdk_pixbuf_new_from_file (path, NULL);
				g_free (path);
			}
			(*pixbuf) = info_iface_desc[i].pixbuf;
			return;
		}
}

void
info_load_iface (Netinfo *info)
{
	GtkTreeModel    *model;
	GtkTreeIter      iter;
	GtkCellRenderer *renderer;
	GList *items = NULL;
	GList *p;
	GdkPixbuf *pixbuf = NULL;
	gchar *iface = NULL;
	gchar *text;

	items = info_get_interfaces (info);
	p = items;
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (info->combo));
	
	if (!items) {
		iface = g_strdup_printf ("<i>%s</i>", _("Network Devices Not Found"));
		
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    0, NULL,
				    1, iface,
				    2, (gpointer) NULL,
				    -1);

		g_free (iface);
	} else {
		while (p) {
			text = g_strdup (p->data);
			
			info_get_interface_from_dev_name (text, &iface, &pixbuf);
			
			gtk_list_store_append (GTK_LIST_STORE (model), &iter);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
					    0, pixbuf,
					    1, iface,
					    2, (gpointer) text,
					    -1);
		
			g_free (iface);
			g_object_unref (pixbuf);

			p = g_list_next (p);
		}
		
		g_list_free (items);
	}

	gtk_cell_layout_clear (GTK_CELL_LAYOUT (info->combo));
	
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (info->combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (info->combo), renderer,
					"pixbuf", 0, NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (info->combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (info->combo), renderer,
					"markup", 1, NULL);

	gtk_combo_box_set_active (GTK_COMBO_BOX (info->combo), 0);
}

static gboolean
info_nic_update_stats (gpointer data)
{
	Netinfo *info = data;
	gchar rx_pkt[10], rx_error[10];
	gchar tx_pkt[10], tx_error[10];
	gchar collisions[10];
	const gchar *nic;
	gchar *text_tx_bytes, *text_rx_bytes;

	glibtop_netload netload;

	g_return_val_if_fail (info != NULL, FALSE);

	nic = info_get_nic (info);
	if (!nic)
		return FALSE;

	glibtop_get_netload (&netload, nic);

	text_rx_bytes = util_legible_bytes (netload.bytes_in);
	text_tx_bytes = util_legible_bytes (netload.bytes_out);

	g_sprintf (rx_pkt, "%" G_GUINT64_FORMAT, netload.packets_in);
	g_sprintf (tx_pkt, "%" G_GUINT64_FORMAT, netload.packets_out);

	g_sprintf (rx_error, "%" G_GUINT64_FORMAT, netload.errors_in);
	g_sprintf (tx_error, "%" G_GUINT64_FORMAT, netload.errors_out);

	g_sprintf (collisions, "%" G_GUINT64_FORMAT, netload.collisions);
	
	gtk_label_set_text (GTK_LABEL (info->tx_bytes), text_tx_bytes);
	gtk_label_set_text (GTK_LABEL (info->tx), tx_pkt);
	gtk_label_set_text (GTK_LABEL (info->tx_errors), tx_error);
	gtk_label_set_text (GTK_LABEL (info->rx_bytes), text_rx_bytes);
	gtk_label_set_text (GTK_LABEL (info->rx), rx_pkt);
	gtk_label_set_text (GTK_LABEL (info->rx_errors), rx_error);
	gtk_label_set_text (GTK_LABEL (info->collisions), collisions);
	
	g_free (text_tx_bytes);
	g_free (text_rx_bytes);

	return TRUE;
}

void
info_nic_changed (GtkWidget *combo, gpointer data)
{
	gchar *text = NULL;
	Netinfo *info = data;
	GtkTreeModel *model;
		
	static gint timeout_source = 0;
	
	g_return_if_fail (info != NULL);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (info->list_ip_addr));
	if (model)
		gtk_list_store_clear (GTK_LIST_STORE (model));

	text = info_get_nic (info);
	if (!text)
		return;
		
	/* Fill the NIC configuration data */
	info_get_nic_information (text, info);
	info_nic_update_stats (info);
	
	if (timeout_source > 0) {
		g_source_remove (timeout_source);
	}
	
	timeout_source = g_timeout_add (DELAY_STATS, info_nic_update_stats, info);
}

static gint
info_ip6_masklen (guint8 *netmask)
{
	gint len = 0;
	guchar val;
	guchar *pnt;

	pnt = (guchar *) netmask;

	while ((*pnt == 0xff) && len < 128) {
		len += 8;
		pnt ++;
	}

	if (len < 128) {
		val = *pnt;
		while (val) {
			len++;
			val <<= 1;
		}
	}
	
	return len;
}

typedef struct {
	   gchar *ip_addr;
	   gchar *ip_prefix;
	   gchar *ip_bcast;
	   gchar *ip_scope;
} InfoIpAddr;

static void
info_ip_addr_free (InfoIpAddr *ip)
{
	g_free (ip->ip_addr);
	g_free (ip->ip_prefix);
	g_free (ip->ip_bcast);
	g_free (ip->ip_scope);
	g_free (ip);
}

static void
info_setup_configure_button (Netinfo *info, gboolean enable)
{
	gchar *network_tool_path;

	network_tool_path = util_find_program_in_path ("nm-connection-editor", NULL);
	if (!network_tool_path)
		network_tool_path = util_find_program_in_path ("network-admin", NULL);

	if (!network_tool_path)
		gtk_widget_hide (info->configure_button);
	else {
		gtk_widget_show (info->configure_button);
		gtk_widget_set_sensitive (info->configure_button, enable);

		g_free (network_tool_path);
	}
}

void
info_get_nic_information (const gchar *nic, Netinfo *info)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gchar *dst;
	InfoIpAddr *ip;
	gint prefix;
	struct in_addr addr, subnet;
	gchar *address_string, *subnet_string;
	gchar address6_string[INET6_ADDRSTRLEN];
	glibtop_netload netload;
#ifdef __linux__
	mii_data_result data;
#endif

	gtk_label_set_text (GTK_LABEL (info->hw_address), NOT_AVAILABLE);
	gtk_label_set_text (GTK_LABEL (info->mtu), NOT_AVAILABLE);
	gtk_label_set_text (GTK_LABEL (info->state), NOT_AVAILABLE);
	gtk_label_set_text (GTK_LABEL (info->multicast), NOT_AVAILABLE);
	gtk_label_set_text (GTK_LABEL (info->link_speed), NOT_AVAILABLE);

	glibtop_get_netload (&netload, nic);

	/* IPv6 */
	/* FIXME: It shows only one IPv6 address. Bug #563768 */
	inet_ntop (AF_INET6, netload.address6, address6_string, INET6_ADDRSTRLEN);
	prefix = info_ip6_masklen (netload.prefix6);

	ip = g_new0 (InfoIpAddr, 1);
	ip->ip_addr = g_strdup (address6_string);
	ip->ip_prefix = g_strdup_printf ("%d", prefix);
	ip->ip_bcast = g_strdup ("");

	switch (netload.scope6) {
		case GLIBTOP_IF_IN6_SCOPE_LINK:
			ip->ip_scope = g_strdup ("Link");
			break;
		case GLIBTOP_IF_IN6_SCOPE_SITE:
			ip->ip_scope = g_strdup ("Site");
			break;
		case GLIBTOP_IF_IN6_SCOPE_GLOBAL:
			ip->ip_scope = g_strdup ("Global");
			break;
		case GLIBTOP_IF_IN6_SCOPE_HOST:
			ip->ip_scope = g_strdup ("Host");
			break;
		case GLIBTOP_IF_IN6_SCOPE_UNKNOWN:
			ip->ip_scope = g_strdup (_("Unknown"));
			break;
		default:
			ip->ip_scope = g_strdup (_("Unknown"));
			break;
	}

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (info->list_ip_addr));

	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
					    0, "IPv6",
					    1, ip->ip_addr,
					    2, ip->ip_prefix,
					    3, ip->ip_bcast,
					    4, ip->ip_scope,
					    -1);
	info_ip_addr_free (ip);

	/* IPv4 */
	addr.s_addr = netload.address;
	subnet.s_addr = netload.subnet;

	address_string = g_strdup (inet_ntoa (addr));
	subnet_string  = g_strdup (inet_ntoa (subnet));	
	
	ip = g_new0 (InfoIpAddr, 1);
	ip->ip_addr = g_strdup (address_string);
	ip->ip_prefix = g_strdup (subnet_string);
	/* FIXME: Get the broadcast address: Bug #563765 */
	ip->ip_bcast = g_strdup ("");

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (info->list_ip_addr));

	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
					    0, "IPv4",
					    1, ip->ip_addr,
					    2, ip->ip_prefix,
					    3, ip->ip_bcast,
					    4, "",
					    -1);

	g_free (address_string);
	g_free (subnet_string);
	info_ip_addr_free (ip);


	/* Get general information about the interface */

	/* Get the Hardware Address */
	if (netload.flags & (1L << GLIBTOP_NETLOAD_HWADDRESS)) {
		dst = g_strdup_printf ("%02x:%02x:%02x:%02x:%02x:%02x",
			   (int) ((guchar *) &netload.hwaddress)[0],
			   (int) ((guchar *) &netload.hwaddress)[1],
			   (int) ((guchar *) &netload.hwaddress)[2],
			   (int) ((guchar *) &netload.hwaddress)[3],
			   (int) ((guchar *) &netload.hwaddress)[4],
			   (int) ((guchar *) &netload.hwaddress)[5]);
	} else {
		dst = g_strdup_printf ("%s", NOT_AVAILABLE);
	}
	gtk_label_set_text (GTK_LABEL (info->hw_address), dst);
	g_free (dst);

	/* Get the interface's Maximum Transfer Unit */
	dst = g_strdup_printf ("%d", netload.mtu);
	gtk_label_set_text (GTK_LABEL (info->mtu), dst);
	g_free (dst);


	/* Get Flags to determine other properties */

	/* Is the interface up? */
	if (netload.if_flags & (1L << GLIBTOP_IF_FLAGS_UP)) {
		gtk_label_set_text (GTK_LABEL (info->state), _("Active"));
	} else {
		gtk_label_set_text (GTK_LABEL (info->state), _("Inactive"));
	}

	/* Is this a loopback device? */
	if (netload.if_flags & (1L << GLIBTOP_IF_FLAGS_LOOPBACK)) {
		dst = g_strdup_printf ("%s", _("Loopback"));
		gtk_label_set_text (GTK_LABEL (info->hw_address), dst);
		g_free (dst);
		ip->ip_bcast = g_strdup ("");
		info_setup_configure_button (info, FALSE);
	} else {
		info_setup_configure_button (info, TRUE);
	}

	/* Does this interface supports multicast? */
	if (netload.if_flags & (1L << GLIBTOP_IF_FLAGS_MULTICAST)) {
		gtk_label_set_text (GTK_LABEL (info->multicast), _("Enabled"));
	} else {
		gtk_label_set_text (GTK_LABEL (info->multicast), _("Disabled"));
	}

	/* Get the Point-To-Point address if any */
	/* FIXME: Bug #563767 */

	/* Get the link negotiation speed.  Only available on Linux
	 * systems, and lately only for with super user privileges
	 * See Bug #387198 */
#ifdef __linux__
	data = mii_get_basic (nic);
	if (data.has_data) {
		gtk_label_set_text (GTK_LABEL (info->link_speed), data.media);
	}
#else
	gtk_label_set_text (GTK_LABEL (info->link_speed), NOT_AVAILABLE);
#endif
}

static gint *
compare (gconstpointer a, gconstpointer b)
{
	return (GINT_TO_POINTER (strcmp (a, b)));
}

static GList *
info_get_interfaces (Netinfo *info)
{
	GList *items = NULL;
	glibtop_netlist netlist;
	gchar **devices;
	gchar *iface;
	guint i;

	devices = glibtop_get_netlist(&netlist);

	for(i = 0; i < netlist.number; ++i) {
		iface = g_strdup (devices[i]);
		if (g_list_find_custom (items, iface, 
					           (GCompareFunc) compare) == NULL) {
			items = g_list_append (items, iface);
		}
	}

	g_strfreev(devices);

	return items;
}

/* Copy on clipboard */
void
info_copy_to_clipboard (Netinfo * netinfo, gpointer user_data)
{
	GString *result;
	const gchar *nic;
	const gchar *hw_address;
	const gchar *multicast;
	const gchar *link_speed;
	const gchar *state;
	const gchar *mtu;
	const gchar *tx;
	const gchar *tx_errors;
	const gchar *rx;
	const gchar *rx_errors;
	const gchar *collisions;

	g_return_if_fail (netinfo != NULL);

	/* The info output in text format:
	   Bytes received, Address Source, Number of Sequence, 
	   Round Trip Time (Time), Units of Time.
	   It's a tabular output, and these belongs to the column titles */
	result = g_string_new ("");

	nic = info_get_nic (netinfo);

	hw_address = gtk_label_get_text (GTK_LABEL (netinfo->hw_address));
	multicast = gtk_label_get_text (GTK_LABEL (netinfo->multicast));
	mtu = gtk_label_get_text (GTK_LABEL (netinfo->mtu));
	link_speed = gtk_label_get_text (GTK_LABEL (netinfo->link_speed));
	state = gtk_label_get_text (GTK_LABEL (netinfo->state));

	tx = gtk_label_get_text (GTK_LABEL (netinfo->tx));
	rx = gtk_label_get_text (GTK_LABEL (netinfo->rx));
	tx_errors = gtk_label_get_text (GTK_LABEL (netinfo->tx_errors));
	rx_errors = gtk_label_get_text (GTK_LABEL (netinfo->rx_errors));
	collisions = gtk_label_get_text (GTK_LABEL (netinfo->collisions));

	/* The info output in a text format (to copy on clipboard) */
	g_string_append_printf (result, _("Network device:\t%s\n"), nic);
	g_string_append_printf (result, _("Hardware address:\t%s\n"), hw_address);
	g_string_append_printf (result, _("Multicast:\t%s\n"), multicast);
	g_string_append_printf (result, _("MTU:\t%s\n"), mtu);
	g_string_append_printf (result, _("Link speed:\t%s\n"), link_speed);
	g_string_append_printf (result, _("State:\t%s\n"), state);

	g_string_append_printf (result, _("Transmitted packets:\t%s\n"), tx);
	g_string_append_printf (result, _("Transmission errors:\t%s\n"), tx_errors);
	g_string_append_printf (result, _("Received packets:\t%s\n"), rx);
	g_string_append_printf (result, _("Reception errors:\t%s\n"), rx_errors);
	g_string_append_printf (result, _("Collisions:\t%s\n"), collisions);


	gtk_clipboard_set_text (gtk_clipboard_get (GDK_NONE), result->str,
				result->len);

	g_string_free (result, TRUE);
}
