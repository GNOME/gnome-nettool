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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>

#include "traceroute.h"
#include "utils.h"

static gint strip_line (gchar * line, traceroute_data * data);
static GtkTreeModel *traceroute_create_model (GtkTreeView *widget);

void
traceroute_stop (Netinfo * netinfo)
{
	g_return_if_fail (netinfo != NULL);

	netinfo_stop_process_command (netinfo);
}

void
traceroute_do (Netinfo * netinfo)
{
	const gchar *host = NULL;
	gchar *command = NULL;
	gchar *program = NULL;
	gchar *program_name = NULL;
	GtkTreeModel *model;
	GtkWidget *parent;
        
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
	netinfo->stbar_text = g_strdup_printf (_("Tracing route to %s"), host);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (netinfo->output));
	if (GTK_IS_LIST_STORE (model)) {
		gtk_list_store_clear (GTK_LIST_STORE (model));
	}

	parent = gtk_widget_get_toplevel (netinfo->output);
	
	switch (netinfo_get_ip_version (netinfo))
	{
	case IPV4:
		program = util_find_program_in_path ("tcptraceroute", NULL);
#ifdef DEBUG
		g_print ("tcptraceroute: %s\n", program);
#endif /* DEBUG */
		if (program != NULL) {
			program_name = g_strdup ("tcptraceroute");
		} else {
			program = util_find_program_dialog ("traceroute", parent);
			program_name = g_strdup ("traceroute");
		}
		break;
	case IPV6:
		program = util_find_program_in_path ("traceroute6", NULL);
		program_name = g_strdup ("traceroute6");
		break;
	default:
		program = NULL;
		break;
	}

	if (program != NULL) {
		command =
                  g_strdup_printf ("%s %s %s %s", program, program_name,
                                   TCPTRACEROUTE_OPTIONS, host);
	
		netinfo->command_line = g_strsplit (command, " ", -1);
	
		netinfo_process_command (netinfo);

		g_strfreev (netinfo->command_line);
		g_free (command);
		g_free (program);
		g_free (program_name);
	}
}

/* Process each line from ping command */
void
traceroute_foreach (Netinfo * netinfo, gchar * line, gssize len,
		    gpointer user_data)
{
	gchar *text_utf8;
	gsize bytes_written;
	GtkTextBuffer *buffer = NULL;
	GtkTextIter iter;

	g_return_if_fail (netinfo != NULL);
	g_return_if_fail (line != NULL);

	buffer =
	    gtk_text_view_get_buffer (GTK_TEXT_VIEW (netinfo->output));
	gtk_text_buffer_get_end_iter (buffer, &iter);

	if (len > 0) {
		text_utf8 = g_locale_to_utf8 (line, len,
					      NULL, &bytes_written, NULL);

		gtk_text_buffer_insert
		    (GTK_TEXT_BUFFER (buffer), &iter, text_utf8,
		     bytes_written);
		g_free (text_utf8);
	}
}

void
traceroute_foreach_with_tree (Netinfo * netinfo, gchar * line, gint len,
			      gpointer user_data)
{
	GtkTreeIter iter;
	GList *columns;
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeView *widget;
	gint count;
	traceroute_data data;

	g_return_if_fail (netinfo != NULL);
	g_return_if_fail (line != NULL);

	widget = (GTK_TREE_VIEW (netinfo->output));

	if (len > 0) {		/* there are data to show */
		/* count = traceroute_strip_line (line, &data); */

		count = strip_line (line, &data);

		if (count == TRACE_NUM_ARGS) {

			gtk_tree_view_set_rules_hint (GTK_TREE_VIEW
						      (widget), TRUE);
			columns =
			    gtk_tree_view_get_columns (GTK_TREE_VIEW
						       (widget));

			if (g_list_length (columns) == 0) {

				model = traceroute_create_model (widget);
				gtk_tree_view_set_model (GTK_TREE_VIEW
							 (widget), model);
			}

			g_list_free (columns);

			model =
			    gtk_tree_view_get_model (GTK_TREE_VIEW
						     (widget));
			
			gtk_tree_view_get_cursor (GTK_TREE_VIEW (widget),
						  &path, NULL);

			gtk_list_store_append (GTK_LIST_STORE
						  (model), &iter);

			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
					    TRACE_HOP, data.hop_count,
					    TRACE_HOSTNAME, data.hostname,
					    TRACE_IP, data.ip,
					    TRACE_RTT1, data.rtt1,
					    TRACE_RTT2, data.rtt2,
/*					    TRACE_RTT3, data.rtt3,*/ -1);

			gtk_tree_view_set_model (GTK_TREE_VIEW (widget),
						 model);

			if (path) {
				gtk_tree_view_set_cursor (
					GTK_TREE_VIEW (widget),
					path, NULL, FALSE);
				gtk_tree_path_free (path);
			}
		}
	}
	while (gtk_events_pending ()) {
		  gtk_main_iteration_do (FALSE);
	}
}

static gint
strip_line (gchar * line, traceroute_data * data)
{
	gint count;

	line = g_strdelimit (line, "()", ' ');
	
	count = sscanf (line, TRACE_FORMAT,
			&(data)->hop_count, data->hostname, data->ip,
			data->rtt1, data->rtt2/*, data->rtt3*/);
	
	if (count == TRACE_NUM_ARGS) {
		return count;
	}

	if (count == TRACE_NUM_ERR) {
		g_sprintf (data->rtt1, "*");
		g_sprintf (data->rtt2, "*");
		return TRACE_NUM_ARGS;
	}
	/* The last line is different, because it has a 
	   extra [open] or [close] string. That string it 
	   does not matter to us 
	 */

	count = sscanf (line, TRACE_FORMAT_OPEN,
			&(data)->hop_count, data->hostname, data->ip,
			data->rtt1, data->rtt2/*, data->rtt3*/);

	if (count == TRACE_NUM_ARGS) {
		return count;
	} else {
		count = sscanf (line, TRACE_FORMAT_CLOSE,
				&(data)->hop_count, data->hostname,
				data->ip, data->rtt1, data->rtt2 /*,
				data->rtt3*/);
	}

	return count;
}

static GtkTreeModel *
traceroute_create_model (GtkTreeView *widget)
{
	GtkCellRenderer *renderer = NULL;
	static GtkTreeViewColumn *column;
	GtkTreeModel *model;

	/*FIXME: Set correctly the align for each renderer */
	
	renderer = gtk_cell_renderer_text_new ();
	/* Number of sequence of each hop in a traceroute output */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Hop"), renderer, "text", TRACE_HOP, NULL);
	g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
	gtk_tree_view_append_column (widget, column);

	renderer = gtk_cell_renderer_text_new ();
	/* Hostname of target we are tracing the route */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Hostname"), renderer, "text", TRACE_HOSTNAME, NULL);
	gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_append_column (widget, column);

	renderer = gtk_cell_renderer_text_new ();
	/* IP address of the hostname we are tracing the route */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("IP"), renderer, "text", TRACE_IP, NULL);
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
	gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_append_column (widget, column);


	renderer = gtk_cell_renderer_text_new ();
	/* Time elapsed between a packets was sent and
	   when was received its reply (1st sample) */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Time 1"), renderer, "text", TRACE_RTT1, NULL);
	g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
	gtk_tree_view_append_column (widget, column);


	renderer = gtk_cell_renderer_text_new ();
	/* Time elapsed between a packets was sent and
	   when was received its reply (2nd sample) */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Time 2"), renderer, "text", TRACE_RTT2, NULL);
	g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
	gtk_tree_view_append_column (widget, column);

	model = GTK_TREE_MODEL (gtk_list_store_new
				(TRACE_NUM_COLUMNS,
				 G_TYPE_INT,
				 G_TYPE_STRING,
				 G_TYPE_STRING,
				 G_TYPE_STRING,
				 G_TYPE_STRING));

	return model;
}

void
traceroute_copy_to_clipboard (Netinfo * netinfo, gpointer user_data)
{
	GString *result, *content;

	g_return_if_fail (netinfo != NULL);

	/* The traceroute output in text format:
	   Hop count, Hostname, IP, Round Trip Time 1 (Time1),
	   Round Trip Time 2 (Time2),
	   It's a tabular output, and these belongs to the column titles */
	result = g_string_new (_("Hop\tHostname\tIP\tTime 1\tTime 2\n"));

	content = util_tree_model_to_string (GTK_TREE_VIEW (netinfo->output));
	
	g_string_append_printf (result, "%s", content->str);
	
	gtk_clipboard_set_text (gtk_clipboard_get (GDK_NONE), result->str,
				result->len);

	g_string_free (content, TRUE);
	g_string_free (result, TRUE);
}
