/* -*- mode:C; indent-tabs-mode:t; tab-width:8; c-basic-offset:8; -*- */

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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>

#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include "callbacks.h"
#include "traceroute.h"
#include "info.h"
#include "ping.h"
#include "netstat.h"
#include "scan.h"
#include "lookup.h"
#include "finger.h"
#include "whois.h"

/* Ping callbacks */
void
on_ping_activate (GtkWidget * widget, gpointer data)
{
	Netinfo *pinger = data;

	g_return_if_fail (pinger != NULL);

	if (pinger->running) {
		ping_stop (pinger);
	} else {
		ping_do (pinger);
	}
}

/* Traceroute callbacks */
void
on_traceroute_activate (GtkWidget * widget, gpointer data)
{
	Netinfo *tracer = data;

	g_return_if_fail (tracer != NULL);

	if (tracer->running) {
		traceroute_stop (tracer);
	} else {
		traceroute_do (tracer);
	}
}

void
on_netstat_activate (GtkWidget * widget, gpointer data)
{
	Netinfo *netstat = data;

	g_return_if_fail (netstat != NULL);

	if (netstat->running) {
		netstat_stop (netstat);
	} else {
		netstat_do (netstat);
	}
}

/* Info callbacks */
#ifdef IFCONFIG_PROGRAM
void
on_info_nic_changed (GtkEntry * entry, gpointer info)
{
	const gchar *nic;

	g_return_if_fail (info != NULL);

	nic = gtk_entry_get_text (entry);

	if (strlen (nic) > 0) {
		info_do (nic, (Netinfo *) & info);
	}
}
#endif

/* Scan callbacks */
void
on_scan_activate (GtkWidget * widget, gpointer data)
{
	Netinfo *scan = data;

	g_return_if_fail (scan != NULL);

	if (scan->running) {
		scan_stop (scan);
	} else {
		scan_do (scan);
	}
}

/* Lookup callbacks */
void
on_lookup_activate (GtkWidget * widget, gpointer data)
{
	Netinfo *lookup = data;

	g_return_if_fail (lookup != NULL);

	if (lookup->running) {
		lookup_stop (lookup);
	} else {
		lookup_do (lookup);
	}
}

/* Finger callbacks */
void
on_finger_activate (GtkWidget * widget, gpointer data)
{
	Netinfo *finger = data;

	g_return_if_fail (finger != NULL);

	if (finger->running) {
		finger_stop (finger);
	} else {
		finger_do (finger);
	}
}

/* Whois callbacks */
void
on_whois_activate (GtkWidget * widget, gpointer data)
{
	Netinfo *whois = data;

	g_return_if_fail (whois != NULL);

	if (whois->running) {
		whois_stop (whois);
	} else {
		whois_do (whois);
	}
}

void
gn_quit_app (GtkWidget * widget, gpointer data)
{
	gint status, pid;

	pid = getpid () + 1;
	while (waitpid (-1, &status, WNOHANG) == 0) {
		if (waitpid (pid, &status, WNOHANG) == 0)
			kill (pid, SIGKILL);
		pid ++;
	}

	gtk_main_quit ();
}

void
on_about_activate (GtkWidget * parent, gpointer data)
{
	static GtkWidget *about_box = NULL;
	GdkPixbuf *pixbuf = NULL;
	const gchar *authors[] = { 
		"Germán Poo Caamaño <gpoo@ubiobio.cl>", 
		"William Jon McCann <mccann@jhu.edu>",
		"Carlos Garcia Campos <carlosgc@gnome.org>",
		"Rodrigo Moya <rodrigo@gnome-db.org>", NULL
	};
	const gchar *documentors[] = { NULL };
	const gchar *translator_credits = _("translator_credits");
	const gchar copyright[1024];

	g_sprintf (copyright, "Copyright \xc2\xa9 2003 %s", "Germán Poo Caamaño");
	
	if (about_box != NULL) {
		gtk_window_present (GTK_WINDOW (about_box));
		return;
	}


	{
		gchar *filename = NULL;
                                                                                
		filename = g_build_filename (GNOME_ICONDIR, "gnome-netinfo.png", NULL);
		if (filename != NULL) {
			pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
			g_free (filename);
		}
	}
                                                                                
	about_box = gnome_about_new (
		"GNOME Network Tool",
		VERSION,
		copyright,
		_("Graphical user interface for common network utilities"),
		authors, documentors,
		strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
		pixbuf);
	
        if (pixbuf != NULL)
                g_object_unref (pixbuf);

	gtk_window_set_transient_for (GTK_WINDOW (about_box),
				      GTK_WINDOW (parent));

	g_signal_connect (G_OBJECT (about_box), "destroy",
			  G_CALLBACK (gtk_widget_destroyed), &about_box);
	gtk_widget_show (about_box);
}

void
on_copy_activate (GtkWidget * notebook, gpointer data)
{
	gint page;
	Netinfo *netinfo;

	g_return_if_fail (GTK_IS_NOTEBOOK (notebook));

	page = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));

	switch (page) {
	case INFO:
		netinfo = g_object_get_data (G_OBJECT (notebook), "info");
		break;
	case PING:
		netinfo =
		    g_object_get_data (G_OBJECT (notebook), "pinger");
		break;
	case TRACEROUTE:
		netinfo =
		    g_object_get_data (G_OBJECT (notebook), "tracer");
		break;
	case NETSTAT:
		netinfo =
		    g_object_get_data (G_OBJECT (notebook), "netstat");
		break;
	case PORTSCAN:
		netinfo = g_object_get_data (G_OBJECT (notebook), "scan");
		break;
	case LOOKUP:
		netinfo = g_object_get_data (G_OBJECT (notebook), "lookup");
		break;
	case FINGER:
		netinfo = g_object_get_data (G_OBJECT (notebook), "finger");
		break;
	case WHOIS:
		netinfo = g_object_get_data (G_OBJECT (notebook), "whois");
		break;
	default:
		g_print ("default notebook page?\n");
		return;
	}
	if (netinfo->copy_output != NULL) {
		(netinfo->copy_output) ((gpointer) netinfo, NULL);
	}
}

void
on_clear_history_activate (GtkWidget * notebook, gpointer data)
{
	Netinfo *netinfo;

	g_return_if_fail (GTK_IS_NOTEBOOK (notebook));

	/* Pages all share a history id for host entry except whois */
	netinfo = g_object_get_data (G_OBJECT (notebook), "finger");
	gnome_entry_clear_history (GNOME_ENTRY (netinfo->host));
	gnome_entry_clear_history (GNOME_ENTRY (netinfo->user));

	netinfo = g_object_get_data (G_OBJECT (notebook), "whois");
	gnome_entry_clear_history (GNOME_ENTRY (netinfo->host));

}
