/* -*- Mode: C; indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8 -*- */

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
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <netdb.h>

#include "netinfo.h"
#include "utils.h"

gushort
netinfo_get_count (Netinfo * netinfo)
{

	g_return_val_if_fail (netinfo != NULL, 1);

	if (gtk_toggle_button_get_active
	    (GTK_TOGGLE_BUTTON (netinfo->limited))) {

		return (gushort)
		    gtk_spin_button_get_value (GTK_SPIN_BUTTON
					       (netinfo->count));

	} else {
		return 999;
	}
}

const gchar *
netinfo_get_host (Netinfo * netinfo)
{
	g_return_val_if_fail (netinfo != NULL, NULL);
	g_return_val_if_fail (GTK_IS_ENTRY
			      (gnome_entry_gtk_entry
			       (GNOME_ENTRY (netinfo->host))), NULL);

	return
	    gtk_entry_get_text (GTK_ENTRY
				(gnome_entry_gtk_entry
				 (GNOME_ENTRY (netinfo->host))));
}

void
netinfo_set_host (Netinfo * netinfo, const gchar *host)
{
	g_return_if_fail (netinfo != NULL);
	g_return_if_fail (GTK_IS_ENTRY
			  (gnome_entry_gtk_entry
			   (GNOME_ENTRY (netinfo->host))));

	gtk_entry_set_text (GTK_ENTRY
			    (gnome_entry_gtk_entry
			     (GNOME_ENTRY (netinfo->host))),
			    host);
}

const gchar *
netinfo_get_user (Netinfo * netinfo)
{
	g_return_val_if_fail (netinfo != NULL, NULL);
	g_return_val_if_fail (GTK_IS_ENTRY
			      (gnome_entry_gtk_entry
			       (GNOME_ENTRY (netinfo->user))), NULL);

	return
	    gtk_entry_get_text (GTK_ENTRY
				(gnome_entry_gtk_entry
				 (GNOME_ENTRY (netinfo->user))));
}

void
netinfo_set_user (Netinfo * netinfo, const gchar *user)
{
	g_return_if_fail (netinfo != NULL);
	g_return_if_fail (GTK_IS_ENTRY
			  (gnome_entry_gtk_entry
			   (GNOME_ENTRY (netinfo->user))));

	gtk_entry_set_text (GTK_ENTRY
			    (gnome_entry_gtk_entry
			     (GNOME_ENTRY (netinfo->user))),
			    user);
}

gint
netinfo_get_ip_version (Netinfo * netinfo)
{
	gchar *ip;
	struct hostent *host;

	g_return_val_if_fail (netinfo != NULL, -1);
	g_return_val_if_fail (GTK_IS_ENTRY
			      (gnome_entry_gtk_entry
			       (GNOME_ENTRY (netinfo->host))), -1);

	ip = g_strdup (gtk_entry_get_text
		       (GTK_ENTRY (gnome_entry_gtk_entry
				   (GNOME_ENTRY (netinfo->host)))));

	if (strlen (ip) > 0) {
		host = gethostbyname2 (ip, AF_INET6);
		if (host == NULL) {
			host = gethostbyname2 (ip, AF_INET);
			if (host == NULL)
				return -1;
			else {
				g_free (ip);
				return IPV4;
			}
			
			return -1;
		}
		else {
			g_free (ip);
			return IPV6;
		}

	}

	if (ip != NULL)
		g_free (ip);
	
	return -1;
}

gboolean
netinfo_validate_host (Netinfo * netinfo)
{
	struct hostent *hostname;
	const gchar *host;
	GtkWidget *dialog;
	gchar *message = NULL;

	host = netinfo_get_host (netinfo);
	if (! strcmp (host, "")) {
		message = g_strdup (_("Network address not specified"));
	}
	else {
		hostname = gethostbyname2 (host, PF_INET6);
		if (hostname == NULL) {
			hostname = gethostbyname2 (host, AF_INET);
			if (hostname == NULL) {
				message = g_strdup_printf 
					(_("The host '%s' cannot be found"),
					 host);
			}
		}
	}

	if (message != NULL) {
		dialog = gtk_message_dialog_new
			(GTK_WINDOW (netinfo->main_window),
			 GTK_DIALOG_DESTROY_WITH_PARENT,
			 GTK_MESSAGE_ERROR,
			 GTK_BUTTONS_CLOSE,
			 "<span weight=\"bold\" size=\"larger\">%s</span>",
			 message);
		gtk_label_set_use_markup 
			(GTK_LABEL (GTK_MESSAGE_DIALOG (dialog)->label), TRUE);
 		gtk_dialog_set_has_separator (GTK_DIALOG (dialog),
					      FALSE);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_free (message);
		return FALSE;
	}

	return TRUE;
}

void
netinfo_process_command (Netinfo * netinfo)
{
	g_return_if_fail (netinfo != NULL);

	netinfo_toggle_state (netinfo, INACTIVE, NULL);

	netinfo_text_buffer_insert (netinfo);
}

void
netinfo_text_buffer_insert (Netinfo * netinfo)
{
	gchar *dir = g_get_current_dir ();
	gint child_pid, pout, perr;
	GIOChannel *channel;
	GError *err = NULL;

	g_return_if_fail (netinfo != NULL);
	g_return_if_fail (netinfo->command_line != NULL);

	if (g_spawn_async_with_pipes (dir, netinfo->command_line, NULL,
				      G_SPAWN_FILE_AND_ARGV_ZERO, NULL,
				      NULL, &child_pid, NULL, &pout,
				      NULL /* &perr */ ,
				      &err)) {

		netinfo->child_pid = child_pid;
		netinfo->pipe_out = pout;
		/*netinfo->pipe_err = perr; */

		channel = g_io_channel_unix_new (pout);
		g_io_add_watch (channel,
				G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
				netinfo_io_text_buffer_dialog, netinfo);
		g_io_channel_unref (channel);

		/*channel = g_io_channel_unix_new (perr);
		   g_io_add_watch (channel,
		   G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
		   netinfo_io_text_buffer_dialog, netinfo);
		   g_io_channel_unref (channel); */
	} else {
		gint len = strlen (err->message);

		if (netinfo->process_line != NULL) {
			(netinfo->process_line) ((gpointer) netinfo,
						 err->message, len, NULL);
		}
		netinfo_toggle_state (netinfo, ACTIVE, NULL);

		g_warning ("Error: %s\n", err->message);
		g_error_free (err);
	}

	g_free (dir);
}

gboolean
netinfo_io_text_buffer_dialog (GIOChannel * channel,
			       GIOCondition condition, gpointer data)
{
	gchar *text;
	gint len;
	Netinfo *netinfo = (Netinfo *) data;

	g_return_val_if_fail (channel != NULL, FALSE);
	g_return_val_if_fail (netinfo != NULL, FALSE);
	g_return_val_if_fail (netinfo->output != NULL, FALSE);

#ifdef DEBUG
	switch (condition) {
	case G_IO_IN:
		g_print ("G_IO_IN\n");
		break;
	case G_IO_HUP:
		g_print ("G_IO_HUP\n");
		break;
	case G_IO_ERR:
		g_print ("G_IO_ERR\n");
		break;
	case G_IO_NVAL:
		g_print ("G_IO_NVAL\n");
		break;
	default:
		g_print ("Nothing\n");
		break;
	}
#endif				/* DEBUG */

	if (condition & G_IO_IN) {
		g_io_channel_read_line (channel, &text, &len, NULL, NULL);

		if (text != NULL) {
			if (netinfo->process_line != NULL) {
				(netinfo->process_line) ((gpointer) netinfo, text,
						 	len, NULL);
			}

			g_free (text);

			return TRUE;
		}
	}

	/* The condition is not G_IO_HUP | G_IO_ERR | G_IO_NVAL, so
	   we are ready to receive a new request from the user */
	close (netinfo->pipe_out);
	/*close (netinfo->pipe_err); */

	if (condition & G_IO_HUP) {
		if (netinfo->child_pid > 0)
			waitpid (netinfo->child_pid, NULL, WNOHANG);
		netinfo_toggle_state (netinfo, ACTIVE, NULL);
	}

	if (condition & G_IO_NVAL) {
		gchar *msg_nval = _("Information not available");
		int len_nval;

		len_nval = strlen (msg_nval);

		(netinfo->process_line) ((gpointer) netinfo, msg_nval,
					 len_nval, NULL);
	}
	return FALSE;
}


void
netinfo_stop_process_command (Netinfo * netinfo)
{

	g_return_if_fail (netinfo != NULL);

	if (netinfo->child_pid > 0) {
		if (kill (netinfo->child_pid, SIGINT) == 0) {
			netinfo->child_pid = 0;
		} else {
			g_warning ("%s", strerror (errno));
		}
	}
}

/* Widget behaviour */
void
netinfo_toggle_state (Netinfo * netinfo, gboolean state,
		      gpointer user_data)
{
	g_assert (netinfo != NULL);
	g_return_if_fail (netinfo != NULL);

	if (GTK_IS_WIDGET (netinfo->sensitive)) {
		gtk_widget_set_sensitive (GTK_WIDGET (netinfo->sensitive),
					  state);
	}

	if (state) {
		netinfo->child_pid = 0;
	}
	netinfo->running = !state;

	netinfo_toggle_button (netinfo);
}

/* We change the state of the button whe its clicked.  
   So, we need to recreate it (equal to the glade interface!) */
void
netinfo_toggle_button (Netinfo * netinfo)
{
	GtkWidget *parent, *alignment, *hbox, *label;
	GtkWidget *button, *icon;
	gchar *text;

	g_return_if_fail (netinfo != NULL);
	g_return_if_fail (GTK_IS_WIDGET (netinfo->button));
	g_return_if_fail (netinfo->button_callback != NULL);

	button = netinfo->button;

	parent = gtk_widget_get_parent (GTK_WIDGET (button));

	gtk_widget_destroy (button);

	button = gtk_button_new ();
	gtk_widget_show (GTK_WIDGET (button));
	gtk_container_add (GTK_CONTAINER (parent), button);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

	alignment = gtk_alignment_new (0.5, 0.5, 0, 0);
	gtk_widget_show (alignment);
	gtk_container_add (GTK_CONTAINER (button), alignment);

	hbox = gtk_hbox_new (FALSE, 2);
	gtk_widget_show (hbox);
	gtk_container_add (GTK_CONTAINER (alignment), hbox);

	/* if the program is running, we able the 'stop' option */
	if (netinfo->running) {
		icon = gtk_image_new_from_stock ("gtk-no",
						 GTK_ICON_SIZE_BUTTON);
		text =
		    (netinfo->label_stop !=
		     NULL) ? netinfo->label_run : _("Stop");
	} else {
		icon = gtk_image_new_from_stock ("gtk-ok",
						 GTK_ICON_SIZE_BUTTON);
		text =
		    (netinfo->label_run !=
		     NULL) ? netinfo->label_run : _("Run");
	}

	gtk_widget_show (icon);
	gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);

	label = gtk_label_new_with_mnemonic (text);
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

	g_signal_connect (G_OBJECT (button), "clicked",
			  netinfo->button_callback, (gpointer) netinfo);

	netinfo->button = button;
}