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
 
#include "nettool.h"

#define WHOIS_OPTIONS ""

void whois_do (Netinfo *netinfo);
void whois_stop (Netinfo *netinfo);
void whois_foreach (Netinfo * netinfo, gchar * line, gssize len, gpointer user_data);
void whois_copy_to_clipboard (Netinfo * netinfo, gpointer user_data);
