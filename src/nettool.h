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

#ifndef __NETINFO__
#define __NETINFO__

#define NETINFO_FOREACH_FUNC(a)  (NetinfoForeachFunc) a
#define NETINFO_COPY_FUNC(a)  (NetinfoCopyFunc) a

typedef void (*NetinfoForeachFunc) (gpointer * netinfo, gchar * line, gint len,
				    gpointer user_data);
typedef void (*NetinfoCopyFunc) (gpointer * netinfo, gpointer user_data);

typedef struct _Netinfo Netinfo;

struct _Netinfo {
	GtkWidget *main_window;
	GtkWidget *progress_bar;
	GtkWidget *page_label;
	GtkWidget *user;
	GtkWidget *host;
	GtkWidget *count;
	GtkWidget *output;
	GtkWidget *limited;
	GtkWidget *button;
	GtkWidget *type;
	GtkWidget *sensitive;
	GtkWidget *routing;
	GtkWidget *protocol;
	GtkWidget *multicast;
	gboolean running;
	gint child_pid;
	gint pipe_out;
	gint pipe_err;
	gchar **command_line;
	gchar *label_run;
	gchar *label_stop;
	NetinfoForeachFunc process_line;
	NetinfoCopyFunc copy_output;
	GCallback button_callback;
	/* extra definitions for ping */
	GtkWidget *min;
	GtkWidget *max;
	GtkWidget *avg;
	GtkWidget *packets_transmitted;
	GtkWidget *packets_received;
	GtkWidget *packets_loss;
	/* extra definitions for info */
	GtkWidget *nic;
	GtkWidget *hw_address;
	GtkWidget *ip_address;
	GtkWidget *broadcast;
	GtkWidget *netmask;
	GtkWidget *dst_address;
	/*GtkWidget *multicast;*/
	GtkWidget *link_speed;
	GtkWidget *state;
	GtkWidget *mtu;
	GtkWidget *tx_bytes;
	GtkWidget *tx;
	GtkWidget *tx_errors;
	GtkWidget *rx_bytes;
	GtkWidget *rx;
	GtkWidget *rx_errors;
	GtkWidget *collisions;
};

enum {
	INACTIVE = FALSE,
	ACTIVE = TRUE
};

/* Notebook pages */
enum {
	INFO = 0,
	PING,
	NETSTAT,
	TRACEROUTE,
	PORTSCAN,
	LOOKUP,
	FINGER,
	WHOIS,
	NUM_PAGES
};

enum {
	IPV4,
	IPV6
};

#endif  /* __NETINFO__ */

/* Generic functions */
void netinfo_process_command (Netinfo * netinfo);
void netinfo_stop_process_command (Netinfo * netinfo);
void netinfo_text_buffer_insert (Netinfo * netinfo);

gushort netinfo_get_count (Netinfo * netinfo);
const gchar * netinfo_get_host (Netinfo * netinfo);
const gchar * netinfo_get_user (Netinfo * netinfo);
void netinfo_set_host (Netinfo * netinfo, const gchar *host);
gboolean netinfo_is_ipv6_enable (void);
void netinfo_set_user (Netinfo * netinfo, const gchar *user);
gint netinfo_get_ip_version (Netinfo * netinfo);
gboolean netinfo_validate_host (Netinfo * netinfo);
void netinfo_toggle_button (Netinfo * netinfo);
void netinfo_toggle_state (Netinfo * netinfo, gboolean state,
			   gpointer user_data);
gboolean netinfo_io_text_buffer_dialog (GIOChannel * channel,
					GIOCondition condition, gpointer data);

void netinfo_progress_indicator_stop (Netinfo * netinfo);

void netinfo_progress_indicator_start (Netinfo * netinfo);
