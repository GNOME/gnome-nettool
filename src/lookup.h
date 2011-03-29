/* gnome-netinfo - A GUI Interface for network utilities
 * Copyright (C) 2003 by William Jon McCann
 * Copyright (C) 2003 by German Poo-Caaman~o
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

#define LOOKUP_OPTIONS "+nocomments +search +nocmd +nostats +noadditional"
#define LOOKUP_FORMAT "%s %d %s %s %[^\n]"
#define LOOKUP_FORMAT_MX "%s %d %s MX %s %[^\n]"
#define LOOKUP_NUM_ARGS 5

typedef struct _lookup_data lookup_data;
typedef struct _lookup_source_data lookup_source_data;

struct _lookup_data {
	gchar source[128];
	gint  ttl;
	gchar addr_type[128];
	gchar record_type[128];
	gchar destination[128];
};

struct _lookup_source_data {
	gchar device[64];
	gchar ip_src[128];
	gint port;
	gchar ip_dst[128];
	gint  hops;
};

enum {
	LOOKUP_SOURCE,
	LOOKUP_TTL,
	LOOKUP_ADDR_TYPE,
	LOOKUP_RECORD_TYPE,
	LOOKUP_DESTINATION,
	LOOKUP_NUM_COLUMNS
};

void lookup_do (Netinfo *netinfo);
void lookup_stop (Netinfo *netinfo);
void lookup_foreach (Netinfo * netinfo, gchar * line, gssize len, gpointer user_data);
void lookup_foreach_with_tree (Netinfo * netinfo, gchar * line, gint len,
			      gpointer user_data);
void lookup_copy_to_clipboard (Netinfo * netinfo, gpointer user_data);
