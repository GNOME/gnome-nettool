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

#include "netinfo.h"

#define INFO_ADDRSTRLEN 128
#define NOT_AVAILABLE _("not available")
#define DELAY_STATS 1000  /* 1 second */

/* Solaris store the MTU in ifr_metric, and doesn't have 
   ifr_mtu define */
#if !defined(ifr_mtu)
#  define ifr_mtu  ifr_ifru.ifru_metric
#endif

void info_do (const gchar * nic, Netinfo * info);
void info_set_nic (Netinfo * info, const gchar *nic);
void info_load_iface (Netinfo * info, GtkWidget * combo);

void info_nic_changed (GtkEditable *editable, gpointer data);

void info_get_nic_information (const gchar *nic, Netinfo *info);
void info_copy_to_clipboard (Netinfo * netinfo, gpointer user_data);