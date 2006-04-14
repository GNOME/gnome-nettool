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
#include <ifaddrs.h> /* getifaddrs () funtion */
#include <sys/ioctl.h>
#include <stdlib.h>
#include <net/if.h>

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
	GdkPixbuf *pixbuf;
	gchar *iface, *text;

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
	g_object_unref (renderer);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (info->combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (info->combo), renderer,
					"markup", 1, NULL);
	g_object_unref (renderer);

	gtk_combo_box_set_active (GTK_COMBO_BOX (info->combo), 0);
}

static gboolean
info_nic_update_stats (gpointer data)
{
	Netinfo *info = data;
	GtkTreeModel *model;
	/*
	gchar mtu[10], met[10], rx[10], rx_error[10], rx_drop[10], rx_ovr[10];
	gchar tx[10], tx_error[10], tx_drop[10], tx_ovr[10]; 
	*/
	gchar iface[30]; /*, flags[30]; */
	gchar rx_bytes[16], rx_pkt[10], rx_error[10], rx_drop[10], rx_fifo[10];
	gchar frame[10], compressed[10], multicast[10]; 
	gchar tx_bytes[16], tx_pkt[10], tx_error[10], tx_drop[10], tx_fifo[10];
	gchar collissions[10];

	GIOChannel *io = NULL;
	gchar *line;
	gboolean title = TRUE;
	const gchar *text;
	gchar *text_tx_bytes, *text_rx_bytes;
		
	g_return_val_if_fail (info != NULL, FALSE);

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (info->combo));
	text = info_get_nic (info);
	if (!text)
		return FALSE;
	
#if defined(__linux__)
	io = g_io_channel_new_file ("/proc/net/dev", "r", NULL);
	
	while (g_io_channel_read_line (io, &line, NULL, NULL, NULL) == G_IO_STATUS_NORMAL) {
		if (title) {
			title = FALSE;
			g_free (line);
			continue;
		}
		line = g_strdelimit (line, ":", ' ');
		sscanf (line, "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s", iface,
			rx_bytes, rx_pkt, rx_error, rx_drop, rx_fifo, frame, 
		    compressed, multicast, 
		    tx_bytes, tx_pkt, tx_error, tx_drop, tx_fifo, collissions);

		if (g_ascii_strcasecmp (iface, text) == 0) {
			/*
			gtk_label_set_text (GTK_LABEL (info->tx), tx);
			gtk_label_set_text (GTK_LABEL (info->tx_errors), tx_error);
			gtk_label_set_text (GTK_LABEL (info->rx), rx);
			gtk_label_set_text (GTK_LABEL (info->rx_errors), rx_error);
			*/
			text_tx_bytes = util_legible_bytes (tx_bytes);
			text_rx_bytes = util_legible_bytes (rx_bytes);
			
			gtk_label_set_text (GTK_LABEL (info->tx_bytes), text_tx_bytes);
			gtk_label_set_text (GTK_LABEL (info->tx), tx_pkt);
			gtk_label_set_text (GTK_LABEL (info->tx_errors), tx_error);
			gtk_label_set_text (GTK_LABEL (info->rx_bytes), text_rx_bytes);
			gtk_label_set_text (GTK_LABEL (info->rx), rx_pkt);
			gtk_label_set_text (GTK_LABEL (info->rx_errors), rx_error);
			gtk_label_set_text (GTK_LABEL (info->collisions), collissions);
			
			g_free (text_tx_bytes);
			g_free (text_rx_bytes);
			}
		g_free (line);
		/*info_free_nic_info (ninfo);*/
	}
	
	g_io_channel_unref (io);
#endif /* defined(__linux__) */

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
info_ip6_masklen (struct in6_addr netmask)
{
	gint len = 0;
	guchar val;
	guchar *pnt;

	pnt = (guchar *) & netmask;

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

static InfoIpAddr *
info_ip6_construct_address (const struct ifaddrs *ifr6)
{
	struct sockaddr_in6 *sinptr6;
	gchar ip_addr[INFO_ADDRSTRLEN], *scope;
	gint prefix;
	InfoIpAddr *ip6;

	sinptr6 = (struct sockaddr_in6 *) ifr6->ifa_addr;
	inet_ntop (AF_INET6, &sinptr6->sin6_addr, ip_addr, INFO_ADDRSTRLEN);

	if (IN6_IS_ADDR_LINKLOCAL (&sinptr6->sin6_addr))
		scope = g_strdup ("Link");
	else if (IN6_IS_ADDR_SITELOCAL (&sinptr6->sin6_addr))
		scope = g_strdup ("Site");
	else if (IN6_IS_ADDR_GLOBAL (&sinptr6->sin6_addr))
		scope = g_strdup ("Global");
	else if (IN6_IS_ADDR_MC_ORGLOCAL (&sinptr6->sin6_addr))
		scope = g_strdup ("Global");
	else if (IN6_IS_ADDR_V4COMPAT (&sinptr6->sin6_addr))
		scope = g_strdup ("Global");
	else if (IN6_IS_ADDR_MULTICAST (&sinptr6->sin6_addr))
		scope = g_strdup ("Global");
	else if (IN6_IS_ADDR_UNSPECIFIED (&sinptr6->sin6_addr))
		scope = g_strdup ("Global");
	else if (IN6_IS_ADDR_LOOPBACK (&sinptr6->sin6_addr))
		scope = g_strdup ("Host");
	else
		scope = g_strdup (_("Unknown"));

	sinptr6 = (struct sockaddr_in6 *) ifr6->ifa_netmask;
	prefix = info_ip6_masklen (sinptr6->sin6_addr);

	ip6 = g_new0 (InfoIpAddr, 1);
	ip6->ip_addr = g_strdup (ip_addr);
	ip6->ip_prefix = g_strdup_printf ("%d", prefix);
	ip6->ip_bcast = g_strdup ("");
	ip6->ip_scope = g_strdup (scope);

	g_free (scope);

	return (ip6);
}

static void
info_setup_configure_button (Netinfo *info, gboolean enable)
{
	if (!info->network_tool_path)
		gtk_widget_hide (info->configure_button);
	else {
		gtk_widget_show (info->configure_button);
		gtk_widget_set_sensitive (info->configure_button, enable);
	}
}

void
info_get_nic_information (const gchar *nic, Netinfo *info)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gint sockfd, len;
	gchar *ptr, buf[2048], dst[INFO_ADDRSTRLEN];
	struct ifconf ifc;
	struct ifreq *ifr = NULL, ifrcopy;
	struct ifaddrs *ifa0, *ifr6;
	struct sockaddr_in *sinptr;
	InfoIpAddr *ip;
	gint flags;
	mii_data_result data;

	getifaddrs (&ifa0);

	for (ifr6 = ifa0; ifr6; ifr6 = ifr6->ifa_next) {
		if (strcmp (ifr6->ifa_name, nic) != 0) {
			continue;
		}

		if (ifr6->ifa_addr == NULL) {
			continue;
		}

		switch (ifr6->ifa_addr->sa_family) {

		case AF_INET6:
			ip = info_ip6_construct_address (ifr6);

			model = gtk_tree_view_get_model (
				GTK_TREE_VIEW (info->list_ip_addr));

			gtk_list_store_append (GTK_LIST_STORE (model), &iter);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
					    0, "IPv6",
					    1, ip->ip_addr,
					    2, ip->ip_prefix,
					    3, ip->ip_bcast,
					    4, ip->ip_scope,
					    -1);
			info_ip_addr_free (ip);

			break;
		case AF_INET:
			/* Get the IPv4 address */
			ip = g_new0 (InfoIpAddr, 1);

			sinptr = (struct sockaddr_in *) ifr6->ifa_addr;
			inet_ntop (AF_INET, &sinptr->sin_addr, dst, INFO_ADDRSTRLEN);

			ip->ip_addr = g_strdup (dst);
			gtk_label_set_text (GTK_LABEL (info->ip_address), ip->ip_addr);
			bzero (dst, INFO_ADDRSTRLEN);

			sockfd = socket (AF_INET, SOCK_DGRAM, 0);
			ifc.ifc_len = sizeof (buf);
			ifc.ifc_req = (struct ifreq *) buf;
			ioctl (sockfd, SIOCGIFCONF, &ifc);

			data = mii_get_basic (nic);

			for (ptr = buf; ptr < buf + ifc.ifc_len;) {
				ifr = (struct ifreq *) ptr;
				len = sizeof (struct sockaddr);
#ifdef HAVE_SOCKADDR_SA_LEN
				if (ifr->ifr_addr.sa_len > len)
					len = ifr->ifr_addr.sa_len;   /* length > 16 */
#endif
				ptr += sizeof (ifr->ifr_name) + len;    /* for next one in buffer */

				if (strcmp (ifr->ifr_name, nic) == 0) {
					break;
				}
			}

			ifrcopy = *ifr;
			flags = ifrcopy.ifr_flags;

			/*Get the Hardware Address */
#ifdef SIOCGIFHWADDR
			ioctl (sockfd, SIOCGIFHWADDR, &ifrcopy);
			sinptr =
				(struct sockaddr_in *) &ifrcopy.ifr_dstaddr;
			g_sprintf (dst, "%02x:%02x:%02x:%02x:%02x:%02x",
				   (int) ((guchar *) &ifrcopy.ifr_hwaddr.sa_data)[0],
				   (int) ((guchar *) &ifrcopy.ifr_hwaddr.sa_data)[1],
				   (int) ((guchar *) &ifrcopy.ifr_hwaddr.sa_data)[2],
				   (int) ((guchar *) &ifrcopy.ifr_hwaddr.sa_data)[3],
				   (int) ((guchar *) &ifrcopy.ifr_hwaddr.sa_data)[4],
				   (int) ((guchar *) &ifrcopy.ifr_hwaddr.sa_data)[5]);
#else
			g_sprintf (dst, NOT_AVAILABLE);
#endif /* SIOCGIFHWADDR */

			gtk_label_set_text (GTK_LABEL (info->hw_address), dst);

			/* Get the netMask address */
#ifdef SIOCGIFNETMASK
			ioctl (sockfd, SIOCGIFNETMASK, &ifrcopy);

			sinptr = (struct sockaddr_in *) &ifrcopy.ifr_addr;

			/* sinptr =  (struct sockaddr_in *) &ifrcopy.ifr_netmask;*/
			inet_ntop (AF_INET, &sinptr->sin_addr, dst, INFO_ADDRSTRLEN);
#else
			g_sprintf (dst, NOT_AVAILABLE);
#endif /* SIOCGIFNETMASK */
			ip->ip_prefix = g_strdup (dst);
			gtk_label_set_text (GTK_LABEL (info->netmask), ip->ip_prefix);
			bzero (dst, INFO_ADDRSTRLEN);

			/* Get the broadcast address */
			ioctl (sockfd, SIOCGIFBRDADDR, &ifrcopy);
			sinptr = (struct sockaddr_in *) &ifrcopy.ifr_broadaddr;
			inet_ntop (AF_INET, &sinptr->sin_addr, dst, INFO_ADDRSTRLEN);
			ip->ip_bcast = g_strdup (dst);
			gtk_label_set_text (GTK_LABEL (info->broadcast), ip->ip_bcast);
			bzero (dst, INFO_ADDRSTRLEN);

			/* Get the MTU */
			ioctl (sockfd, SIOCGIFMTU, &ifrcopy);
			g_sprintf (dst, "%d", ifrcopy.ifr_mtu);
			gtk_label_set_text (GTK_LABEL (info->mtu), dst);
			bzero (dst, INFO_ADDRSTRLEN);

			/* Get Flags to determine other properties */
			ioctl (sockfd, SIOCGIFFLAGS, &ifrcopy);
			flags = ifrcopy.ifr_flags;

			/* Interface is up */
			if (flags & IFF_UP) {
				gtk_label_set_text (GTK_LABEL (info->state), _("Active"));
			} else {
				gtk_label_set_text (GTK_LABEL (info->state), _("Inactive"));
			}

			/* Is a loopback device */
			if ((flags & IFF_LOOPBACK)) {
				gtk_label_set_text (GTK_LABEL (info->hw_address), _("Loopback"));
				gtk_label_set_text (GTK_LABEL (info->broadcast), " ");
				ip->ip_bcast = g_strdup ("");
				gtk_label_set_text (GTK_LABEL (info->link_speed), " ");
				info_setup_configure_button (info, FALSE);
			} else {
				if (data.has_data) {
					gtk_label_set_text (GTK_LABEL (info->link_speed), data.media);
				} else {
					gtk_label_set_text (GTK_LABEL (info->link_speed), NOT_AVAILABLE);
				}

				info_setup_configure_button (info, TRUE);
			}

			/* Supports multicast */
			if (flags & IFF_MULTICAST) {
				gtk_label_set_text (GTK_LABEL (info->multicast), _("Enabled"));
			} else {
				gtk_label_set_text (GTK_LABEL (info->multicast), _("Disabled"));
			}

			/* Interface is a point to point link */
			if (flags & IFF_POINTOPOINT) {
				ioctl (sockfd, SIOCGIFDSTADDR, &ifrcopy);
				sinptr = (struct sockaddr_in *) &ifrcopy.ifr_dstaddr;

				printf ("\tP-t-P: %s\n",
					inet_ntop (AF_INET,
						   &sinptr->sin_addr, dst,
						   INFO_ADDRSTRLEN));
			}
			bzero (dst, INFO_ADDRSTRLEN);

			model = gtk_tree_view_get_model (
				GTK_TREE_VIEW (info->list_ip_addr));

			gtk_list_store_append (GTK_LIST_STORE (model), &iter);
			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
					    0, "IPv4",
					    1, ip->ip_addr,
					    2, ip->ip_prefix,
					    3, ip->ip_bcast,
					    4, "",
					    -1);

			info_ip_addr_free (ip);

			break;
		default:
			gtk_label_set_text (GTK_LABEL (info->hw_address), NOT_AVAILABLE);
			gtk_label_set_text (GTK_LABEL (info->ip_address), NOT_AVAILABLE);
			gtk_label_set_text (GTK_LABEL (info->broadcast), NOT_AVAILABLE);
			gtk_label_set_text (GTK_LABEL (info->netmask), NOT_AVAILABLE);
			/*gtk_label_set_text (GTK_LABEL (info->dst_address), NOT_AVAILABLE);*/
			gtk_label_set_text (GTK_LABEL (info->mtu), NOT_AVAILABLE);
			gtk_label_set_text (GTK_LABEL (info->state), NOT_AVAILABLE);
			gtk_label_set_text (GTK_LABEL (info->multicast), NOT_AVAILABLE);
			gtk_label_set_text (GTK_LABEL (info->link_speed), NOT_AVAILABLE);
			
			break;
		}
	}

	freeifaddrs (ifa0);
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
	gchar *iface;
	struct ifaddrs *ifa0, *ifr;
	gboolean ipv6 = FALSE;

	getifaddrs (&ifa0);

	for (ifr = ifa0; ifr; ifr = ifr->ifa_next) {
		iface = g_strdup (ifr->ifa_name);

		if (((ifr->ifa_flags & IFF_UP) != 0) && ifr->ifa_addr &&
		    (ifr->ifa_addr->sa_family == AF_INET6)) {
			ipv6 = TRUE;
		}

		if (((ifr->ifa_flags & IFF_UP) != 0) &&
		    (g_list_find_custom (items, iface, (GCompareFunc) compare) == NULL)) {
			items = g_list_append (items, iface);
		}
	}

	freeifaddrs (ifa0);

	if (ipv6) {
		gtk_widget_show (info->ipv6_frame);
		gtk_widget_hide (info->ipv4_frame);
	} else {
		gtk_widget_hide (info->ipv6_frame);
		gtk_widget_show (info->ipv4_frame);
	}

	return items;
}

/* Copy on clipboard */
void
info_copy_to_clipboard (Netinfo * netinfo, gpointer user_data)
{
	GString *result;
	const gchar *nic;
	const gchar *hw_address;
	const gchar *ip_address;
	const gchar *broadcast;
	const gchar *netmask;
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
	ip_address = gtk_label_get_text (GTK_LABEL (netinfo->ip_address));
	netmask = gtk_label_get_text (GTK_LABEL (netinfo->netmask));
	broadcast = gtk_label_get_text (GTK_LABEL (netinfo->broadcast));
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
	g_string_append_printf (result, _("IP address:\t%s\n"), ip_address);
	g_string_append_printf (result, _("Netmask:\t%s\n"), netmask);
	g_string_append_printf (result, _("Broadcast:\t%s\n"), broadcast);
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
