/* gnome-netinfo - A GUI Interface for network utilities
 * Copyright (C) 2003 by German Poo-Caaman~o
 *
 * This include part of GLIB:
 * GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997, 2002  Peter Mattis, Red Hat, Inc.
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

/* This is a backport of g_sprintf to allow compile gnome-netinfo
   on GNOME 2.0 */
 
#include <gnome.h>
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#if (GLIB_MINOR_VERSION < 2)

#  define _g_vsprintf  vsprintf

gint g_sprintf (gchar *str, gchar const *fmt, ...);

#endif /* GLIB_MINOR_VERSION */

GString * util_tree_model_to_string (GtkTreeView *treeview);
gchar * util_find_program_in_path (const gchar * program, const gchar * path);
gchar * util_find_program_dialog (gchar * program, GtkWidget *parent);
gchar * util_legible_bytes (gchar *bytes);
