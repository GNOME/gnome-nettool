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
#include <glib/gprintf.h>

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
#include "utils.h"
#include "gn-combo-history.h"

/* Ping callbacks */
void
on_ping_activate (GtkWidget * widget, gpointer data)
{
	Netinfo *pinger = data;
	GtkEntry *entry_host;
	gchar *text;

	g_return_if_fail (pinger != NULL);

	if (pinger->running) {
		ping_stop (pinger);
	} else {
		if (netinfo_validate_host (pinger)) {
			entry_host = GTK_ENTRY (
				gtk_bin_get_child (GTK_BIN (pinger->host)));
			text = g_strdup (gtk_entry_get_text (entry_host));
			
			gn_combo_history_add (pinger->history, text);
			
			g_free (text);
			
			ping_do (pinger);
		}
	}
}
void
on_ping_toggled (GtkToggleButton *button, gpointer data)
{
	Netinfo *info = data;

	gtk_widget_set_sensitive (info->count,
			gtk_toggle_button_get_active (button));
}

/* Traceroute callbacks */
void
on_traceroute_activate (GtkWidget * widget, gpointer data)
{
	Netinfo *tracer = data;
	GtkEntry *entry_host;
	gchar *text;

	g_return_if_fail (tracer != NULL);

	if (tracer->running) {
		traceroute_stop (tracer);
	} else {
		if (netinfo_validate_host (tracer)) {
			entry_host = GTK_ENTRY (
				gtk_bin_get_child (GTK_BIN (tracer->host)));
			text = g_strdup (gtk_entry_get_text (entry_host));

			gn_combo_history_add (tracer->history, text);

			g_free (text);

			traceroute_do (tracer);
		}
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
void
on_configure_button_clicked (GtkButton *button, gpointer data)
{
	GString *command_line;
	GtkComboBox *combo;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkWidget *dialog;
	Netinfo *info;
	GError *error;
	gchar *nic;

	g_return_if_fail (data != NULL);
	info = (Netinfo *) data;
	g_return_if_fail (info->network_tool_path != NULL);

	combo = GTK_COMBO_BOX (info->combo);
	model = gtk_combo_box_get_model (combo);

	if (gtk_combo_box_get_active_iter (combo, &iter)) {
		gtk_tree_model_get (model, &iter, 2, &nic, -1);

		command_line = g_string_new (info->network_tool_path);
		g_string_append (command_line, " --configure ");
		g_string_append (command_line, nic);

		if (!g_spawn_command_line_async (command_line->str, &error)) {
			dialog = gtk_message_dialog_new (GTK_WINDOW (info->main_window),
							 GTK_DIALOG_DESTROY_WITH_PARENT,
							 GTK_MESSAGE_ERROR,
							 GTK_BUTTONS_CLOSE,
							 error->message);
			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
		}

		g_string_free (command_line, TRUE);
	}
}

/* Scan callbacks */
void
on_scan_activate (GtkWidget * widget, gpointer data)
{
	Netinfo *scan = data;
	GtkEntry *entry_host;
	gchar *text;

	g_return_if_fail (scan != NULL);

	if (scan->running) {
		scan_stop (scan);
	} else {
		if (netinfo_validate_host (scan)) {
			entry_host = GTK_ENTRY (
				gtk_bin_get_child (GTK_BIN (scan->host)));
			text = g_strdup (gtk_entry_get_text (entry_host));

			gn_combo_history_add (scan->history, text);

			g_free (text);

			scan_do (scan);
		}
	}
}

/* Lookup callbacks */
void
on_lookup_activate (GtkWidget * widget, gpointer data)
{
	Netinfo *lookup = data;
	GtkEntry *entry_host;
	gchar *text;

	g_return_if_fail (lookup != NULL);

	if (lookup->running) {
		lookup_stop (lookup);
	} else {
		if (netinfo_validate_domain (lookup)) {
			entry_host = GTK_ENTRY (
				gtk_bin_get_child (GTK_BIN (lookup->host)));
			text = g_strdup (gtk_entry_get_text (entry_host));

			gn_combo_history_add (lookup->history, text);

			g_free (text);

			lookup_do (lookup);
		}
	}
}

/* Finger callbacks */
void
on_finger_activate (GtkWidget * widget, gpointer data)
{
	Netinfo *finger = data;
	GtkEntry *entry_host;
	gchar *text;

	g_return_if_fail (finger != NULL);

	if (finger->running) {
		finger_stop (finger);
	} else {
		entry_host = GTK_ENTRY (
			gtk_bin_get_child (GTK_BIN (finger->host)));
		text = g_strdup (gtk_entry_get_text (entry_host));
		g_strstrip (text);

		if (g_strcasecmp (text, "") != 0)
			gn_combo_history_add (finger->history, text);
		
		g_free (text);

		entry_host = GTK_ENTRY (
			gtk_bin_get_child (GTK_BIN (finger->user)));
		text = g_strdup (gtk_entry_get_text (entry_host));
		g_strstrip (text);
		
		if (g_strcasecmp (text, "") != 0)
			gn_combo_history_add (finger->history_user, text);
		
		g_free (text);

		finger_do (finger);
	}
}

/* Whois callbacks */
void
on_whois_activate (GtkWidget * widget, gpointer data)
{
	Netinfo *whois = data;
	GtkEntry *entry_host;
	gchar *text;

	g_return_if_fail (whois != NULL);

	if (whois->running) {
		whois_stop (whois);
	} else {
		if (netinfo_validate_domain (whois)) {
			entry_host = GTK_ENTRY (
				gtk_bin_get_child (GTK_BIN (whois->host)));
			text = g_strdup (gtk_entry_get_text (entry_host));

			gn_combo_history_add (whois->history, text);

			g_free (text);

			whois_do (whois);
		}
	}
}

gboolean
gn_quit_app (GtkWidget * widget, gpointer data)
{
	gint status, pid;

	pid = getpid () + 1;
	while (waitpid (-1, &status, WNOHANG) == 0) {
		if (waitpid (pid, &status, WNOHANG) == 0)
			kill (pid, SIGKILL);
		pid ++;
	}

	netinfo_progress_indicator_stop (NULL);

	gtk_main_quit ();

	return TRUE;
}

void
on_about_activate (GtkWidget *menu_item, gpointer data)
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
	gchar copyright[1024];
	GtkWindow *parent;

	parent = (GtkWindow *) data;

	g_sprintf (copyright, "Copyright \xc2\xa9 2003-2004 %s", "Germán Poo Caamaño");
	
	if (about_box != NULL) {
		gtk_window_present (GTK_WINDOW (about_box));
		return;
	}


	{
		gchar *filename = NULL;
                                                                                
		filename = g_build_filename (PIXMAPS_DIR, "gnome-nettool.png", NULL);
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

	gtk_window_set_transient_for (GTK_WINDOW (about_box), parent);
	
	gtk_window_set_screen (GTK_WINDOW (about_box),
			       gtk_widget_get_screen (GTK_WIDGET (parent)));

	g_signal_connect (G_OBJECT (about_box), "destroy",
			  G_CALLBACK (gtk_widget_destroyed),
			  &about_box);
	
	gtk_widget_show (about_box);
}

static Netinfo *
get_netinfo_for_page (GtkNotebook * notebook, gint page_num)
{
	Netinfo *netinfo = NULL;

	switch (page_num) {
	case INFO:
		netinfo = g_object_get_data (G_OBJECT (notebook), "info");
		break;
	case PING:
		netinfo = g_object_get_data (G_OBJECT (notebook), "pinger");
		break;
	case TRACEROUTE:
		netinfo = g_object_get_data (G_OBJECT (notebook), "tracer");
		break;
	case NETSTAT:
		netinfo = g_object_get_data (G_OBJECT (notebook), "netstat");
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
		g_warning ("Unknown notebook page");
	}

	return netinfo;
}

void
on_copy_activate (GtkWidget * notebook, gpointer data)
{
	gint page;
	Netinfo *netinfo;

	g_return_if_fail (GTK_IS_NOTEBOOK (notebook));

	page = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));

	netinfo = get_netinfo_for_page (GTK_NOTEBOOK (notebook), page);
	if (!netinfo)
		return;

	if (netinfo->copy_output != NULL) {
		(netinfo->copy_output) ((gpointer) netinfo, NULL);
	}
}

void
on_clear_history_activate (GtkWidget *notebook, gpointer data)
{
	Netinfo *netinfo;

	g_return_if_fail (GTK_IS_NOTEBOOK (notebook));

	/* Pages all share a history id for host entry except whois */
	netinfo = g_object_get_data (G_OBJECT (notebook), "pinger");
	gn_combo_history_clear (netinfo->history);

	netinfo = g_object_get_data (G_OBJECT (notebook), "finger");
	gn_combo_history_clear (netinfo->history);
	gn_combo_history_clear (netinfo->history_user);

	netinfo = g_object_get_data (G_OBJECT (notebook), "whois");
	gn_combo_history_clear (netinfo->history);
}

void
on_page_switch (GtkNotebook     * notebook,
		GtkNotebookPage * page,
		guint             page_num,
		gpointer          data)
{
	Netinfo *netinfo;
	char *title;

	netinfo = get_netinfo_for_page (notebook, page_num);
	if (!netinfo)
		return;

	if (netinfo->running) {
		netinfo_progress_indicator_start (netinfo);
		if (netinfo->stbar_text) {
			gtk_statusbar_pop (GTK_STATUSBAR (netinfo->status_bar), 0);
			gtk_statusbar_push (GTK_STATUSBAR (netinfo->status_bar),
					    0, netinfo->stbar_text);
		}
	} else {
		netinfo_progress_indicator_stop (netinfo);
		gtk_statusbar_pop (GTK_STATUSBAR (netinfo->status_bar), 0);
	}

	title = g_strdup_printf ("Network Tools - %s",
				 gtk_label_get_text (GTK_LABEL (netinfo->page_label)));
	gtk_window_set_title (GTK_WINDOW (netinfo->main_window), title);
	g_free (title);
}
