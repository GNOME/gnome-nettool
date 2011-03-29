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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "utils.h"
#include <string.h>
#include <glib/gi18n.h>

#if (GLIB_MINOR_VERSION < 2)

gint
g_sprintf (gchar	*str,
	   gchar const *fmt,
	   ...)
{
  va_list args;
  gint retval;

  va_start (args, fmt);
  retval = g_vsprintf (str, fmt, args);
  va_end (args);
  
  return retval;
}
#endif /* GLIB_MINOR_VERSION */

/* Based on execvp() from GNU Libc.
 * Some of this code is cut-and-pasted into gspawn.c
 */
static gchar*
my_strchrnul (const gchar *str, gchar c)
{
  gchar *p = (gchar*)str;
  while (*p && (*p != c))
    ++p;

  return p;
}

/* Based on g_find_program_in_path, stolen from glib/gutils.c
 * It allow to ask in a specific PATH, if fail then as in the user PATH. 
 */
gchar *
util_find_program_in_path (const gchar * program, const gchar * path)
{
	const gchar *p;
	gchar *name, *freeme;
	size_t len;
	size_t pathlen;

	g_return_val_if_fail (program != NULL, NULL);

	/* If it is an absolute path, or a relative path including subdirectories,
	 * don't look in PATH.
	 */
	if (g_path_is_absolute (program)
	    || strchr (program, G_DIR_SEPARATOR) != NULL) {
		if (g_file_test (program, G_FILE_TEST_IS_EXECUTABLE))
			return g_strdup (program);
		else
			return NULL;
	}

	if (path == NULL) {
		path = g_getenv ("PATH");

		if (path == NULL) {
			path = "/bin:/usr/bin:.";
		}
	}
	
	len = strlen (program) + 1;
	pathlen = strlen (path);
	freeme = name = g_malloc (pathlen + len + 1);

	/* Copy the file name at the top, including '\0'  */
	memcpy (name + pathlen + 1, program, len);
	name = name + pathlen;
	/* And add the slash before the filename  */
	*name = G_DIR_SEPARATOR;

	p = path;
	do {
		char *startp;

		path = p;
		p = my_strchrnul (path, G_SEARCHPATH_SEPARATOR);

		if (p == path)
			/* Two adjacent colons, or a colon at the beginning or the end
			 * of `PATH' means to search the current directory.
			 */
			startp = name + 1;
		else
			startp =
			    memcpy (name - (p - path), path, p - path);

		if (g_file_test (startp, G_FILE_TEST_IS_EXECUTABLE)) {
			gchar *ret;
			ret = g_strdup (startp);
			g_free (freeme);
			return ret;
		}
	}
	while (*p++ != '\0');

	g_free (freeme);

	return NULL;
}

/* Process each row for a GtkTreeModel */
static gboolean
output_foreach (GtkTreeModel * model, GtkTreePath * path,
		     GtkTreeIter * iter, gpointer data)
{
	gint columns;
	gint j;
	GType type;
	GString *result = (GString *) data;

	columns = gtk_tree_model_get_n_columns (model);

	/* Get values from each colum */
	for (j = 0; j < columns; j++) {
		GValue value = { 0, };
		gchar *svalue;
		gint ivalue;

		type = gtk_tree_model_get_column_type (model, j);

		switch (type) {
		case G_TYPE_INT:
			gtk_tree_model_get_value (model, iter, j, &value);
			ivalue = g_value_get_int (&value);
			g_value_unset (&value);
			g_string_append_printf (result, "%d", ivalue);
			break;
		case G_TYPE_STRING:
			gtk_tree_model_get_value (model, iter, j, &value);
			svalue = g_value_dup_string (&value);
			g_value_unset (&value);
			g_string_append_printf (result, "%s", svalue);
			g_free (svalue);
			break;
		}
		if (j == columns - 1) {
			g_string_append_printf (result, "\n");
		} else {
			g_string_append_printf (result, "\t");
		}
	}
	return FALSE;
}

/* Get the model from a GtkTreeView and convert its content in
   a GString.  The columns are separated by '\t' and rows by '\n'.
   The string returned must be freed.
*/
GString *
util_tree_model_to_string (GtkTreeView *treeview)
{
	GtkTreeModel *model;
	GString *result;
	
	result = g_string_new ("");

	model = gtk_tree_view_get_model (treeview);

	if (!GTK_IS_TREE_MODEL (model)) {
		return result;
	}

	gtk_tree_model_foreach (model, output_foreach,
				(gpointer) result);

	return result;
}

#define EXTRA_PATH "/sbin:/usr/sbin:/usr/etc:/usr/local/bin:/usr/local/sbin:/usr/ucb"
/*
 * Find a program in PATH and EXTRA_PATH.  If fail, show a dialog box.
 * Returns the abosulet path and program.
 * The result must be freed.
 */
gchar *
util_find_program_dialog (gchar * program, GtkWidget *parent) {
	gchar *result;
	const gchar * path;
	gchar * my_path;

	path = g_getenv ("PATH");
	
	my_path = g_strconcat (path, ":", EXTRA_PATH, NULL);
	
	result = util_find_program_in_path (program, my_path);
	g_free (my_path);
	
	if (result == NULL && parent != NULL) {
		GtkWidget *dialog;
		
		dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
								  GTK_DIALOG_DESTROY_WITH_PARENT,
								  GTK_MESSAGE_WARNING,
								  GTK_BUTTONS_CLOSE,
								  _("In order to use this feature of the program, %s must be installed in your system"),
								  program);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);		
	}
	
	return result;
}

gchar *
util_legible_bytes (guint64 bytes)
{
	guint64 rx, short_rx;
	const gchar *unit = "B";
	gchar *result;
	
	rx = bytes;
	short_rx = rx * 10;  

	if (rx > 1125899906842624ull) {
	    short_rx /= 1125899906842624ull;
	    unit = "PiB";
	} else if (rx > 1099511627776ull) {
	    short_rx /= 1099511627776ull;
	    unit = "TiB";
	} else if (rx > 1073741824ull) {
	    short_rx /= 1073741824ull;
	    unit = "GiB";
	} else if (rx > 1048576) {
	    short_rx /= 1048576;
	    unit = "MiB";
	} else if (rx > 1024) {
	    short_rx /= 1024;
	    unit = "KiB";
	}
	
	result = g_strdup_printf ("%lld.%lld %s", short_rx / 10,
                                  short_rx % 10, unit);
	return result;	
}
