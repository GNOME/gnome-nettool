/* gnome-netinfo - A GUI Interface for network utilities
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

#define SCAN_FORMAT "%s %s %s"

typedef struct _scan_data scan_data;
	
struct _scan_data {
	gchar port[128];
	gchar state[128];
	gchar service[128];
};

/* Scan funtions */
void scan_do (Netinfo * netinfo);
void scan_stop (Netinfo * netinfo);
void scan_foreach (Netinfo * netinfo, gchar * line, gint len, gpointer user_data);
void scan_define_model (Netinfo * netinfo, scan_data data);
void scan_copy_to_clipboard (Netinfo * netinfo, gpointer user_data);

