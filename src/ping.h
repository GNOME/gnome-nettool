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

#include <glib.h>
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "nettool.h"

/* The ping usage and output is different between Unix flavours */
/* FIXME: Add BSD support */
#if defined(__linux__) || defined(__OSF__) || defined(__FreeBSD__)
#   define PING_PROGRAM_FORMAT "%s ping -c %d -n %s"
#   define PING_PROGRAM_FORMAT_6 "%s ping6 -c %d -n %s"
#   define PING_FORMAT "%d bytes from %s icmp_seq=%d ttl=%d time=%s %s"
#   define PING_PARAMS_6
#elif defined(__sun__) 
#   define PING_PROGRAM_FORMAT "%s ping -s -n %s 56 %d"
#   define PING_FORMAT "%d bytes from %s icmp_seq=%d. time=%f %s"
#   define PING_PARAMS_5
#elif defined(__hpux__)
#   define PING_PROGRAM_FORMAT "%s ping %s -n %d"
#   define PING_FORMAT "%d bytes from %s icmp_seq=%d. time=%f %s"
#   define PING_PARAMS_5
#else
#   define PING_PROGRAM_FORMAT "%s ping -c %d -n %s"
#   define PING_FORMAT "%d bytes from %s icmp_seq=%d. time=%f %s"
#   define PING_PARAMS_5
#endif

#define PING_TOTAL "%d packets transmitted, %d %s"

typedef struct _ping_data ping_data;
	
struct _ping_data {
	gint bytes;
	gint icmp_seq;
	gint ttl;
	gfloat rtt;
	gchar srtt[128];
	gchar ip[128];
	gchar unit[128];
};

/* Ping funtions */
void ping_do (Netinfo * netinfo);
void ping_stop (Netinfo * netinfo);
void ping_foreach (Netinfo * netinfo, gchar * line, gint len, gpointer user_data);
void ping_foreach_with_tree (Netinfo * netinfo, gchar * line, gint len, gpointer user_data);
void ping_copy_to_clipboard (Netinfo * netinfo, gpointer user_data);
