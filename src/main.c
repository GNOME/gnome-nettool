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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gnome.h>
#include <glade/glade.h>

#include "callbacks.h"
#include "ping.h"
#include "traceroute.h"
#include "info.h"
#include "netstat.h"
#include "scan.h"
#include "lookup.h"
#include "finger.h"
#include "whois.h"

Netinfo *load_ping_widgets_from_xml (GladeXML * xml);
Netinfo *load_traceroute_widgets_from_xml (GladeXML * xml);
Netinfo *load_netstat_widgets_from_xml (GladeXML * xml);
Netinfo *load_scan_widgets_from_xml (GladeXML * xml);
Netinfo *load_lookup_widgets_from_xml (GladeXML * xml);
Netinfo *load_finger_widgets_from_xml (GladeXML * xml);
Netinfo *load_whois_widgets_from_xml (GladeXML * xml);
Netinfo *load_info_widgets_from_xml (GladeXML * xml);
static gboolean start_initial_process_cb (gpointer data);

int
main (int argc, char *argv[])
{
	GtkWidget *window;
	GladeXML *xml;
	GtkWidget *notebook;
	const gchar *dialog = DATADIR "gnome-netinfo.glade";
	Netinfo *pinger;
	Netinfo *tracer;
	Netinfo *netstat;
	Netinfo *info;
	Netinfo *scan;
	Netinfo *lookup;
	Netinfo *finger;
	Netinfo *whois;
	gchar *icon_path;
	gint current_page = 0;
	static gchar *info_input = NULL;
	static gchar *ping_input = NULL;
	static gchar *netstat_input = NULL;
	static gchar *scan_input = NULL;
	static gchar *traceroute_input = NULL;
	static gchar *lookup_input = NULL;
	static gchar *finger_input = NULL;
	static gchar *whois_input = NULL;
	static const struct poptOption options[] = {
		{ "info", 'i', POPT_ARG_STRING, &info_input, 0,
		  N_("Load information for a network device"),
		  N_("DEVICE") },
		{ "ping", 'p', POPT_ARG_STRING, &ping_input, 0,
		  N_("Send a ping to a network address"),
		  N_("HOST") },
		{ "netstat", 'n', POPT_ARG_STRING, &netstat_input, 0,
		  N_("Get netstat information.  Valid options are: route, active, multicast."),
		  N_("COMMAND") },
		{ "traceroute", 't', POPT_ARG_STRING, &traceroute_input, 0,
		  N_("Trace a route to a network address"),
		  N_("HOST") },
		{ "port-scan", 's', POPT_ARG_STRING, &scan_input, 0,
		  N_("Port scan a network address"),
		  N_("HOST") },
		{ "lookup", 'l', POPT_ARG_STRING, &lookup_input, 0,
		  N_("Look up a network address"),
		  N_("HOST") },
		{ "finger", 'f', POPT_ARG_STRING, &finger_input, 0,
		  N_("Finger command to run"),
		  N_("USER") },
		{ "whois", 'w', POPT_ARG_STRING, &whois_input, 0,
		  N_("Perform a whois lookup for a network domain"),
		  N_("DOMAIN") },
		{ NULL, '\0', 0, NULL, 0 }
	};

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, GNOME_NETINFO_LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE,
			    argc, argv,
			    GNOME_PARAM_HUMAN_READABLE_NAME,
			    _("Network Information"),
			    GNOME_PARAM_POPT_TABLE, options,
			    GNOME_PARAM_APP_DATADIR, DATADIR,
			    GNOME_PARAM_NONE);

	if (!g_file_test (dialog, G_FILE_TEST_EXISTS)) {
		g_critical (_("The file %s doesn't exist, "
			      "please check if gnome-netinfo is correcly installed"),
			    dialog);
		return -1;
	}

	icon_path = g_build_filename (GNOME_ICONDIR, "gnome-netinfo.png", NULL);
	gnome_window_icon_set_default_from_file (icon_path);
	g_free (icon_path);

	xml = glade_xml_new (dialog, "main_window", NULL);
	window = glade_xml_get_widget (xml, "main_window");

	pinger = load_ping_widgets_from_xml (xml);
	tracer = load_traceroute_widgets_from_xml (xml);
	netstat = load_netstat_widgets_from_xml (xml);
	info = load_info_widgets_from_xml (xml);
	scan = load_scan_widgets_from_xml (xml);
	lookup = load_lookup_widgets_from_xml (xml);
	finger = load_finger_widgets_from_xml (xml);
	whois = load_whois_widgets_from_xml (xml);

	if (info_input) {
		current_page = INFO;
		info_set_nic (info, info_input);
	}
	if (ping_input) {
		current_page = PING;
		netinfo_set_host (pinger, ping_input);
		gtk_idle_add (start_initial_process_cb, pinger);
	}
	if (netstat_input) {
		current_page = NETSTAT;
		if (! strcmp (netstat_input, "route"))
			gtk_toggle_button_set_active
				(GTK_TOGGLE_BUTTON (netstat->routing), TRUE);
		else if (! strcmp (netstat_input, "active"))
			gtk_toggle_button_set_active
				(GTK_TOGGLE_BUTTON (netstat->protocol), TRUE);
		else if (! strcmp (netstat_input, "multicast"))
			gtk_toggle_button_set_active
				(GTK_TOGGLE_BUTTON (netstat->multicast), TRUE);
		gtk_idle_add (start_initial_process_cb, netstat);
	}
	if (traceroute_input) {
		current_page = TRACEROUTE;
		netinfo_set_host (tracer, traceroute_input);
		gtk_idle_add (start_initial_process_cb, tracer);
	}
	if (scan_input) {
		current_page = PORTSCAN;
		netinfo_set_host (scan, scan_input);
		gtk_idle_add (start_initial_process_cb, scan);
	}
	if (lookup_input) {
		current_page = LOOKUP;
		netinfo_set_host (lookup, lookup_input);
		gtk_idle_add (start_initial_process_cb, lookup);
	}
	if (finger_input) {
		gchar **split_input = NULL;
		current_page = FINGER;
		split_input = g_strsplit (finger_input, "@", 2);
		if (split_input[0])
			netinfo_set_user (finger, split_input[0]);
		if (split_input[1])
			netinfo_set_host (finger, split_input[1]);
		g_strfreev (split_input);
		gtk_idle_add (start_initial_process_cb, finger);
	}
	if (whois_input) {
		current_page = WHOIS;
		netinfo_set_host (whois, whois_input);
		gtk_idle_add (start_initial_process_cb, whois);
	}

	notebook = glade_xml_get_widget (xml, "notebook");
	g_object_set_data (G_OBJECT (notebook), "pinger", pinger);
	g_object_set_data (G_OBJECT (notebook), "tracer", tracer);
	g_object_set_data (G_OBJECT (notebook), "netstat", netstat);
	g_object_set_data (G_OBJECT (notebook), "info", info);
	g_object_set_data (G_OBJECT (notebook), "scan", scan);
	g_object_set_data (G_OBJECT (notebook), "lookup", lookup);
	g_object_set_data (G_OBJECT (notebook), "finger", finger);
	g_object_set_data (G_OBJECT (notebook), "whois", whois);
	
	glade_xml_signal_autoconnect (xml);
	g_object_unref (G_OBJECT (xml));

	gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), current_page);

	gtk_widget_show_all (window);

	gtk_main ();

	g_free (pinger);
	g_free (tracer);
	g_free (netstat);
	g_free (info);
	g_free (scan);
	g_free (lookup);
	g_free (finger);
	g_free (whois);

	return 0;
}

static gboolean
start_initial_process_cb (gpointer data)
{
	Netinfo *ni = data;
	NetinfoActivateFn fn_cb;

	g_return_val_if_fail (data != NULL, FALSE);

	fn_cb = (NetinfoActivateFn) ni->button_callback;
	if (fn_cb)
		(*fn_cb) (ni->button, data);
	return FALSE;
}

/* The value returned must be released from memory */
Netinfo *
load_ping_widgets_from_xml (GladeXML * xml)
{
	Netinfo *pinger;
	GtkWidget *vbox_ping;

	g_return_val_if_fail (xml != NULL, NULL);

	pinger = g_new0 (Netinfo, 1);

	pinger->main_window = glade_xml_get_widget (xml, "main_window");
	pinger->running = FALSE;
	pinger->child_pid = 0;
	pinger->host = glade_xml_get_widget (xml, "ping_host");
	pinger->count = glade_xml_get_widget (xml, "ping_count");
	pinger->output = glade_xml_get_widget (xml, "ping_output");
	pinger->limited = glade_xml_get_widget (xml, "ping_limited");
	pinger->button = glade_xml_get_widget (xml, "ping_button");
	pinger->sensitive = pinger->host;
	pinger->label_run = _("Ping");
	pinger->label_stop = NULL;
	pinger->routing = NULL;
	pinger->protocol = NULL;
	pinger->multicast = NULL;
	pinger->min = glade_xml_get_widget (xml, "ping_minimum");
	pinger->avg = glade_xml_get_widget (xml, "ping_average");
	pinger->max = glade_xml_get_widget (xml, "ping_maximum");
	pinger->packets_transmitted = glade_xml_get_widget (xml, "ping_packets_transmitted");
	pinger->packets_received = glade_xml_get_widget (xml, "ping_packets_received");
	pinger->packets_loss = glade_xml_get_widget (xml, "ping_packets_loss");

	vbox_ping = glade_xml_get_widget (xml, "vbox_ping");
	
	pinger->button_callback = G_CALLBACK (on_ping_activate);
	pinger->process_line = NETINFO_FOREACH_FUNC (ping_foreach_with_tree);
	pinger->copy_output = NETINFO_COPY_FUNC (ping_copy_to_clipboard);
	
	g_signal_connect (G_OBJECT (pinger->host), "activate",
				  G_CALLBACK (on_ping_activate),
				  pinger);
	g_signal_connect (G_OBJECT (pinger->button), "clicked",
				  pinger->button_callback,
				  pinger);

	return pinger;
}

/* The value returned must be released from memory */
Netinfo *
load_traceroute_widgets_from_xml (GladeXML * xml)
{
	Netinfo *tracer;
	GtkWidget *vbox_traceroute;

	g_return_val_if_fail (xml != NULL, NULL);

	tracer = g_new0 (Netinfo, 1);

	tracer->main_window = glade_xml_get_widget (xml, "main_window");
	tracer->running = FALSE;
	tracer->child_pid = 0;
	tracer->host = glade_xml_get_widget (xml, "traceroute_host");
	tracer->output = glade_xml_get_widget (xml, "traceroute_output");
	tracer->button = glade_xml_get_widget (xml, "traceroute_button");
	tracer->count = NULL;
	tracer->limited = NULL;
	tracer->sensitive = tracer->host;
	tracer->label_run = _("Trace");
	tracer->label_stop = NULL;
	tracer->routing = NULL;
	tracer->protocol = NULL;
	tracer->multicast = NULL;
	
	vbox_traceroute = glade_xml_get_widget (xml, "vbox_traceroute");

	tracer->button_callback = G_CALLBACK (on_traceroute_activate);
	tracer->process_line = NETINFO_FOREACH_FUNC (traceroute_foreach_with_tree);
	tracer->copy_output = NETINFO_COPY_FUNC (traceroute_copy_to_clipboard);

	g_signal_connect (G_OBJECT (tracer->host), "activate",
				  G_CALLBACK (on_traceroute_activate),
				  tracer);
	g_signal_connect (G_OBJECT (tracer->button), "clicked",
				  tracer->button_callback,
				  tracer);

	return tracer;
}

Netinfo *
load_netstat_widgets_from_xml (GladeXML * xml)
{
	Netinfo *netstat;
	GtkWidget *vbox_netstat;

	g_return_val_if_fail (xml != NULL, NULL);

	netstat = g_new0 (Netinfo, 1);

	netstat->main_window = glade_xml_get_widget (xml, "main_window");
	netstat->running = FALSE;
	netstat->child_pid = 0;
	netstat->host = NULL;
	netstat->count = NULL;
	netstat->output = glade_xml_get_widget (xml, "netstat_output");
	netstat->limited = NULL;
	netstat->button = glade_xml_get_widget (xml, "netstat_button");
	netstat->routing = glade_xml_get_widget (xml, "netstat_routing");
	netstat->protocol = glade_xml_get_widget (xml, "netstat_protocol");
	netstat->multicast = glade_xml_get_widget (xml, "netstat_multicast");
	netstat->sensitive = NULL;
	netstat->label_run = _("Netstat");
	netstat->label_stop = NULL;
	
	vbox_netstat = glade_xml_get_widget (xml, "vbox_netstat");
	
	netstat->button_callback = G_CALLBACK (on_netstat_activate);
	netstat->process_line = NETINFO_FOREACH_FUNC (netstat_foreach_with_tree);
	netstat->copy_output = NETINFO_COPY_FUNC (netstat_copy_to_clipboard);	
	
	g_signal_connect (G_OBJECT (netstat->button), "clicked",
				  netstat->button_callback,
				  netstat);
/*
	g_signal_connect (G_OBJECT (netstat->protocol), "toggled",
				  G_CALLBACK (on_protocol_button_toggled),
				  netstat);
	g_signal_connect (G_OBJECT (netstat->routing), "toggled",
				  G_CALLBACK (on_protocol_button_toggled),
				  netstat);
	g_signal_connect (G_OBJECT (netstat->multicast), "toggled",
				  G_CALLBACK (on_protocol_button_toggled),
				  netstat);
*/
	return netstat;
}

/* The value returned must be released from memory */
Netinfo *
load_info_widgets_from_xml (GladeXML * xml)
{
	Netinfo *info;
	GtkWidget *combo;
	GtkWidget *vbox_info;

	g_return_val_if_fail (xml != NULL, NULL);

	info = g_malloc (sizeof (Netinfo));

	info->main_window = glade_xml_get_widget (xml, "main_window");
	info->hw_address = glade_xml_get_widget (xml, "info_hw_address");
	info->ip_address = glade_xml_get_widget (xml, "info_ip_address");
	info->netmask = glade_xml_get_widget (xml, "info_netmask");
	info->broadcast = glade_xml_get_widget (xml, "info_broadcast");
	info->multicast = glade_xml_get_widget (xml, "info_multicast");
	info->link_speed = glade_xml_get_widget (xml, "info_link_speed");
	info->state = glade_xml_get_widget (xml, "info_state");
	info->mtu = glade_xml_get_widget (xml, "info_mtu");
	info->tx_bytes = glade_xml_get_widget (xml, "info_tx_bytes");
	info->tx = glade_xml_get_widget (xml, "info_tx");
	info->tx_errors = glade_xml_get_widget (xml, "info_tx_errors");
	info->rx_bytes = glade_xml_get_widget (xml, "info_rx_bytes");
	info->rx = glade_xml_get_widget (xml, "info_rx");
	info->rx_errors = glade_xml_get_widget (xml, "info_rx_errors");
	info->collisions = glade_xml_get_widget (xml, "info_collisions");

	vbox_info = glade_xml_get_widget (xml, "vbox_info");

/*
#ifdef IFCONFIG_PROGRAM
*/
	info->nic = glade_xml_get_widget (xml, "info_nic");
	combo = glade_xml_get_widget (xml, "info_combo");
	
	g_signal_connect (G_OBJECT (info->nic), "changed",
				  G_CALLBACK (info_nic_changed),
				  info);
	info_load_iface (info, combo);
/*
#else
	gtk_widget_set_sensitive (vbox_info, FALSE);
#endif
*/
	info->copy_output = NETINFO_COPY_FUNC (info_copy_to_clipboard);

	return info;
}

Netinfo *
load_scan_widgets_from_xml (GladeXML * xml)
{
	Netinfo *scan;

	g_return_val_if_fail (xml != NULL, NULL);

	scan = g_new0 (Netinfo, 1);

	scan->main_window = glade_xml_get_widget (xml, "main_window");
	scan->running = FALSE;
	scan->child_pid = 0;
	scan->host = glade_xml_get_widget (xml, "scan_host");
	scan->count = NULL;
	scan->output = glade_xml_get_widget (xml, "scan_output");
	scan->limited = NULL;
	scan->button = glade_xml_get_widget (xml, "scan_button");
	scan->routing = NULL;
	scan->protocol = NULL;
	scan->multicast = NULL;
	scan->sensitive = NULL;
	scan->label_run = _("Scan");
	scan->label_stop = NULL;
	
	scan->button_callback = G_CALLBACK (on_scan_activate);
	scan->copy_output = NETINFO_COPY_FUNC (scan_copy_to_clipboard);
	scan->process_line = NETINFO_FOREACH_FUNC (scan_foreach);
	
	g_signal_connect (G_OBJECT (scan->host), "activate",
                          scan->button_callback,
                          scan);
	g_signal_connect (G_OBJECT (scan->button), "clicked",
                          scan->button_callback,
                          scan);

	return scan;
}

/* The value returned must be released from memory */
Netinfo *
load_lookup_widgets_from_xml (GladeXML * xml)
{
	Netinfo *lookup;
	GtkWidget *vbox_lookup;

	g_return_val_if_fail (xml != NULL, NULL);

	lookup = g_new0 (Netinfo, 1);

	lookup->main_window = glade_xml_get_widget (xml, "main_window");
	lookup->running = FALSE;
	lookup->child_pid = 0;
	lookup->host = glade_xml_get_widget (xml, "lookup_host");
	lookup->output = glade_xml_get_widget (xml, "lookup_output");
	lookup->button = glade_xml_get_widget (xml, "lookup_button");
	lookup->type = glade_xml_get_widget (xml, "lookup_type");
	lookup->count = NULL;
	lookup->limited = NULL;
	lookup->sensitive = lookup->host;
	lookup->label_run = _("Lookup");
	lookup->label_stop = NULL;
	lookup->routing = NULL;
	lookup->protocol = NULL;
	lookup->multicast = NULL;
	
	vbox_lookup = glade_xml_get_widget (xml, "vbox_lookup");

	lookup->button_callback = G_CALLBACK (on_lookup_activate);
	lookup->process_line = NETINFO_FOREACH_FUNC (lookup_foreach_with_tree);
	lookup->copy_output = NETINFO_COPY_FUNC (lookup_copy_to_clipboard);

	g_signal_connect (G_OBJECT (lookup->host), "activate",
			  G_CALLBACK (on_lookup_activate),
			  lookup);
	g_signal_connect (G_OBJECT (lookup->button), "clicked",
			  lookup->button_callback,
			  lookup);

	return lookup;
}

/* The value returned must be released from memory */
Netinfo *
load_finger_widgets_from_xml (GladeXML * xml)
{
	Netinfo *finger;
	GtkWidget *vbox_finger;
	PangoFontDescription *font_desc;

	g_return_val_if_fail (xml != NULL, NULL);

	finger = g_new0 (Netinfo, 1);

	finger->main_window = glade_xml_get_widget (xml, "main_window");
	finger->running = FALSE;
	finger->child_pid = 0;
	finger->user = glade_xml_get_widget (xml, "finger_user");
	finger->host = glade_xml_get_widget (xml, "finger_host");
	finger->output = glade_xml_get_widget (xml, "finger_output");
	finger->button = glade_xml_get_widget (xml, "finger_button");
	finger->type = glade_xml_get_widget (xml, "finger_type");
	finger->count = NULL;
	finger->limited = NULL;
	finger->sensitive = glade_xml_get_widget (xml, "finger_input_box");
	finger->label_run = _("Finger");
	finger->label_stop = NULL;
	finger->routing = NULL;
	finger->protocol = NULL;
	finger->multicast = NULL;
	
	vbox_finger = glade_xml_get_widget (xml, "vbox_finger");

	font_desc = pango_font_description_new ();
	pango_font_description_set_family (font_desc, "monospace");
	gtk_widget_modify_font (finger->output, font_desc);
	pango_font_description_free (font_desc);

	finger->button_callback = G_CALLBACK (on_finger_activate);
	finger->process_line = NETINFO_FOREACH_FUNC (finger_foreach);
	finger->copy_output = NETINFO_COPY_FUNC (finger_copy_to_clipboard);

	g_signal_connect (G_OBJECT (finger->user), "activate",
			  G_CALLBACK (on_finger_activate),
			  finger);
	g_signal_connect (G_OBJECT (finger->host), "activate",
			  G_CALLBACK (on_finger_activate),
			  finger);
	g_signal_connect (G_OBJECT (finger->button), "clicked",
			  finger->button_callback,
			  finger);

	return finger;
}

/* The value returned must be released from memory */
Netinfo *
load_whois_widgets_from_xml (GladeXML * xml)
{
	Netinfo *whois;
	GtkWidget *vbox_whois;
	PangoFontDescription *font_desc;

	g_return_val_if_fail (xml != NULL, NULL);

	whois = g_new0 (Netinfo, 1);

	whois->main_window = glade_xml_get_widget (xml, "main_window");
	whois->running = FALSE;
	whois->child_pid = 0;
	whois->host = glade_xml_get_widget (xml, "whois_host");
	whois->output = glade_xml_get_widget (xml, "whois_output");
	whois->button = glade_xml_get_widget (xml, "whois_button");
	whois->count = NULL;
	whois->limited = NULL;
	whois->sensitive = glade_xml_get_widget (xml, "whois_input_box");
	whois->label_run = _("Whois");
	whois->label_stop = NULL;
	whois->routing = NULL;
	whois->protocol = NULL;
	whois->multicast = NULL;

	vbox_whois = glade_xml_get_widget (xml, "vbox_whois");

	font_desc = pango_font_description_new ();
	pango_font_description_set_family (font_desc, "monospace");
	gtk_widget_modify_font (whois->output, font_desc);
	pango_font_description_free (font_desc);

	whois->button_callback = G_CALLBACK (on_whois_activate);
	whois->process_line = NETINFO_FOREACH_FUNC (whois_foreach);
	whois->copy_output = NETINFO_COPY_FUNC (whois_copy_to_clipboard);

	g_signal_connect (G_OBJECT (whois->host), "activate",
			  G_CALLBACK (on_whois_activate),
			  whois);
	g_signal_connect (G_OBJECT (whois->button), "clicked",
			  whois->button_callback,
			  whois);

	return whois;
}