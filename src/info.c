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

#include <gnome.h>
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

#include "info.h"
#include "utils.h"
#include "util-mii.h"

static gboolean info_nic_update_stats (gpointer data);
static GList * info_get_interfaces (void);

void
info_do (const gchar * nic, Netinfo * info)
{

}

void
info_set_nic (Netinfo * netinfo, const gchar *nic)
{
	GList *interfaces = NULL, *p;

	g_return_if_fail (netinfo != NULL);
	
	if (nic == NULL)
		return;

	interfaces = info_get_interfaces ();
	for (p = interfaces; p != NULL; p = p->next) {
		if (! strcmp (p->data, nic)) {
			gtk_entry_set_text (GTK_ENTRY (netinfo->nic),
					    nic);
		}
	}
	g_list_free (interfaces);
}

void
info_load_iface (Netinfo * info, GtkWidget * combo)
{
	GList *items = NULL;

	items = info_get_interfaces ();
	if (items != NULL) {
		gtk_combo_set_popdown_strings (GTK_COMBO (combo), items);
	}
	
	g_list_free (items);
}

static gboolean
info_nic_update_stats (gpointer data)
{
	Netinfo *info = data;
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
	
	text = gtk_entry_get_text (GTK_ENTRY (info->nic));
	
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
info_nic_changed (GtkEditable *editable, gpointer data)
{
	const gchar *text;
	Netinfo *info = data;
		
	static gint timeout_source = 0;
	
	g_return_if_fail (info != NULL);
	
	text = gtk_entry_get_text (GTK_ENTRY (editable));

	/* Fill the NIC configuration data */
	info_get_nic_information (text, info);
	info_nic_update_stats (data);
	
	if (timeout_source > 0) {
		g_source_remove (timeout_source);
	}
	
	timeout_source = g_timeout_add (DELAY_STATS, info_nic_update_stats, data);
}

void
info_get_nic_information (const gchar *nic, Netinfo *info)
{
	gint sockfd, len;
	gchar *ptr, buf[2048], dst[INFO_ADDRSTRLEN];
	struct ifconf ifc;
	struct ifreq *ifr = NULL, ifrcopy;
	struct sockaddr_in *sinptr;
	gint flags;
	mii_data_result data;
		
	sockfd = socket (AF_INET, SOCK_DGRAM, 0);

	ifc.ifc_len = sizeof (buf);
	ifc.ifc_req = (struct ifreq *) buf;
	ioctl (sockfd, SIOCGIFCONF, &ifc);

	for (ptr = buf; ptr < buf + ifc.ifc_len;) {
		ifr = (struct ifreq *) ptr;
		len = sizeof (struct sockaddr);
#if	defined(HAVE_SOCKADDR_SA_LEN) || defined(__FreeBSD__)
		if (ifr->ifr_addr.sa_len > len)
			len = ifr->ifr_addr.sa_len;	/* length > 16 */
#endif
		ptr += sizeof (ifr->ifr_name) + len;	/* for next one in buffer */

		if (strcmp (ifr->ifr_name, nic) != 0) {
			continue;
		}

		memset (&data, 0, sizeof(data));
		
#ifdef __linux__
		data = mii_get_basic (nic);
#endif
		
		switch (ifr->ifr_addr.sa_family) {
		case AF_INET:
			
			/* Get the IPv4 address */
			sinptr = (struct sockaddr_in *) &ifr->ifr_addr;
			inet_ntop (AF_INET, &sinptr->sin_addr, dst, INFO_ADDRSTRLEN);
			
			gtk_label_set_text (GTK_LABEL (info->ip_address), dst);
			bzero (dst, INFO_ADDRSTRLEN);
			
			ifrcopy = *ifr;
			flags = ifrcopy.ifr_flags;

			/* Get the Hardware Address */
#ifdef SIOCGIFHWADDR
			ioctl (sockfd, SIOCGIFHWADDR, &ifrcopy);
			sinptr =
				(struct sockaddr_in *) &ifrcopy.ifr_dstaddr;
			g_sprintf (dst, "%02x:%02x:%02x:%02x:%02x:%02x",
				 (int) ((guchar *) &ifrcopy.ifr_hwaddr.sa_data)[0],
				 (int) ((guchar*)  &ifrcopy.ifr_hwaddr.sa_data)[1],
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

/*			sinptr =  (struct sockaddr_in *) &ifrcopy.ifr_netmask;*/
			inet_ntop (AF_INET, &sinptr->sin_addr, dst, INFO_ADDRSTRLEN);
#else
			g_sprintf (dst, NOT_AVAILABLE);
#endif /* SIOCGIFNETMASK */
			gtk_label_set_text (GTK_LABEL (info->netmask), dst);
			bzero (dst, INFO_ADDRSTRLEN);
			
			/* Get the broadcast address */
			ioctl (sockfd, SIOCGIFBRDADDR, &ifrcopy);
			sinptr = (struct sockaddr_in *) &ifrcopy.ifr_broadaddr;
			inet_ntop (AF_INET, &sinptr->sin_addr, dst, INFO_ADDRSTRLEN);
			gtk_label_set_text (GTK_LABEL (info->broadcast), dst);
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
				gtk_label_set_text (GTK_LABEL (info->link_speed), " ");
			} else {
				if (data.has_data) {
					gtk_label_set_text (GTK_LABEL (info->link_speed), data.media);
				} else {
					gtk_label_set_text (GTK_LABEL (info->link_speed), NOT_AVAILABLE);
				}
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
			
			if (data.has_data) {
				gtk_label_set_text (GTK_LABEL (info->link_speed), data.media);
			} else {
				gtk_label_set_text (GTK_LABEL (info->link_speed), NOT_AVAILABLE);
			}
			break;
		}
	}
}

static GList *
info_get_interfaces ()
{
	GList *items = NULL;
	gchar *iface;
	gchar *ptr, buf[2048];
	struct ifconf ifc;
	struct ifreq *ifr;
	int sockfd, len;

	sockfd = socket (AF_INET, SOCK_DGRAM, 0);

	memset (&ifc, 0, sizeof (struct ifconf));
	memset (&buf, 0, sizeof (buf));
	ifc.ifc_len = sizeof (buf);
	ifc.ifc_req = (struct ifreq *) buf;

	ioctl (sockfd, SIOCGIFCONF, &ifc);

	for (ptr = buf; ptr < buf + ifc.ifc_len;) {
		ifr = (struct ifreq *) ptr;
		len = sizeof (struct sockaddr);

		iface = g_strdup (ifr->ifr_name);
		if (g_list_find_custom (items, iface, (GCompareFunc) g_ascii_strcasecmp) == NULL) {
			items = g_list_append (items, iface);
		}

#if defined(HAVE_SOCKADDR_SA_LEN) || defined(__FreeBSD__)
		if (ifr->ifr_addr.sa_len > len)
			len = ifr->ifr_addr.sa_len;	/* length > 16 */
#endif
		ptr += sizeof (ifr->ifr_name) + len;	/* for next one in buffer */
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

	nic = gtk_entry_get_text (GTK_ENTRY (netinfo->nic));
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
