/* gnome-netinfo - A GUI Interface for network utilities
 * Copyright (C) 2002 by German Poo-Caaman~o
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

#include "nettool.h"

#if defined(__linux__)
	/* proto 0 0 ip port ip port state */
#   define NETSTAT_PROTOCOL_FORMAT "%s %d %d %s %s %s"
#   define NETSTAT_ROUTE_FORMAT "%s %s %s %s %d %d %d %s"
#   define NETSTAT_ROUTE6_FORMAT "%s %s %s %d %d %d %s"
#   define NETSTAT_MULTICAST_FORMAT "%s %d %s"

#elif defined(__FreeBSD__)
#   define NETSTAT_PROTOCOL_FORMAT "%s %d %d %d.%d.%d.%d.%s %s %s"
#   define ALT_NETSTAT_PROTOCOL_FORMAT "%s %d %d *.%s %s %s"
#   define NETSTAT_ROUTE_FORMAT "%s %s %s %d %d %s"
#   define NETSTAT_MULTICAST_FORMAT "%s %d %s"

#endif

typedef enum {
	NONE,
	ROUTE,
	PROTOCOL,
	MULTICAST
} NetstatOption;

typedef struct _netstat_protocol_data netstat_protocol_data;
	
struct _netstat_protocol_data {
	gchar protocol[30];
	gchar ip_src[50];
	gchar port_src[30];
//	gint  port_src;
	gchar state[30];
};

typedef struct _netstat_route_data netstat_route_data;
	
struct _netstat_route_data {
	gchar destination[50];
	gchar gateway[50];
	gchar netmask[30];
	gint metric;
	gchar iface[30];
};

typedef struct _netstat_multicast_data netstat_multicast_data;
	
struct _netstat_multicast_data {
	gchar iface[30];
	gchar members[30];
	gchar group[30];
};

void netstat_do (Netinfo *netinfo);
void netstat_stop (Netinfo *netinfo);
void netstat_foreach (Netinfo * netinfo, gchar * line, gint len, gpointer user_data);
void netstat_foreach_with_tree (Netinfo * netinfo, gchar * line, gint len, gpointer user_data);
void on_protocol_button_toggled (GtkToggleButton *togglebutton, gpointer user_data);
void netstat_copy_to_clipboard (Netinfo * netinfo, gpointer user_data);
