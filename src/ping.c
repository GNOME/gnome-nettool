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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

#include "ping.h"
#include "utils.h"

enum {
	BYTES_COLUMN = 0,
	IP_COLUMN,
	ICMP_SEQ_COLUMN,
	SRTT_COLUMN,
	UNIT_COLUMN,
};

static gint strip_line (gchar * line, ping_data * data, Netinfo * netinfo);
static gint strip_total_line (gchar * line, gint * packet_total);
static GtkTreeModel *ping_create_model (GtkTreeView * widget);
			 
static gfloat rttmin, rttmax, rttavg;
static gint packets_transmitted;
static gint packets_received;
static gint packets_success;

typedef struct {
	gdouble value;
	gboolean valid;
} PingGraphBarData;

static void
draw_centered_text (GtkWidget *widget, cairo_t *cr, gint x, gint y, gchar *text)
{
	int width;
	PangoLayout *layout;

	layout = gtk_widget_create_pango_layout (widget, text);

	pango_layout_get_pixel_size (layout, &width, NULL);

	x -= width/2;

	gtk_paint_layout (gtk_widget_get_style(widget), cr,
			  GTK_STATE_NORMAL, TRUE, 
			  widget, NULL, x, y,
			  layout);
	g_object_unref (layout);
}

static gdouble 
get_bar_data (Netinfo *netinfo, PingGraphBarData *bar_data, int ntodisplay,
	      gint rangemin, gint rangemax)
{
	gint i, index;
	GtkTreeModel *results;
	GtkTreeIter node;
	gboolean nodeavailable;
	gint seqnumber;
	gchar *srtt_str;
	gdouble max;

	for (i = 0; i<ntodisplay; i++) {
		bar_data[i].value = 0;
		bar_data[i].valid = FALSE;
	}
	
	results = gtk_tree_view_get_model (GTK_TREE_VIEW (netinfo->output));

	if (results != NULL) {
		nodeavailable = gtk_tree_model_get_iter_first (results, &node);
		while (nodeavailable) {
			gtk_tree_model_get (results, &node, 
					    ICMP_SEQ_COLUMN, &seqnumber, -1);
			index = seqnumber - rangemin - 1;
			if (seqnumber > rangemin) {
				gtk_tree_model_get (results, &node, 
						    SRTT_COLUMN, &srtt_str, 
						    -1);
				bar_data[index].value = g_strtod (srtt_str, 
								  NULL);
				g_free (srtt_str);
				bar_data[index].valid = TRUE;
			}
			nodeavailable = gtk_tree_model_iter_next (results, &node);
		}

	}

	max = 0.0;
	for (i = 0; i<ntodisplay; i++)
		if (bar_data[i].valid && (bar_data[i].value > max))
			max = bar_data[i].value;

	return max;
}

static void 
draw_ping_graph (Netinfo *netinfo)
{
	cairo_t *cr;
	GtkStyle *style;
	GtkWidget *widget;
	PangoLayout *layout;
	gint ntodisplay = 5;
	gint rangemin, rangemax;
	PingGraphBarData *bar_data;
	gdouble max;
	gint width, height;
	gint font_height, offset;
	gint bar_height, separator_height;
	gdouble scale_factor;
	gint line1h, line2h;
	gint index;
	gint step, x, h;
	gchar *tmpstr;

	widget = netinfo->graph;
	cr = gdk_cairo_create (gtk_widget_get_window(widget));
	style = gtk_widget_get_style(widget);

	rangemax = packets_transmitted;
	rangemin = MAX (0, rangemax - ntodisplay);

	bar_data = g_newa (PingGraphBarData, ntodisplay);
	max = get_bar_data (netinfo, bar_data, ntodisplay, rangemin,
			    rangemax);

	/* Created up here so we can get the geometry info. */
	layout = gtk_widget_create_pango_layout (widget, _("Time (ms):"));
	/* We guess that the first label is representative. */
	pango_layout_get_pixel_size (layout, NULL, &font_height);
	width = gtk_widget_get_allocated_width (widget);
	height = gtk_widget_get_allocated_height (widget);

	offset = 0.05*height;
	bar_height = height - 2.5*font_height - offset;
	scale_factor = bar_height / max;
	separator_height = bar_height + offset;
	line1h = bar_height + 0.125*font_height + offset;
	line2h = bar_height + 1.25*font_height + offset;

	gtk_paint_box (style, cr, GTK_STATE_NORMAL, GTK_SHADOW_ETCHED_IN, 
		       widget, NULL, 0, 0, width, height);

	gtk_paint_layout (style, cr, GTK_STATE_NORMAL, TRUE, 
			  widget, NULL, 0.02*width, line1h,
			  layout);
	g_object_unref (layout);

	layout = gtk_widget_create_pango_layout (widget, _("Seq. No.:"));
	gtk_paint_layout (style, cr, GTK_STATE_NORMAL, TRUE, 
			  widget, NULL, 0.02*width, line2h,
			  layout);
	g_object_unref (layout);

	gtk_paint_hline (style, cr, GTK_STATE_NORMAL, widget, NULL,
			 0.02*width, 0.98*width, separator_height);

	index = 0;
	step = width / (ntodisplay + 1.0);
	for (x = 1.5*step; x < width; x += step) {
		if (bar_data[index].valid) {
			h = scale_factor*bar_data[index].value;
			gtk_paint_flat_box (style, cr, GTK_STATE_SELECTED,
					    GTK_SHADOW_ETCHED_IN, widget,
					    NULL, x - 0.4*step,
					    offset + bar_height - h,
					    0.8*step, h);
			tmpstr = g_strdup_printf ("%.2f", bar_data[index].value);
		} else {
			tmpstr = g_strdup ("-");
		}
		draw_centered_text (widget, cr, x, line1h, tmpstr);
		g_free (tmpstr);
		if (index + rangemin + 1 <= rangemax) {
			tmpstr = g_strdup_printf ("%d", index + rangemin + 1);
		} else {
			tmpstr = g_strdup ("-");
		}
		draw_centered_text (widget, cr, x, line2h, tmpstr);
		g_free (tmpstr);
		index++;
	}

	cairo_destroy (cr);
}

gint 
on_ping_graph_draw (GtkWidget *widget, cairo_t *cr,
		      Netinfo *info)
{
	draw_ping_graph (info);

	return TRUE;
}

void
ping_stop (Netinfo * netinfo)
{
	g_return_if_fail (netinfo != NULL);

	netinfo_stop_process_command (netinfo);
}

void
ping_do (Netinfo * netinfo)
{
	gint count;
	const gchar *host = NULL;
	gchar *command = NULL;
	GtkTreeModel *model;
	GtkLabel *min, *avg, *max, *pkt_transmitted, *pkt_received,
	    *pkt_success;
	gchar stmp[128];
	gchar *program = NULL;
	gchar *count_string = NULL;
	GtkWidget *parent;
	gint ip_version;

	g_return_if_fail (netinfo != NULL);

	/* Because of the delay, we can't check twice for a hostname/IP.
	 * It was made before this function was called.  Anyway, we
	 * check at least if we have a text as hostname */
	if (netinfo_validate_domain (netinfo) == FALSE) {
		netinfo_stop_process_command (netinfo);
		return;
	}

	count = netinfo_get_count (netinfo);
	host = netinfo_get_host (netinfo);

	if (netinfo->stbar_text)
		g_free (netinfo->stbar_text);
	netinfo->stbar_text = g_strdup_printf (_("Sending ping requests to %s"), host);
	
	rttmin = rttavg = rttmax = 0.0;
	packets_transmitted = packets_received = packets_success = 0;

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
	pkt_success = GTK_LABEL (netinfo->packets_success);
	g_sprintf (stmp, "%d", packets_transmitted);
	gtk_label_set_text (pkt_transmitted, stmp);
	g_sprintf (stmp, "%d", packets_received);
	gtk_label_set_text (pkt_received, stmp);
	g_sprintf (stmp, "%d%%", packets_success);
	gtk_label_set_text (pkt_success, stmp);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (netinfo->output));
	if (GTK_IS_LIST_STORE (model)) {
		gtk_list_store_clear (GTK_LIST_STORE (model));
	}

	draw_ping_graph (netinfo);

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
		/* unlimited or limited ping? */
		if (count == -1) {
			count_string = g_strdup_printf(" ");
		} else {
#if defined(__sun__) || defined(__hpux__)
			count_string = g_strdup_printf("%d", count);
#else
			count_string = g_strdup_printf(" -c %d ", count);
#endif
		}

		if (ip_version == IPV4) {
			command =
#if defined(__sun__) || defined(__hpux__)
				g_strdup_printf (PING_PROGRAM_FORMAT, program, 
						host, count_string);
#else
				g_strdup_printf (PING_PROGRAM_FORMAT, program, 
						count_string, host);
#endif
		} else {
			command =
#if defined(__sun__) || defined(__hpux__)
				g_strdup_printf (PING_PROGRAM_FORMAT_6, program, 
						host, count_string);
#else
				g_strdup_printf (PING_PROGRAM_FORMAT_6, program, 
						count_string, host);
#endif
		}
		g_print("command: %s\n", command);

		netinfo->command_line = g_strsplit (command, " ", -1);
	
		netinfo_process_command (netinfo);
	
		g_strfreev (netinfo->command_line);
	}
	
	g_free (count_string);
	g_free (command);
	g_free (program);
}

/* Process each line from ping command */
void
ping_foreach (Netinfo * netinfo, gchar * line, gssize len,
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
ping_foreach_with_tree (Netinfo * netinfo, gchar * line, gint len,
			gpointer user_data)
{
	GtkTreeIter iter;
	GList *columns;
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeView *widget;
	gint count;
	ping_data data;
	gdouble rtt;
	GtkLabel *min, *avg, *max, *pkt_transmitted, *pkt_received,
	    *pkt_success;
	gchar stmp[128];

	g_return_if_fail (netinfo != NULL);
	g_return_if_fail (line != NULL);

	widget = (GTK_TREE_VIEW (netinfo->output));
	min = GTK_LABEL (netinfo->min);
	avg = GTK_LABEL (netinfo->avg);
	max = GTK_LABEL (netinfo->max);
	pkt_transmitted = GTK_LABEL (netinfo->packets_transmitted);
	pkt_received = GTK_LABEL (netinfo->packets_received);
	pkt_success = GTK_LABEL (netinfo->packets_success);

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

			gtk_list_store_append (GTK_LIST_STORE
					       (model), &iter);

			gtk_list_store_set (GTK_LIST_STORE (model), &iter,
					    BYTES_COLUMN, data.bytes,
					    IP_COLUMN, data.ip,
					    ICMP_SEQ_COLUMN, data.icmp_seq,
					    SRTT_COLUMN, data.srtt,
					    UNIT_COLUMN, data.unit, -1);

			gtk_tree_view_set_model (GTK_TREE_VIEW (widget),
						 model);

			if (path) {
				gtk_tree_view_set_cursor (
						GTK_TREE_VIEW (widget),
						path, NULL, FALSE);
				gtk_tree_path_free (path);
			}

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

			/* Beep if user selected to */
			if (netinfo->has_beep) {
				gdk_beep ();
			};

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

		if (packets_transmitted == 0) {
			packets_success = 0;
		} else {
			packets_success =
				((float) packets_received / packets_transmitted * 100);
		}

		g_sprintf (stmp, "%d", packets_transmitted);
		gtk_label_set_text (pkt_transmitted, stmp);
		g_sprintf (stmp, "%d", packets_received);
		gtk_label_set_text (pkt_received, stmp);
		g_sprintf (stmp, "%d%%", packets_success);
		gtk_label_set_text (pkt_success, stmp);
	}
	draw_ping_graph (netinfo);
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
	const gchar *pkt_transmitted, *pkt_received, *pkt_success;

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
	pkt_success = gtk_label_get_text (GTK_LABEL (netinfo->packets_success));

	/* The ping output in a text format (to copy on clipboard) */
	g_string_append_printf (result, _("Time minimum:\t%s ms\n"), min);
	g_string_append_printf (result, _("Time average:\t%s ms\n"), avg);
	g_string_append_printf (result, _("Time maximum:\t%s ms\n"), max);

	g_string_append_printf (result, _("Packets transmitted:\t%s\n"),
				pkt_transmitted);
	g_string_append_printf (result, _("Packets received:\t%s\n"),
				pkt_received);
	g_string_append_printf (result, 
	            _("Successful packets:\t%s\n"), pkt_success);

	gtk_clipboard_set_text (gtk_clipboard_get (GDK_NONE), result->str,
				result->len);

	g_string_free (content, TRUE);
	g_string_free (result, TRUE);
}
