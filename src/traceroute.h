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

#include "nettool.h"

#define TRACE_FORMAT "%d %s %s %s ms"
#define TRACE_PATH_FORMAT "%d: %s %s %sms"
#define TRACE_NUM_ARGS 4
#define TRACE_NUM_ERR  2

/* Try 40 hops maximum and send only 1 packet */
#define TCPTRACEROUTE_OPTIONS "-q 1 -N -m 40"
#define TRACEROUTE_OPTIONS "-q 1 -m 40"
#define TRACEPATH_OPTIONS "-b"

typedef struct _traceroute_data traceroute_data;
typedef struct _trace_source_data trace_source_data;

struct _traceroute_data {
	gint hop_count;
	gchar hostname[128];
	gchar ip[128];
	gchar rtt1[128];
	gchar rtt2[128];
};

struct _trace_source_data {
	gchar device[64];
	gchar ip_src[128];
	gint port;
	gchar ip_dst[128];
	gint  hops;
};

enum {
	TRACE_HOP,
	TRACE_HOSTNAME,
	TRACE_IP,
	TRACE_RTT1,
	TRACE_NUM_COLUMNS
};

void traceroute_do (Netinfo *netinfo);
void traceroute_stop (Netinfo *netinfo);
void traceroute_foreach (Netinfo * netinfo, gchar * line, gssize len, gpointer user_data);
void traceroute_foreach_with_tree (Netinfo * netinfo, gchar * line, gint len, gpointer user_data);
void traceroute_copy_to_clipboard (Netinfo * netinfo, gpointer user_data);
