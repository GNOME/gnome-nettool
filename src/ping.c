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
#include <glib/gprintf.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

#include "ping.h"
#include "utils.h"

static gint strip_line (gchar * line, ping_data * data, Netinfo * netinfo);
static gint strip_total_line (gchar * line, gint * packet_total);
static GtkTreeModel *ping_create_model (GtkTreeView * widget);
			 
static gfloat rttmin, rttmax, rttavg;
static gint packets_transmitted;
static gint packets_received;
static gint packets_loss;

void
ping_stop (Netinfo * netinfo)
{
	g_return_if_fail (netinfo != NULL);

	netinfo_stop_process_command (netinfo);
}

void
ping_do (Netinfo * netinfo)
{
	gushort count;
	const gchar *host = NULL;
	gchar *command = NULL;
	GtkTreeModel *model;
	GtkLabel *min, *avg, *max, *pkt_transmitted, *pkt_received,
	    *pkt_loss;
	gchar stmp[128];
	gchar *program = NULL;
	GtkWidget *parent;
	gint ip_version;

	g_return_if_fail (netinfo != NULL);

	if (netinfo_validate_host (netinfo) == FALSE) {
		netinfo_stop_process_command (netinfo);
		return;
	}

	count = netinfo_get_count (netinfo);
	host = netinfo_get_host (netinfo);

	rttmin = rttavg = rttmax = packets_loss = 0.0;
	packets_transmitted = packets_received = 0;

	/* Clear the statistics before starting a ping */

	min = GTK_LABEL (netinfo->min);
	avg = GTK_LABEL (netinfo->avg);
	max = GTK_LABEL (netinfo->max);
	gtk_label_set_text (min,
			    g_ascii_formatd (stmp, 128, "%0.2f", rttmin));
	gtk_label_set_text (avg,
			    g_ascii_formatd (stmp, 128, "%0.2f", rttavg));
	gtk_label_set_text (max,
			    g_ascii_formatd (stmp, 128, "%0.2f", rttmax));

	pkt_transmitted = GTK_LABEL (netinfo->packets_transmitted);
	pkt_received = GTK_LABEL (netinfo->packets_received);
	pkt_loss = GTK_LABEL (netinfo->packets_loss);
	g_sprintf (stmp, "%d", packets_transmitted);
	gtk_label_set_text (pkt_transmitted, stmp);
	g_sprintf (stmp, "%d", packets_received);
	gtk_label_set_text (pkt_received, stmp);
	g_sprintf (stmp, "%d%%", packets_loss);
	gtk_label_set_text (pkt_loss, stmp);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (netinfo->output));
	if (GTK_IS_LIST_STORE (model)) {
		gtk_list_store_clear (GTK_LIST_STORE (model));
	}
/*
	buffer =
	    gtk_text_view_get_buffer (GTK_TEXT_VIEW (netinfo->output));

	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_get_end_iter (buffer, &end);

	gtk_text_buffer_delete (buffer, &start, &end);
*/
	parent = gtk_widget_get_toplevel (netinfo->output);

	ip_version = netinfo_get_ip_version (netinfo);
	switch (ip_version)
	{
	case IPV4:
		program = util_find_program_dialog ("ping", parent);
		break;
	case IPV6:
		program = util_find_program_dialog ("ping6", parent);
		
		break;
	case -1:
	default:
		/*invalid host or ip address*/
		return;
	}
	
	if (program != NULL) {
#if defined(__sun__) || defined(__hpux__)
		if (ip_version == IPV4)
			command =
				g_strdup_printf (PING_PROGRAM_FORMAT, program, host,
	 //	    g_strdup_printf (PING_PROGRAM_FORMAT, PING_PROGRAM, host,	
						 count);
		else
			command =
				g_strdup_printf (PING_PROGRAM_FORMAT_6, program, host,
						 count);
#else
		if (ip_version == IPV4)
			command =
				g_strdup_printf (PING_PROGRAM_FORMAT, program, count,
	//	    g_strdup_printf (PING_PROGRAM_FORMAT, PING_PROGRAM, count,
						 host);
		else
			command =
				g_strdup_printf (PING_PROGRAM_FORMAT_6, program, count,
						 host);
#endif

/*	command =
	    g_strdup_printf ("%s ping -c %d %s", PING_PROGRAM, count,
			     host);*/

		netinfo->command_line = g_strsplit (command, " ", -1);
	
		netinfo_process_command (netinfo);
	
		g_strfreev (netinfo->command_line);
	}
	
	g_free (command);
	g_free (program);
}

/* Process each line from ping command */
void
ping_foreach (Netinfo * netinfo, gchar * line, gint len,
	      gpointer user_data)
{
	gchar *text_utf8;
	gssize bytes_written;
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
ping_foreach_with_tree (Netinfo * netinfo, gchar * line, gint len,
			gpointer user_data)
{
	GtkTreeIter iter, sibling;
	GList *columns;
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeView *widget;
	gint count;
	ping_data data;
	gdouble rtt;
	GtkLabel *min, *avg, *max, *pkt_transmitted, *pkt_received,
	    *pkt_loss;
	gchar stmp[128];

	g_return_if_fail (netinfo != NULL);
	g_return_if_fail (line != NULL);

	widget = (GTK_TREE_VIEW (netinfo->output));
	min = GTK_LABEL (netinfo->min);
	avg = GTK_LABEL (netinfo->avg);
	max = GTK_LABEL (netinfo->max);
	pkt_transmitted = GTK_LABEL (netinfo->packets_transmitted);
	pkt_received = GTK_LABEL (netinfo->packets_received);
	pkt_loss = GTK_LABEL (netinfo->packets_loss);

	if (len > 0) {		/* there are data to show */
		count = strip_line (line, &data, netinfo);
		if ((count == 5) || (count == 6)) {

			/* Creation of GtkTreeView */
			gtk_tree_view_set_rules_hint (GTK_TREE_VIEW
						      (widget), TRUE);
			columns =
			    gtk_tree_view_get_columns (GTK_TREE_VIEW
						       (widget));

			if (g_list_length (columns) == 0) {

				model = ping_create_model (widget);
				gtk_tree_view_set_model (GTK_TREE_VIEW
							 (widget), model);
			}
			g_list_free (columns);

			model =
			    gtk_tree_view_get_model (GTK_TREE_VIEW
						     (widget));

			gtk_tree_view_get_cursor (GTK_TREE_VIEW (widget),
						  &path, NULL);

#ifdef DEBUG
			g_print ("%d %s %d %d %s %s\n", data.bytes,
				 data.ip, data.icmp_seq, data.ttl,
				 data.srtt, data.unit);
#endif /* DEBUG */

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
					    0, data.bytes,
					    1, data.ip,
					    2, data.icmp_seq,
					    3, data.srtt,
					    4, data.unit, -1);

			gtk_tree_view_set_model (GTK_TREE_VIEW (widget),
						 model);
			path = gtk_tree_model_get_path (model, &iter);
			gtk_tree_view_set_cursor (GTK_TREE_VIEW (widget),
						  path, NULL, FALSE);
			gtk_tree_path_free (path);

			rtt = g_ascii_strtod (data.srtt, NULL);
			rttmin = (rttmin > 0.0) ? MIN (rttmin, rtt) : rtt;
			rttmax = MAX (rttmax, rtt);
			if (data.icmp_seq == 0 || rttavg == 0.0) {
				rttavg = rtt;
			} else {
				rttavg =
				    (rttavg * data.icmp_seq +
				     rtt) / (data.icmp_seq + 1.0);
			}
			/* ICMP SEQuence tarts at 0, but we need to calculate it as count */
			packets_transmitted = data.icmp_seq + 1;
			packets_received++;

			gtk_label_set_text (min,
					    g_ascii_formatd (stmp, 128,
							     "%0.2f",
							     rttmin));
			gtk_label_set_text (avg,
					    g_ascii_formatd (stmp, 128,
							     "%0.2f",
							     rttavg));
			gtk_label_set_text (max,
					    g_ascii_formatd (stmp, 128,
							     "%0.2f",
							     rttmax));
#ifdef DEBUG
			g_print ("min/avg/max: %0.2f/%0.2f/%0.2f", rttmin,
				 rttavg, rttmax);
			g_print ("\npackets received: %d\n",
				 packets_received);
#endif /* DEBUG */
		} else if (g_strrstr (line, "packets transmitted")) {
			count =
			    strip_total_line (line, &packets_transmitted);
		}
		packets_loss =
		    100 -
		    ((float) packets_received / packets_transmitted * 100);
		g_sprintf (stmp, "%d", packets_transmitted);
		gtk_label_set_text (pkt_transmitted, stmp);
		g_sprintf (stmp, "%d", packets_received);
		gtk_label_set_text (pkt_received, stmp);
		g_sprintf (stmp, "%d%%", packets_loss);
		gtk_label_set_text (pkt_loss, stmp);
	}
}

static gint
strip_line (gchar * line, ping_data * data, Netinfo * netinfo)
{
	gint count;

	if (netinfo_get_ip_version (netinfo) == IPV4)
		line = g_strdelimit (line, ":", ' ');
	else
	    	line = g_strdelimit (line, ",", ' ');

#ifdef PING_PARAMS_5
	count = sscanf (line, PING_FORMAT,
			&(data)->bytes, data->ip, &(data)->icmp_seq,
			data->srtt, data->unit);
#endif
#ifdef PING_PARAMS_6
	count = sscanf (line, PING_FORMAT,
			&(data)->bytes, data->ip, &(data)->icmp_seq,
			&(data)->ttl, data->srtt, data->unit);
#endif
	if (count != 5 && count != 6) {

	}
	/*printf ("DBG: bytes: %d, ip: %s, icmp_seq: %d\n", data->bytes, data->ip, data->icmp_seq);*/
	return count;
}

static gint
strip_total_line (gchar * line, gint * packet_total)
{
	gint count;
	gint packets_received;
	gchar stmp[128];

	count =
	    sscanf (line, PING_TOTAL, packet_total, &packets_received,
		    stmp);

	return count;
}

static GtkTreeModel *
ping_create_model (GtkTreeView * widget)
{
	GtkCellRenderer *renderer;
	static GtkTreeViewColumn *column;
	GtkTreeModel *model;

	renderer = gtk_cell_renderer_text_new ();
	/* Number of bytes received in a ping reply */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Bytes"), renderer, "text", 0, NULL);
	gtk_tree_view_append_column (widget, column);

	renderer = gtk_cell_renderer_text_new ();
	/* IP address that reply a ping */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Source"), renderer, "text", 1, NULL);
	gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_append_column (widget, column);

	renderer = gtk_cell_renderer_text_new ();
	/* Number of sequence of a ICMP request (ping) */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Seq"), renderer, "text", 2, NULL);
	g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
	gtk_tree_view_append_column (widget, column);


	renderer = gtk_cell_renderer_text_new ();
	/* Time elapsed between a packets was sent and
	   when was received its reply */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Time"), renderer, "text", 3, NULL);
	g_object_set (G_OBJECT (renderer), "xalign", 1.0, NULL);
	gtk_tree_view_append_column (widget, column);

	renderer = gtk_cell_renderer_text_new ();
	/* Unit of time elapsed (commonly will be ms) */
	column =
	    gtk_tree_view_column_new_with_attributes
	    (_("Units"), renderer, "text", 4, NULL);
	gtk_tree_view_append_column (widget, column);

	model = GTK_TREE_MODEL (gtk_list_store_new
				(5,
				 G_TYPE_INT,
				 G_TYPE_STRING,
				 G_TYPE_INT,
				 G_TYPE_STRING, G_TYPE_STRING));
	return model;
}

/* Copy on clipboard */
void
ping_copy_to_clipboard (Netinfo * netinfo, gpointer user_data)
{
	GString *result, *content;
	const gchar *min, *avg, *max;
	const gchar *pkt_transmitted, *pkt_received, *pkt_loss;

	g_return_if_fail (netinfo != NULL);

	/* The ping output in text format:
	   Bytes received, Address Source, Number of Sequence, 
	   Round Trip Time (Time), Units of Time.
	   It's a tabular output, and these belongs to the column titles */
	result = g_string_new (_("Bytes\tSource\tSeq\tTime\tUnits\n"));

	content = util_tree_model_to_string (GTK_TREE_VIEW (netinfo->output));
	g_string_append_printf (result, "%s", content->str);

	min = gtk_label_get_text (GTK_LABEL (netinfo->min));
	avg = gtk_label_get_text (GTK_LABEL (netinfo->avg));
	max = gtk_label_get_text (GTK_LABEL (netinfo->max));
	pkt_transmitted =
	    gtk_label_get_text (GTK_LABEL (netinfo->packets_transmitted));
	pkt_received =
	    gtk_label_get_text (GTK_LABEL (netinfo->packets_received));
	pkt_loss = gtk_label_get_text (GTK_LABEL (netinfo->packets_loss));

	/* The ping output in a text format (to copy on clipboard) */
	g_string_append_printf (result, _("Time minimum:\t%s ms\n"), min);
	g_string_append_printf (result, _("Time average:\t%s ms\n"), avg);
	g_string_append_printf (result, _("Time maximum:\t%s ms\n"), max);

	g_string_append_printf (result, _("Packets transmitted:\t%s\n"),
				pkt_transmitted);
	g_string_append_printf (result, _("Packets received:\t%s\n"),
				pkt_received);
	g_string_append_printf (result, _("Packet loss:\t%s\n"), pkt_loss);

	gtk_clipboard_set_text (gtk_clipboard_get (GDK_NONE), result->str,
				result->len);

	g_string_free (content, TRUE);
	g_string_free (result, TRUE);
}
