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
 
#include <gnome.h>
#include "netstat.h"
#include "utils.h"

static gint strip_protocol_line (gchar * line, netstat_protocol_data *data);
void netstat_create_protocol_model (GtkTreeView *widget);
void netstat_protocol_tree_insert (GtkTreeView *widget, gchar *line);

static gint strip_route_line (gchar * line, netstat_route_data *data);
void netstat_route_tree_insert (GtkTreeView *widget, gchar *line);
void netstat_create_route_model (GtkTreeView *widget);

void netstat_multicast_tree_insert (GtkTreeView *widget, gchar *line);
static gint strip_multicast_line (gchar * line, netstat_multicast_data *data);
void netstat_create_multicast_model (GtkTreeView *widget);

static NetstatOption netstat_get_active_option2 (Netinfo * netinfo);
static gchar * netstat_get_active_option (Netinfo * netinfo);

static GtkTreeModel *protocol_model, *route_model, *multicast_model;

void clean_gtk_tree_view (GtkTreeView *widget);

/* Check the ToggleButton active to show the GtkTreeView with
 * the right GtkTreeModel
 */
void
on_protocol_button_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
	GtkTreeView *widget;
	Netinfo *netinfo = user_data;
	/* GtkWidget *parent, *child; */
	
	g_return_if_fail (GTK_IS_TREE_VIEW (netinfo->output));
	widget = GTK_TREE_VIEW (netinfo->output);
	
	/*
	parent = gtk_widget_get_parent (GTK_WIDGET (widget));
	g_print ("name: %s\n", gtk_widget_get_name (parent));
	child = gtk_bin_get_child (GTK_BIN (parent));
	
	gtk_widget_ref (child);
	
	gtk_container_remove (GTK_CONTAINER (parent), child);
	*/
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (netinfo->routing))) {
		g_print ("routing set\n");
		/*gtk_container_add (GTK_CONTAINER (parent), route_tree);
		gtk_tree_view_set_model (widget, route_model); */
	}
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (netinfo->protocol))) {
		g_print ("protocol\n");
		/* gtk_container_add (GTK_CONTAINER (parent), protocol_tree);
		gtk_tree_view_set_model (widget, protocol_model); */
	}
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (netinfo->multicast))) {
		g_print ("multicast\n");
		/* gtk_container_add (GTK_CONTAINER (parent), multicast_tree);
		gtk_tree_view_set_model (widget, multicast_model); */
	}
	
	/* gtk_widget_unref (child); */
}

static NetstatOption
netstat_get_active_option2 (Netinfo * netinfo)
{
	NetstatOption option = NONE;
	
	g_return_val_if_fail (netinfo != NULL, ROUTE);
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (netinfo->routing))) {
		option = ROUTE;
	}
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (netinfo->protocol))) {
		option = PROTOCOL;
	}
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (netinfo->multicast))) {
		option = MULTICAST;
	}
	return option;
}

static gchar *
netstat_get_active_option (Netinfo * netinfo)
{
	gchar *option = NULL;
	
	g_return_val_if_fail (netinfo != NULL, NULL);
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (netinfo->routing))) {
		/* Works for Solaris and Linux */
		if (netinfo_is_ipv6_enable ()) {
			option = g_strdup ("-rn -A inet -A inet6");
		} else {
			option = g_strdup ("-rn -A inet");
		}

		if (netinfo->stbar_text)
			g_free (netinfo->stbar_text);
		netinfo->stbar_text = g_strdup (_("Getting routing table"));
	}
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (netinfo->protocol))) {
		/* Only works for Solaris */
#ifdef __FreeBSD__
	    	option = g_strdup ("-a -f inet -ln");
#else
		if (netinfo_is_ipv6_enable ()) {
			option = g_strdup ("-A inet -A inet6 -ln");
		} else {
			option = g_strdup ("-A inet -ln");
		}

		if (netinfo->stbar_text)
			g_free (netinfo->stbar_text);
		netinfo->stbar_text = g_strdup (_("Getting active Internet connections"));
#endif
	}
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (netinfo->multicast))) {
		/* It works for Solaris and Linux */
		option = g_strdup ("-g");

		if (netinfo->stbar_text)
			g_free (netinfo->stbar_text);
		netinfo->stbar_text = g_strdup (_("Getting group memberships"));
	}
	return option;
}

void
netstat_stop (Netinfo * netinfo)
{
	g_return_if_fail (netinfo != NULL);
	
	netinfo_stop_process_command (netinfo);
}

void
netstat_do (Netinfo * netinfo)
{
	gchar *command = NULL;
	gchar *option = NULL;
	gchar *program = NULL;
	GtkTreeModel *model;
	GtkWidget *parent;

	g_return_if_fail (netinfo != NULL);

	option = netstat_get_active_option (netinfo);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (netinfo->output));
	if (GTK_IS_LIST_STORE (model)) {
		gtk_list_store_clear (GTK_LIST_STORE (model));
	}

	parent = gtk_widget_get_toplevel (netinfo->output);
	
	program = util_find_program_dialog ("netstat", parent);

	if (program != NULL) {
		command =
			g_strdup_printf ("%s netstat %s", program, option);
	
		netinfo->command_line = g_strsplit (command, " ", -1);
	
		netinfo_process_command (netinfo);
	
		g_strfreev (netinfo->command_line);
	}
	
	g_free (command);
	g_free (option);
	g_free (program);
}

/* Process each line from netstat command */
void 
netstat_foreach (Netinfo * netinfo, gchar * line, gint len, gpointer user_data)
{
	gchar *text_utf8;
	gssize bytes_written;
	GtkTextBuffer *buffer = NULL;
	GtkTextIter iter;
	
	g_return_if_fail (netinfo != NULL);
	/*g_return_if_fail (line != NULL);*/
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (netinfo->output));
	gtk_text_buffer_get_end_iter (buffer, &iter);

	if (netstat_get_active_option2 (netinfo) == PROTOCOL) {
		netstat_protocol_data data;
		gint count;
		
		count = strip_protocol_line (line, &data);
#ifdef DEBUG
		if (count == 7 || count == 8) {

			g_print ("%s\t%s:%s\t%s\n", data.protocol,
				data.ip_src, data.port_src, data.state);
		}
#endif /* DEBUG */
	}

	if (len > 0) {
		text_utf8 = g_locale_to_utf8 (line, len,
						  NULL, &bytes_written,
						  NULL);

		gtk_text_buffer_insert
			(GTK_TEXT_BUFFER (buffer), &iter, text_utf8,
			 bytes_written);
		g_free (text_utf8);
	}
}

/* Collect data from netstat command and insert them in a GtkTreeView.

   Basically it consist on receive each line, divide it in fields
   and insert each value in a tree if belong.  The function strip_line
   divide the lines and return the number of fields.  To distinguish
   header and data from the output is useful to know then right number
   of fields.

   It could be avoid with a helper with two options: with/without 
   headers.  To execute on a shell or just to process its output as
   this program.
*/
void
netstat_foreach_with_tree (Netinfo * netinfo, gchar * line, gint len,
			   gpointer user_data)
{
	GtkTreeView *widget;

	g_return_if_fail (netinfo != NULL);
	//g_return_if_fail (line != NULL);

	widget = (GTK_TREE_VIEW (netinfo->output));

	if (len > 0) {		/* there are data to show */
		g_return_if_fail (line != NULL);
		switch (netstat_get_active_option2 (netinfo)) {
		case PROTOCOL:
			netstat_protocol_tree_insert (widget, line);
			break;
		case ROUTE:
			netstat_route_tree_insert (widget, line);
			break;
		case MULTICAST:
			netstat_multicast_tree_insert (widget, line);
			break;
		case NONE:
		default:
			break;
		}
	}
}

/* PROTOCOL */
void
netstat_protocol_tree_insert (GtkTreeView *widget, gchar *line)
{
	GtkTreeIter iter, sibling;
	/*GList *columns;*/
	GtkTreePath *path;
	GtkTreeModel *model;
	gint count;
	netstat_protocol_data data;

	g_return_if_fail (GTK_IS_TREE_VIEW (widget));
	g_return_if_fail (line != NULL);

	count = strip_protocol_line (line, &data);
#ifdef __FreeBSD__
	if (count == 5 || count == 6 || count == 9 || count == 10) {
#else
	if (count == 5 || count == 6) {
#endif
#ifdef DEBUG
		g_print ("%s\t%s:%s\t%s\n", data.protocol,
			data.ip_src, data.port_src, data.state);
#endif /* DEBUG */
		
		/* Creation of GtkTreeView */
		gtk_tree_view_set_rules_hint (widget, TRUE);
/*		columns = gtk_tree_view_get_columns (widget);

		if (g_list_length (columns) == 0) {
			model = netstat_create_protocol_model (widget);
			gtk_tree_view_set_model (widget, model);
		}
		g_list_free (columns);*/
		
		model = gtk_tree_view_get_model (widget);
		
		if (protocol_model == NULL || protocol_model != model) {
			clean_gtk_tree_view (widget);

			protocol_model = GTK_TREE_MODEL (gtk_list_store_new
						(4,
						 G_TYPE_STRING,
						 G_TYPE_STRING,
						 G_TYPE_STRING,
						 G_TYPE_STRING));
			netstat_create_protocol_model (widget);
			
			gtk_tree_view_set_model (widget, protocol_model);
		}

		model = gtk_tree_view_get_model (widget);
		
		gtk_tree_view_get_cursor (widget, &path, NULL);

		if (path != NULL) {
			gtk_tree_model_get_iter (model, &sibling, path);
			gtk_list_store_insert_after (GTK_LIST_STORE
							 (model),
							 &iter,
							 &sibling);
		} else {
			gtk_list_store_append (GTK_LIST_STORE
						   (model), &iter);
		}

		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
					0, data.protocol,
					1, data.ip_src,
					2, data.port_src,
					3, data.state, -1);

		gtk_tree_view_set_model (widget, model);
		path = gtk_tree_model_get_path (model, &iter);
		gtk_tree_view_set_cursor (widget, path, NULL, FALSE);
		gtk_tree_path_free (path);
	}
}

static gint
strip_protocol_line (gchar * line, netstat_protocol_data *data)
{
	gint count = 0;
#ifdef __FreeBSD__
	gint a1, a2, a3, a4;
	gchar s9[30];
#else
	gchar s6[30], laddr[50];
	gchar *port;
#endif
	gint n2, n3;

	/*line = g_strdelimit (line, ":", ' ');*/

#ifdef __FreeBSD__
	line = g_strdelimit (line, ":", ' ');
	
	count = sscanf (line, NETSTAT_PROTOCOL_FORMAT,
			data->protocol, &n2, &n3,
			&a1, &a2, &a3, &a4, data->port_src,
			s9, data->state);
	g_snprintf (data->ip_src, 30, "%d.%d.%d.%d", a1, a2, a3, a4);

	if (count == 9) {
	    bzero (&(data)->state, 30);
	}

	if (count == 3) {
	    /* Handle the *.* entries. */
	    gchar s5[30];
	    count = sscanf (line, ALT_NETSTAT_PROTOCOL_FORMAT,
		    	    data->protocol, &n2, &n3,
			    data->port_src, s5,
			    data->state);
	    g_snprintf (data->ip_src, 30, "*");

	    if (count == 5) {
		bzero (&(data)->state, 30);
	    }
	}

#else
	/*count = sscanf (line, NETSTAT_PROTOCOL_FORMAT,
			data->protocol, &n2, &n3,
			data->ip_src, data->port_src, 
			s6, s7, data->state);
	
	if (count == 7) {
		bzero (&(data)->state, 30);
		}*/
	count = sscanf (line, NETSTAT_PROTOCOL_FORMAT,
			data->protocol, &n2, &n3,
			laddr, s6, data->state);

	port = g_strrstr (laddr, ":");

	if (port != NULL) {
		g_strlcpy (data->ip_src, laddr, 50 * sizeof (gchar));
		data->ip_src[strlen (laddr) - strlen (port)] = '\0';
		port ++;
		g_strlcpy (data->port_src, port, 30 * sizeof (gchar));
	}

	if (count == 5) {
		bzero (&(data)->state, 30);
	}
#endif
	
	return count;
}

void
netstat_create_protocol_model (GtkTreeView *widget)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	/*clean_gtk_tree_view (widget);*/
	
	renderer = gtk_cell_renderer_text_new ();
	/* Transport Protocol that runs over */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Protocol"), renderer, "text", 0, NULL);
	gtk_tree_view_append_column (widget, column);

	renderer = gtk_cell_renderer_text_new ();
	/* IP address where to accept connections */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("IP Source"), renderer, "text", 1, NULL);
	gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_append_column (widget, column);

	renderer = gtk_cell_renderer_text_new ();
	/* Number of port where the service is listening */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Port/Service"), renderer, "text", 2, NULL);
	g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
	gtk_tree_view_append_column (widget, column);

	renderer = gtk_cell_renderer_text_new ();
	/* State of the service (commonly LISTEN) */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("State"), renderer, "text", 3, NULL);
	gtk_tree_view_append_column (widget, column);

	/* return model;*/
}
/* END PROTOCOL */

/* ROUTE */
void
netstat_route_tree_insert (GtkTreeView *widget, gchar *line)
{
	GtkTreeIter iter, sibling;
	/*GList *columns;*/
	GtkTreePath *path;
	GtkTreeModel *model;
	gint count;
	netstat_route_data data;

	g_return_if_fail (GTK_IS_TREE_VIEW (widget));
	g_return_if_fail (line != NULL);

	count = strip_route_line (line, &data);
#ifdef __FreeBSD__
	if (count == 6) {
#else
	if ((count == 8) || (count == 7)) {
#endif
#ifdef DEBUG
		g_print ("%s\t%s:%s\t%d\t%s\n", data.destination,
			data.gateway, data.netmask, data.metric,
			data.iface);
#endif /* DEBUG */

		/* Creation of GtkTreeView */
		gtk_tree_view_set_rules_hint (widget, TRUE);
/*		columns = gtk_tree_view_get_columns (widget);

		if (g_list_length (columns) == 0) {
			model = netstat_create_route_model (widget);
			gtk_tree_view_set_model (widget, model);
		}
		g_list_free (columns);*/
		
		model = gtk_tree_view_get_model (widget);
		
		if (route_model == NULL || route_model != model) {
			clean_gtk_tree_view (widget);
			
			route_model = GTK_TREE_MODEL (gtk_list_store_new
				(4,
				 G_TYPE_STRING,
				 G_TYPE_STRING,
				 G_TYPE_STRING,
				 G_TYPE_STRING));
			netstat_create_route_model (widget);
			
			gtk_tree_view_set_model (widget, route_model);
		}

		model = gtk_tree_view_get_model (widget);
		
		gtk_tree_view_get_cursor (widget, &path, NULL);

		if (path != NULL) {
			gtk_tree_model_get_iter (model, &sibling, path);
			gtk_list_store_insert_after (GTK_LIST_STORE
							 (model),
							 &iter,
							 &sibling);
		} else {
			gtk_list_store_append (GTK_LIST_STORE
						   (model), &iter);
		}

		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
					0, data.destination,
					1, data.gateway,
					2, data.netmask,
					3, data.iface, -1);

		gtk_tree_view_set_model (widget, model);
		path = gtk_tree_model_get_path (model, &iter);
		gtk_tree_view_set_cursor (widget, path, NULL, FALSE);
		gtk_tree_path_free (path);
	}
}

static gint
strip_route_line (gchar * line, netstat_route_data *data)
{
	gint count = 0;
	gchar flags[30];
	gint ref, use;
#ifndef __FreeBDD__
	gchar dest[50];
	gchar **items;
#endif

#ifdef __FreeBSD__
	count = sscanf (line, NETSTAT_ROUTE_FORMAT,
			data->destination,
			data->gateway, flags,
			&ref, &use, data->iface);
#else
	count = sscanf (line, NETSTAT_ROUTE_FORMAT,
			data->destination,
			data->gateway, data->netmask,
			flags, &(data)->metric, &ref, &use,
			data->iface);

	if (count == 6) {
		count = sscanf (line, NETSTAT_ROUTE6_FORMAT,
				dest, data->netmask,
				flags, &(data)->metric,
				&ref, &use, data->iface);

		items = NULL;

		items = g_strsplit (dest, "/", 2);
		if (items != NULL) {
			g_strlcpy (data->destination, items[0], 50 * sizeof (gchar));
			g_strlcpy (data->netmask, items[1], 50 * sizeof (gchar));

			g_strfreev (items);
		}
	}
	
#endif
	return count;
}

//static GtkTreeModel *
void
netstat_create_route_model (GtkTreeView *widget)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_text_new ();
	column =
	    gtk_tree_view_column_new_with_attributes
#ifdef __FreeBSD__
	    (_("Destination/Prefix"), renderer, "text", 0, NULL);
#else
	    (_("Destination"), renderer, "text", 0, NULL);
#endif
	gtk_tree_view_append_column (widget, column);

	renderer = gtk_cell_renderer_text_new ();
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Gateway"), renderer, "text", 1, NULL);
	gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_append_column (widget, column);

#ifndef __FreeBSD__
	renderer = gtk_cell_renderer_text_new ();
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Netmask"), renderer, "text", 2, NULL);
		
	gtk_tree_view_append_column (widget, column);
#endif

	renderer = gtk_cell_renderer_text_new ();
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Interface"), renderer, "text", 3, NULL);
	gtk_tree_view_append_column (widget, column);
}
/* END ROUTE */

/* MULTICAST */
void
netstat_multicast_tree_insert (GtkTreeView *widget, gchar *line)
{
	GtkTreeIter iter, sibling;
	GtkTreePath *path;
	GtkTreeModel *model;
	gint count;
	netstat_multicast_data data;

	g_return_if_fail (GTK_IS_TREE_VIEW (widget));
	g_return_if_fail (line != NULL);

	count = strip_multicast_line (line, &data);

	if (count == 3) {
#ifdef DEBUG
		g_print ("%s\t%s\t%s\n", data.iface,
			data.members, data.group);
#endif /* DEBUG */

		/* Creation of GtkTreeView */
		
		/*columns = gtk_tree_view_get_columns (widget);

		if (g_list_length (columns) == 0) {
			model = netstat_create_multicast_model (widget);
			gtk_tree_view_set_model (widget, model);
		}
		g_list_free (columns);*/

		model = gtk_tree_view_get_model (widget);
		
		if (multicast_model == NULL || multicast_model != model) {
			clean_gtk_tree_view (widget);
			
			multicast_model = GTK_TREE_MODEL (gtk_list_store_new
						(3,
						 G_TYPE_STRING,
						 G_TYPE_STRING,
						 G_TYPE_STRING));
			netstat_create_multicast_model (widget);
			
			gtk_tree_view_set_model (widget, multicast_model);
		}

		model = gtk_tree_view_get_model (widget);
		
		gtk_tree_view_set_rules_hint (widget, TRUE);
		
		gtk_tree_view_get_cursor (widget, &path, NULL);

		if (path != NULL) {
			gtk_tree_model_get_iter (model, &sibling, path);
			gtk_list_store_insert_after (GTK_LIST_STORE
							 (model),
							 &iter,
							 &sibling);
		} else {
			gtk_list_store_append (GTK_LIST_STORE
						   (model), &iter);
		}

		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
					0, data.iface,
					1, data.members,
					2, data.group, -1);

		gtk_tree_view_set_model (widget, model);
		path = gtk_tree_model_get_path (model, &iter);
		gtk_tree_view_set_cursor (widget, path, NULL, FALSE);
		gtk_tree_path_free (path);
	}
}

static gint
strip_multicast_line (gchar * line, netstat_multicast_data *data)
{
	gint count = 0;
	gint members;

	count = sscanf (line, NETSTAT_MULTICAST_FORMAT,
			data->iface,
			&members, data->group);

	snprintf ((data)->members, 30, "%d", members);
	
	return count;
}

//static GtkTreeModel *
void 
netstat_create_multicast_model (GtkTreeView *widget)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	
	renderer = gtk_cell_renderer_text_new ();
	/* Interface of multicast group associated */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Interface"), renderer, "text", 0, NULL);
	gtk_tree_view_append_column (widget, column);

	renderer = gtk_cell_renderer_text_new ();
	/* Members of multicast group */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Member"), renderer, "text", 1, NULL);
	g_object_set (G_OBJECT (renderer), "xalign", 0.5, NULL);
	gtk_tree_view_append_column (widget, column);

	renderer = gtk_cell_renderer_text_new ();
	/* Multicast group */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Group"), renderer, "text", 2, NULL);
	gtk_tree_view_append_column (widget, column);
}
/* END MULTICAST */

/* Remove all columns from a GtkTreeView */
void
clean_gtk_tree_view (GtkTreeView *widget)
{
	GList *columns, *list;
	gint n, i;
	GtkTreeViewColumn *column;
	
	columns = gtk_tree_view_get_columns (widget);
	
	list = columns;
	
	n = g_list_length (columns);
	
	for (i = n; i > 0; i--) {
		column = gtk_tree_view_get_column (widget, i-1);
		gtk_tree_view_remove_column (widget, column);
	}

	g_list_free (columns);
}

/* Copy on clipboard */
void
netstat_copy_to_clipboard (Netinfo * netinfo, gpointer user_data)
{
	GString *result, *content;

	g_return_if_fail (netinfo != NULL);

	result = g_string_new ("");
	
	switch (netstat_get_active_option2 (netinfo)) {
	case PROTOCOL:
		/* The netstat "Display active network services" output in 
	       text format.
		   It's a tabular output, and these belongs to the column titles */
		result = g_string_new (_("Protocol\tIP Source\tPort/Service\tState\n"));
		break;
	case ROUTE:
		/* The netstat "Display routing" output in text format.
		   This seems as a route table.
		   It's a tabular output, and these belongs to the column titles */
		result = g_string_new (_("Destination\tGateway\tNetmask\tInterface\n"));	
		break;
	case MULTICAST:
		/* The netstat "Multicast information" output in text format.
		   It's a tabular output, and these belongs to the column titles */
		result = g_string_new (_("Interface\tMember\tGroup\n"));
		break;
	case NONE:
	default:
		break;
	}

	content = util_tree_model_to_string (GTK_TREE_VIEW (netinfo->output));
	
	g_string_append_printf (result, "%s", content->str);
	
	gtk_clipboard_set_text (gtk_clipboard_get (GDK_NONE), result->str,
				result->len);

	g_string_free (content, TRUE);
	g_string_free (result, TRUE);
}
