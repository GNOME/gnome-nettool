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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <netdb.h>
#include <fcntl.h>
#include <netinet/in.h>

#include "nettool.h"
#include "utils.h"

guint progress_timeout_id = 0;

gint
netinfo_get_count (Netinfo * netinfo)
{

	g_return_val_if_fail (netinfo != NULL, 1);

	if (gtk_toggle_button_get_active
	    (GTK_TOGGLE_BUTTON (netinfo->limited))) {

		return (gushort)
		    gtk_spin_button_get_value (GTK_SPIN_BUTTON
					       (netinfo->count));

	} else {
		return -1;
	}
}

const gchar *
netinfo_get_host (Netinfo * netinfo)
{
	g_return_val_if_fail (netinfo != NULL, NULL);
	g_return_val_if_fail (GTK_IS_ENTRY
			      (gtk_bin_get_child
			       (GTK_BIN (netinfo->host))), NULL);

	return
		gtk_entry_get_text (GTK_ENTRY
				    (gtk_bin_get_child
				     (GTK_BIN (netinfo->host))));
}

void
netinfo_set_host (Netinfo * netinfo, const gchar *host)
{
	g_return_if_fail (netinfo != NULL);
	g_return_if_fail (GN_IS_COMBO_HISTORY (netinfo->history));

	gn_combo_history_add (GN_COMBO_HISTORY (netinfo->history), host);
}

gboolean
netinfo_is_ipv6_enable (void)
{
	gint sock;
	struct sockaddr_in6 sin6;
	guint len;

	if ((sock = socket (PF_INET6, SOCK_STREAM, 0)) == -1) {
		return FALSE;
	} else {
		len = sizeof (struct sockaddr_in6);
		if (getsockname (sock, (struct sockaddr *)&sin6, (void *)&len) < 0) {
			close (sock);
			return FALSE;
		} else {
			close (sock);
			return TRUE;
		}
	}
}

const gchar *
netinfo_get_user (Netinfo * netinfo)
{
	g_return_val_if_fail (netinfo != NULL, NULL);
	g_return_val_if_fail (GTK_IS_ENTRY
			      (gtk_bin_get_child 
			       (GTK_BIN (netinfo->user))), NULL);

	return
	    gtk_entry_get_text (GTK_ENTRY
				(gtk_bin_get_child
				 (GTK_BIN (netinfo->user))));
}

void
netinfo_set_user (Netinfo * netinfo, const gchar *user)
{
	g_return_if_fail (netinfo != NULL);
	g_return_if_fail (GN_IS_COMBO_HISTORY (netinfo->history_user));

	gn_combo_history_add (GN_COMBO_HISTORY (netinfo->history_user), user);
}

gint
netinfo_get_ip_version (Netinfo * netinfo)
{
	gchar *ip;
	struct hostent *host;

	g_return_val_if_fail (netinfo != NULL, -1);
	g_return_val_if_fail (GTK_IS_ENTRY
			      (gtk_bin_get_child
			       (GTK_BIN (netinfo->host))), -1);

	ip = g_strdup (gtk_entry_get_text
		       (GTK_ENTRY (gtk_bin_get_child
				   (GTK_BIN (netinfo->host)))));

	if (strlen (ip) > 0) {
		host = gethostbyname2 (ip, AF_INET);
		if (host == NULL) {
			host = gethostbyname2 (ip, AF_INET6);
			if (host == NULL)
				return -1;
			else {
				g_free (ip);
				return IPV6;
			}
			
			return -1;
		}
		else {
			g_free (ip);
			return IPV4;
		}

	}

	if (ip != NULL)
		g_free (ip);
	
	return -1;
}

void
netinfo_error_message (Netinfo     * netinfo,
		       const gchar * primary,
		       const gchar * secondary)
{
	GtkWidget *dialog;
 
	g_return_if_fail (primary != NULL);

	dialog = gtk_message_dialog_new (GTK_WINDOW (netinfo->main_window),
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_CLOSE,
					 "%s", primary);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  "%s", secondary ? secondary : " ");
	gtk_window_set_title (GTK_WINDOW (dialog), "");

        gtk_container_set_border_width (GTK_CONTAINER (dialog), 6);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

gboolean
netinfo_validate_host (Netinfo * netinfo)
{
	struct hostent *hostname;
	const gchar    *host;
	gchar          *primary = NULL;
	gchar          *secondary = NULL;

	host = netinfo_get_host (netinfo);
	if (! strcmp (host, "")) {
		primary = g_strdup (_("A network address was not specified"));
		secondary = g_strdup (_("Please enter a valid network address and try again."));
	}
	else {
		hostname = gethostbyname2 (host, PF_INET);
		if (hostname == NULL) {
			hostname = gethostbyname2 (host, PF_INET6);
			if (hostname == NULL) {
				primary = g_strdup_printf (_("The address '%s' cannot be found"),
							   host);
				secondary = g_strdup (_("Please enter a valid network address and try again."));
			}
		}
	}

	if (primary) {
		netinfo_error_message (netinfo, primary, secondary);
		g_free (primary);
		if (secondary)
			g_free (secondary);
		return FALSE;
	}

	return TRUE;
}

gboolean
netinfo_validate_domain (Netinfo * netinfo)
{
	gchar *domain;
	gchar *primary = NULL;
	gchar *secondary = NULL;

	domain = g_strdup (netinfo_get_host (netinfo));
	g_strstrip (domain);

	if (! strcmp (domain, "")) {
		primary = g_strdup (_("A domain address was not specified"));
		secondary = g_strdup (_("Please enter a valid domain address and try again."));
	}
	g_free (domain);

	if (primary) {
		netinfo_error_message (netinfo, primary, secondary);
		g_free (primary);
		if (secondary)
			g_free (secondary);
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
	gint child_pid, pout; /* , perr; */
	GIOChannel *channel;
	const gchar *charset;
	GIOStatus status;
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
		fcntl (pout, F_SETFL, O_NONBLOCK);
		fcntl (pout, F_SETFL, O_NONBLOCK);
		netinfo->command_output = NULL;

		/*netinfo->pipe_err = perr; */

		g_get_charset(&charset);
		channel = g_io_channel_unix_new (pout);
		status = g_io_channel_set_encoding(channel,
						   charset,
						   &err);
		if (G_IO_STATUS_NORMAL == status) {
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
			g_warning ("Error: %s\n", err->message);
			g_error_free (err);
		}
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
	gchar *text = NULL;
	gsize len;
	Netinfo *netinfo = (Netinfo *) data;
	GError *err = NULL;
	const gchar *encoding;

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
		GIOStatus status;

		status = g_io_channel_read_line (channel, &text, &len, NULL, &err);

		if (status == G_IO_STATUS_NORMAL) {
			if (netinfo->command_output) {
				g_string_append (netinfo->command_output, text);
				g_free (text);
				text = g_string_free (netinfo->command_output, FALSE);
				netinfo->command_output = NULL;
			}

			if (netinfo->process_line != NULL) {
				(netinfo->process_line) ((gpointer) netinfo, text,
						 	len, NULL);
			}

		} else if (status == G_IO_STATUS_AGAIN) {
			char buf[1];

			/* A non-terminated line was read, read the data into the buffer. */
			status = g_io_channel_read_chars (channel, buf, 1, NULL, NULL);
			if (status == G_IO_STATUS_NORMAL) {
				if (netinfo->command_output == NULL) {
					netinfo->command_output = g_string_new (NULL);
				}
				g_string_append_c (netinfo->command_output, buf[0]);
			}
		} else if (status == G_IO_STATUS_EOF) {
			
		} else if (status == G_IO_STATUS_ERROR) {
			encoding = g_io_channel_get_encoding (channel);

			if (err->code == G_CONVERT_ERROR_ILLEGAL_SEQUENCE) {
				g_warning ("Warning: change of encoding: %s. The encoding "
						   "was changed from %s to ISO-8859-1 only "
						   "for this string\n", 
						   err->message,
						   encoding);
		
				g_io_channel_set_encoding (channel, "ISO-8859-1", NULL);
				g_io_channel_read_line (channel, &text, &len, NULL, NULL);

			} else {
				g_warning ("Error: %s\n", err->message);
				g_free (text);
				g_free (err);
			}

		}

		g_free (text);

		return TRUE;
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
	GdkCursor *cursor;
	PangoFontDescription *font_desc;

	g_assert (netinfo != NULL);
	g_return_if_fail (netinfo != NULL);

	if (! netinfo->toggle) {
		netinfo->running = !state;
		return;
	}

	if (GTK_IS_WIDGET (netinfo->sensitive)) {
		gtk_widget_set_sensitive (GTK_WIDGET (netinfo->sensitive),
					  state);
	}

	font_desc = pango_font_description_new ();

	if (state) {
		pango_font_description_set_weight (font_desc,
						   PANGO_WEIGHT_NORMAL);

		netinfo_progress_indicator_stop (netinfo);
		gdk_window_set_cursor (gtk_widget_get_window(netinfo->output), NULL);
		netinfo->child_pid = 0;
		
		gtk_statusbar_pop (GTK_STATUSBAR (netinfo->status_bar), 0);
		gtk_statusbar_push (GTK_STATUSBAR (netinfo->status_bar),
					    0, _("Idle"));
	} else {
		pango_font_description_set_weight (font_desc,
						   PANGO_WEIGHT_BOLD);

		netinfo_progress_indicator_start (netinfo);
		cursor = gdk_cursor_new (GDK_WATCH);
		if (!gtk_widget_get_realized (netinfo->output))
			gtk_widget_realize (GTK_WIDGET (netinfo->output));
		gdk_window_set_cursor (gtk_widget_get_window(netinfo->output), cursor);
		gdk_cursor_unref (cursor);

		if (netinfo->stbar_text) {
			gtk_statusbar_pop (GTK_STATUSBAR (netinfo->status_bar), 0);
			gtk_statusbar_push (GTK_STATUSBAR (netinfo->status_bar),
					    0, netinfo->stbar_text);
		}
	}

	gtk_widget_modify_font (netinfo->page_label, font_desc);
	pango_font_description_free (font_desc);

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
	gtk_widget_set_can_default (GTK_WIDGET (button), TRUE);

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


void
netinfo_progress_indicator_stop (Netinfo * netinfo)
{
	if (progress_timeout_id != 0) {
		g_source_remove (progress_timeout_id);
		progress_timeout_id = 0;
		if (netinfo)
			gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (netinfo->progress_bar), 0.0);
	}
}

static gboolean
update_progress_bar (gpointer data)
{
	gtk_progress_bar_pulse (GTK_PROGRESS_BAR (data));
	return TRUE;
}

void
netinfo_progress_indicator_start (Netinfo * netinfo)
{
	g_return_if_fail (netinfo != NULL);

	if (progress_timeout_id != 0)
		netinfo_progress_indicator_stop (netinfo);

	progress_timeout_id = g_timeout_add (100, update_progress_bar,
					     netinfo->progress_bar);
}
