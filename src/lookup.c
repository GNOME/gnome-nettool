/* -*- mode:C; indent-tabs-mode:t; tab-width:8; c-basic-offset:8; -*- */
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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>

#include "lookup.h"
#include "utils.h"
#include <regex.h>

static gint strip_line (gchar * line, lookup_data * data);
static GtkTreeModel * lookup_create_model (GtkTreeView *widget);

void
lookup_stop (Netinfo * netinfo)
{
	g_return_if_fail (netinfo != NULL);

	netinfo_stop_process_command (netinfo);
}

static int
pattern_match (const char *string, const char *pattern)
{
	int status;
	regex_t re;
	if (regcomp (&re, pattern, REG_EXTENDED | REG_NOSUB) != 0) {
                return FALSE;      /* report error */
	}
	status = regexec (&re, string, (size_t) 0, NULL, 0);
	regfree (&re);
	
	if (status != 0) {
		return FALSE;      /* report error */
	}
	return TRUE;
}
 

void
lookup_do (Netinfo * netinfo)
{
	const gchar *host = NULL;
	gchar *command = NULL;
	gchar *program = NULL;
	GtkTreeModel *model;
	GtkWidget *parent;
	gboolean use_reverse_lookup;
	const gchar *address_regular_expression = "^[ ]*[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}[ ]*$";
	gint active_index;
	const gchar *query_types[12] = {"", "A", "CNAME", "HINFO", "MX", "MINFO", "NS", "PTR", "SOA", "TXT", "WKS", "ANY"};
	gchar **command_line;
	gchar **command_options;
	gint i, j, num_terms;

	g_return_if_fail (netinfo != NULL);

	host = netinfo_get_host (netinfo);

	if (netinfo->stbar_text)
		g_free (netinfo->stbar_text);
	netinfo->stbar_text = g_strdup_printf (_("Looking up %s"), host);

	if (netinfo_validate_domain (netinfo) == FALSE) {
		netinfo_stop_process_command (netinfo);
		return;
	}

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (netinfo->output));
	if (GTK_IS_LIST_STORE (model)) {
		gtk_list_store_clear (GTK_LIST_STORE (model));
	}

	/*option_menu = netinfo->type;
	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (option_menu));
	active_item = gtk_menu_get_active (GTK_MENU (menu));
	active_index = g_list_index (GTK_MENU_SHELL (menu)->children, active_item);*/
	active_index = gtk_combo_box_get_active (GTK_COMBO_BOX (netinfo->type));

	parent = gtk_widget_get_toplevel (netinfo->output);
	
	program = util_find_program_dialog ("dig", parent);

	if (program != NULL) {
		use_reverse_lookup = pattern_match (host, address_regular_expression);

		num_terms = 4;
		if (use_reverse_lookup)
			num_terms++;
		if (active_index > 0)
			num_terms++;
		command_options = g_strsplit (LOOKUP_OPTIONS, " ", -1);
		if (command_options != NULL) {
			for (j = 0; command_options[j] != NULL; j++)
				num_terms++;
		}
		command_line = g_new (gchar *, num_terms + 1);
		i = 0;
		command_line[i++] = g_strdup (program);
		command_line[i++] = g_strdup ("dig");
		if (command_options != NULL) {
			for (j = 0; command_options[j] != NULL; j++)
				command_line[i++] = g_strdup (command_options[j]);
		}
		if (use_reverse_lookup)
			command_line[i++] = g_strdup ("-x");
		command_line[i++] = g_strdup (host);
		if (active_index > 0)
			command_line[i++] = g_strdup (query_types[active_index]);
		command_line[i++] = NULL;

		netinfo->command_line = command_line;

		netinfo_process_command (netinfo);

		g_strfreev (command_options);
		g_strfreev (netinfo->command_line);
	}

	g_free (command);
	g_free (program);
}

/* Process each line from lookup command */
void
lookup_foreach (Netinfo * netinfo, gchar * line, gssize len,
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
lookup_foreach_with_tree (Netinfo * netinfo, gchar * line, gint len,
			  gpointer user_data)
{
	GtkTreeIter iter, sibling;
	GList *columns;
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeView *widget;
	gint count;
	lookup_data data;

	g_return_if_fail (netinfo != NULL);
	g_return_if_fail (line != NULL);

	widget = (GTK_TREE_VIEW (netinfo->output));

	if (len > 0) {		/* there are data to show */
		count = strip_line (line, &data);

		if (count == LOOKUP_NUM_ARGS) {

			gtk_tree_view_set_rules_hint (GTK_TREE_VIEW
						      (widget), TRUE);
			columns =
			    gtk_tree_view_get_columns (GTK_TREE_VIEW
						       (widget));

			if (g_list_length (columns) == 0) {

				model = lookup_create_model (widget);
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
					    LOOKUP_SOURCE, data.source,
					    LOOKUP_TTL, data.ttl,
					    LOOKUP_ADDR_TYPE, data.addr_type,
					    LOOKUP_RECORD_TYPE, data.record_type,
					    LOOKUP_DESTINATION, data.destination, 
						-1);

			gtk_tree_view_set_model (GTK_TREE_VIEW (widget),
						 model);
			path = gtk_tree_model_get_path (model, &iter);
			gtk_tree_view_set_cursor (GTK_TREE_VIEW (widget),
						  path, NULL, FALSE);
			gtk_tree_path_free (path);
		}
	} 
}

static gint
strip_line (gchar * line, lookup_data * data)
{
	gint count;
	gchar priority[128], host_mx[128];

	line = g_strdelimit (line, "\t\t", '\t');
	
	count = sscanf (line, LOOKUP_FORMAT_MX, data->source, &(data)->ttl, 
			        data->addr_type, priority, host_mx);
	
	if (count == LOOKUP_NUM_ARGS) {
		g_sprintf (data->record_type, "MX");
		g_sprintf (data->destination, "%s (%s)", host_mx, priority);
	} else {
		count = sscanf (line, LOOKUP_FORMAT,
				data->source, &(data)->ttl, data->addr_type,
				data->record_type, data->destination);
	}

	return count;
}

static GtkTreeModel *
lookup_create_model (GtkTreeView *widget)
{
	GtkCellRenderer *renderer = NULL;
	static GtkTreeViewColumn *column;
	GtkTreeModel *model;

	renderer = gtk_cell_renderer_text_new ();
	/* Hostname */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Name"), renderer, "text", LOOKUP_SOURCE, NULL);

	gtk_tree_view_append_column (widget, column);

	renderer = gtk_cell_renderer_text_new ();
	/* Time To Live of a hostname in a name server */
	column =
	    gtk_tree_view_column_new_with_attributes
		/* Time To Live of a hostname in a name server */
	    (_("TTL"), renderer, "text", LOOKUP_TTL, NULL);
	gtk_tree_view_column_set_alignment (column, 0.5);
	g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
	gtk_tree_view_append_column (widget, column);

	renderer = gtk_cell_renderer_text_new ();
	/* Type of address in the resolution (name server) */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Address Type"), renderer, "text", LOOKUP_ADDR_TYPE, NULL);
	g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
	gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_append_column (widget, column);


	renderer = gtk_cell_renderer_text_new ();
	/* Type of record (A, HINFO, PTR, etc.) */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Record Type"), renderer, "text", LOOKUP_RECORD_TYPE, NULL);

	gtk_tree_view_append_column (widget, column);


	renderer = gtk_cell_renderer_text_new ();
	/* Address/Name associated */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Address"), renderer, "text", LOOKUP_DESTINATION, NULL);

	gtk_tree_view_append_column (widget, column);

	model = GTK_TREE_MODEL (gtk_list_store_new
				(LOOKUP_NUM_COLUMNS,
				 G_TYPE_STRING,
				 G_TYPE_INT,
				 G_TYPE_STRING,
				 G_TYPE_STRING,
				 G_TYPE_STRING));

	return model;
}

void
lookup_copy_to_clipboard (Netinfo * netinfo, gpointer user_data)
{
	GString *result, *content;

	g_return_if_fail (netinfo != NULL);

	/* The lookup output in text format:
	   Source of query (hostname/ip address), 
	   Time To Live (TTL), Address Type,
	   Record Type (A, PTR, HINFO, NS, TXT, etc.), 
	   Resolution (results of the query)
	   It's a tabular output, and these belongs to the column titles */
	result = g_string_new (_("Source\tTTL\tAddress Type\tRecord Type1\tResolution\n"));

	content = util_tree_model_to_string (GTK_TREE_VIEW (netinfo->output));
	
	g_string_append_printf (result, "%s", content->str);
	
	gtk_clipboard_set_text (gtk_clipboard_get (GDK_NONE), result->str,
				result->len);

	g_string_free (content, TRUE);
	g_string_free (result, TRUE);
}
