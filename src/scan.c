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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>

#include "scan.h"
#include "utils.h"

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
	gint ip_version;
	gchar *program = NULL;
	gchar *command = NULL;
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
	netinfo->stbar_text = g_strdup_printf (_("Scanning %s for open ports"), host);

	/* Clear the current output */
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (netinfo->output));
	if (GTK_IS_LIST_STORE (model)) {
		gtk_list_store_clear (GTK_LIST_STORE (model));
	}

	parent = gtk_widget_get_toplevel (netinfo->output);
	program = util_find_program_dialog ("nmap", parent);

	if (program != NULL) {
		ip_version = netinfo_get_ip_version (netinfo);

		if (ip_version == IPV6) {
			command = g_strdup_printf ("%s %s %s %s", program,
						   "nmap", "-6", host);
		} else {
			command = g_strdup_printf ("%s %s %s", program,
						   "nmap", host);
		}
	
		g_strfreev (netinfo->command_line);
		netinfo->command_line = g_strsplit (command, " ", -1);
	
		netinfo_process_command (netinfo);
	}

	g_free (command);
	g_free (program);
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

	if (! g_regex_match_simple ("^\\d+/\\w", line, 0, 0)) {
		return 0;
	}

	count = sscanf (line, SCAN_FORMAT,
			data->port, data->state, data->service);

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
	g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
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
				 G_TYPE_STRING,
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
