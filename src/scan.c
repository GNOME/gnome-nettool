/* -*- Mode: C; indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8 -*- */

/* gnome-netinfo - A GUI Interface for network utilities
 * Copyright (C) 2003 by German Poo-Caaman~o
 * Copyright (C) 2003 by William Jon McCann
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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#include "scan.h"
#include "utils.h"

#define SIZE 1024

static GtkTreeModel *scan_create_model (GtkTreeView *widget);
static gint strip_line (gchar * line, scan_data * data);

void
scan_stop (Netinfo * netinfo)
{
	g_return_if_fail (netinfo != NULL);

	netinfo_stop_process_command (netinfo);
}

void
scan_do (Netinfo * netinfo)
{
	const gchar *host = NULL;
	GtkTreeModel *model;
	
	struct sockaddr_in addr;
	struct sockaddr_in6 addr6;
	struct hostent *hp = NULL;
	struct servent *service = NULL;
	gint i, sock, start_port = 1, end_port = 7000;
	GIOChannel *channel;
	GIOChannel *channel2;
	gint pfd[2];
	gint pid;
	gchar buf[SIZE];
	gchar *service_name = NULL;
	gint ip_version, pf;
	struct sockaddr *addr_ptr;
	gint size;

	g_return_if_fail (netinfo != NULL);

	/* Because of the delay, we can't check twice for a hostname/IP.
	 * It was made before this function was called.  Anyway, we
	 * check at least if we have a text as hostname */
	if (netinfo_validate_domain (netinfo) == FALSE) {
		netinfo_stop_process_command (netinfo);
		return;
	}

	host = netinfo_get_host (netinfo);

	if (netinfo->stbar_text)
		g_free (netinfo->stbar_text);
	netinfo->stbar_text = g_strdup_printf (_("Scanning %s for open ports"), host);

	/* Clear the current output */
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (netinfo->output));
	if (GTK_IS_LIST_STORE (model)) {
		gtk_list_store_clear (GTK_LIST_STORE (model));
	}
	
	switch (ip_version = netinfo_get_ip_version (netinfo))
	{
	case IPV4:
		pf = PF_INET;
		break;
	case IPV6:
		pf = PF_INET6;
		break;
	case -1:
	default:
#ifdef DEBUG
		g_print ("Error: Host unkown\n");
#endif /* DEBUG */
		return;
		/*g_return_if_fail (hp != NULL);*/
		break;
	}

	hp = gethostbyname2 (host, pf);

	if (pipe (pfd) == -1) {
		perror ("pipe failed");
		return;
	}

        netinfo_toggle_state (netinfo, INACTIVE, NULL);

	if ((pid = fork ()) < 0) {
		perror ("fork failed");
		return;
	}

	if (pid == 0) {
		/* child */
		close (pfd[0]);
		for (i = start_port; i <= end_port; i++) {
			if ((sock = socket (pf, SOCK_STREAM, 0)) == -1) {
#ifdef DEBUG			
				g_print ("Unable to create socket\n");
#endif /* DEBUG */
				continue;
			}

			channel = g_io_channel_unix_new (sock);

			if (ip_version == IPV4) {
				addr.sin_family = PF_INET;
				bcopy (hp->h_addr, &addr.sin_addr, hp->h_length);
				addr.sin_port = htons (i);
				addr_ptr = (struct sockaddr *) &addr;
				size = sizeof (addr);
			}
			else {
				addr6.sin6_family = PF_INET6;
				addr6.sin6_flowinfo = 0;
				bcopy (hp->h_addr, &addr6.sin6_addr, hp->h_length);
				addr6.sin6_port = htons (i);
				addr_ptr = (struct sockaddr *) &addr6;
				size = sizeof (addr6);
			}
			
			if (connect (sock, addr_ptr, size) == 0) {
				service = getservbyport (htons (i), "tcp");

				if (service != NULL) {
					service_name = g_strdup (service->s_name);
				} else {
					service_name = g_strdup (_("unknown"));
				}

				sprintf (buf, "%d open %s\n", i, service_name);
				g_free (service_name);
				write (pfd[1], buf, strlen (buf));
			}
			/* close (sock); */
			g_io_channel_shutdown (channel, FALSE, NULL);
		}
		close (pfd[1]);
		exit (0);
	} else {
		/* parent */
		close (pfd[1]);
		
		netinfo->child_pid = pid;
		netinfo->pipe_out = pfd[0];

		channel2 = g_io_channel_unix_new (pfd[0]);
		g_io_add_watch (channel2,
				G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL,
				netinfo_io_text_buffer_dialog, netinfo);
		g_io_channel_unref (channel2);
	}
	
}

/* Process each line from ping command */
void
scan_foreach (Netinfo * netinfo, gchar * line, gint len,
	      gpointer user_data)
{
	scan_data data;
	gint count;

	g_return_if_fail (netinfo != NULL);
	g_return_if_fail (line != NULL);

	count = strip_line (line, &data);
	if (count == 3) {
		scan_define_model (netinfo, data);
	}
}

static gint
strip_line (gchar * line, scan_data * data)
{
	gint count;

	count = sscanf (line, SCAN_FORMAT,
			&(data)->port, data->state, data->service);

	return count;
}

void
scan_define_model (Netinfo * netinfo, scan_data data)
{
	GtkTreeIter iter, sibling;
	GList *columns;
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeView *widget;

	g_return_if_fail (netinfo != NULL);

	widget = (GTK_TREE_VIEW (netinfo->output));

	/* Creation of GtkTreeView */
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW
					  (widget), TRUE);
	columns =
		gtk_tree_view_get_columns (GTK_TREE_VIEW
					   (widget));

	if (g_list_length (columns) == 0) {

		model = scan_create_model (widget);
		gtk_tree_view_set_model (GTK_TREE_VIEW
					 (widget), model);
	}
	g_list_free (columns);

	model =
		gtk_tree_view_get_model (GTK_TREE_VIEW
					 (widget));
	
	gtk_tree_view_get_cursor (GTK_TREE_VIEW (widget),
				  &path, NULL);

	if (path != NULL) {
		gtk_tree_model_get_iter (model, &sibling,
					 path);
		gtk_list_store_insert_after (GTK_LIST_STORE
						 (model),
						 &iter,
						 &sibling);
	} else {
		gtk_list_store_append (GTK_LIST_STORE
					   (model), &iter);
	}

	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				0, data.port,
				1, data.state,
				2, data.service, -1);

	gtk_tree_view_set_model (GTK_TREE_VIEW (widget),
				 model);
	path = gtk_tree_model_get_path (model, &iter);
	gtk_tree_view_set_cursor (GTK_TREE_VIEW (widget),
				  path, NULL, FALSE);
	gtk_tree_path_free (path);
}



static GtkTreeModel *
scan_create_model (GtkTreeView *widget)
{
	GtkCellRenderer *renderer;
	static GtkTreeViewColumn *column;
	GtkTreeModel *model;

	renderer = gtk_cell_renderer_text_new ();
	/* Number of bytes received in a ping reply */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Port"), renderer, "text", 0, NULL);
	g_object_set (G_OBJECT (renderer), "xalign", 1.0);
	gtk_tree_view_append_column (widget, column);

	renderer = gtk_cell_renderer_text_new ();
	/* IP address that reply a ping */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("State"), renderer, "text", 1, NULL);
	gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_append_column (widget, column);

	renderer = gtk_cell_renderer_text_new ();
	/* Number of sequence of a ICMP request (ping) */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Service"), renderer, "text", 2, NULL);
	gtk_tree_view_append_column (widget, column);

	model = GTK_TREE_MODEL (gtk_list_store_new
				(3,
				 G_TYPE_INT,
				 G_TYPE_STRING,
				 G_TYPE_STRING));
	return model;
}

/* Copy on clipboard */
void
scan_copy_to_clipboard (Netinfo * netinfo, gpointer user_data)
{
	GString *result, *content;

	g_return_if_fail (netinfo != NULL);

	/* The portscan output in text format:
	   Port, State, Service.
	   It's a tabular output, and these belongs to the column titles */
	result = g_string_new (_("Port\tState\tService\n"));
	
	content = util_tree_model_to_string (GTK_TREE_VIEW (netinfo->output));
	
	g_string_append_printf (result, "%s", content->str);
	
	gtk_clipboard_set_text (gtk_clipboard_get (GDK_NONE), result->str,
				result->len);

	g_string_free (content, TRUE);
	g_string_free (result, TRUE);
}
