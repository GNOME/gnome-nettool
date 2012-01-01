/* -*- Mode: C; indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2004 by Carlos Garc�a Campos
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

#include "gn-combo-history.h"

#define PARENT_TYPE G_TYPE_OBJECT

enum {
	PROP_0,
	PROP_COMBO,
	PROP_ID,
	PROP_MAX_HISTORY
};

struct _GnComboHistoryPrivate {
	GtkComboBox *combo;
	gchar       *id;
	guint        max_history;

	GSList       *items;

	GSettings   *settings;
};

static void     gn_combo_history_init         (GnComboHistory      *history);
static void     gn_combo_history_class_init   (GnComboHistoryClass *klass);
static void     gn_combo_history_finalize     (GObject             *object);

static void     gn_combo_history_set_property (GObject             *object,
					       guint                prop_id,
					       const GValue        *value,
					       GParamSpec          *pspec);
static void     gn_combo_history_get_property (GObject             *object,
					       guint                prop_id,
					       GValue              *value,
					       GParamSpec          *pspec);

static void     gn_combo_history_settings_register_id   (GnComboHistory *history);
static void     gn_combo_history_settings_load       (GnComboHistory *history);
static void     gn_combo_history_settings_save       (GnComboHistory *history);
static void     gn_combo_history_set_popdown_strings (GnComboHistory *history);

static GObjectClass *parent_class = NULL;

GType
gn_combo_history_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (GnComboHistoryClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gn_combo_history_class_init,
			NULL,
			NULL,
			sizeof (GnComboHistory),
			0,
			(GInstanceInitFunc) gn_combo_history_init
		};
		type = g_type_register_static (PARENT_TYPE, "GnComboHistory",
					       &info, 0);
	}
	return type;
}

static void
gn_combo_history_init (GnComboHistory *history)
{
	g_return_if_fail (GN_IS_COMBO_HISTORY (history));

	history->priv = g_new0 (GnComboHistoryPrivate, 1);
	history->priv->combo = NULL;
	history->priv->id = NULL;
	history->priv->max_history = 10;
	history->priv->items = NULL;
	history->priv->settings = g_settings_new ("org.gnome.gnome-nettool");
}

static void
gn_combo_history_class_init (GnComboHistoryClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->set_property = gn_combo_history_set_property;
	object_class->get_property = gn_combo_history_get_property;

	g_object_class_install_property (object_class, PROP_COMBO,
					 g_param_spec_pointer ("combo", NULL, NULL,
							       G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_ID,
					 g_param_spec_string ("id", NULL, NULL,
							      NULL, G_PARAM_READWRITE));
	g_object_class_install_property (object_class, PROP_MAX_HISTORY,
					 g_param_spec_uint ("max_history", NULL, NULL,
							    0, G_MAXUINT, 10,
							    G_PARAM_READWRITE));

	object_class->finalize = gn_combo_history_finalize;
}

static void
gn_combo_free_items (GnComboHistory *history)
{
	if (history->priv->items) {
		g_slist_foreach (history->priv->items, (GFunc) g_free, NULL);
		g_slist_free (history->priv->items);
		history->priv->items = NULL;
	}
}

static void
gn_combo_history_finalize (GObject *object)
{
	GnComboHistory *history = GN_COMBO_HISTORY (object);

	if (history->priv) {
		if (history->priv->combo) {
			g_object_unref (G_OBJECT (history->priv->combo));
			history->priv->combo = NULL;
		}

		if (history->priv->id) {
			g_free (history->priv->id);
			history->priv->id = NULL;
		}

		gn_combo_free_items (history);

		g_object_unref (G_OBJECT (history->priv->settings));
		history->priv->settings = NULL;
	}

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
gn_combo_history_set_property (GObject  *object, guint prop_id, const GValue *value,
			       GParamSpec *spec)
{
	GnComboHistory *history;

	g_return_if_fail (GN_IS_COMBO_HISTORY (object));

	history = GN_COMBO_HISTORY (object);

	switch (prop_id) {
	case PROP_COMBO:
		history->priv->combo = g_value_get_pointer (value);
		break;
	case PROP_ID:
		if (history->priv->id) g_free (history->priv->id);
		history->priv->id = g_value_dup_string (value);
		break;
	case PROP_MAX_HISTORY:
		history->priv->max_history = g_value_get_uint (value);
		break;
	default:
		break;
	}
}

static void
gn_combo_history_get_property (GObject  *object, guint prop_id, GValue *value,
			       GParamSpec *spec)
{
	GnComboHistory *history;

	g_return_if_fail (GN_IS_COMBO_HISTORY (object));

	history = GN_COMBO_HISTORY (object);

	switch (prop_id) {
	case PROP_COMBO:
		g_value_set_pointer (value, history->priv->combo);
		break;
	case PROP_ID:
		g_value_set_string (value, history->priv->id);
		break;
	case PROP_MAX_HISTORY:
		g_value_set_uint (value, history->priv->max_history);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, spec);
	}
}

GnComboHistory *
gn_combo_history_new (void)
{
	GnComboHistory *history;

	history = g_object_new (GN_TYPE_COMBO_HISTORY, NULL);

	return (history);
}

static void
gn_combo_history_settings_load (GnComboHistory *history)
{
	gchar **items;
	guint i;

	g_return_if_fail (GN_IS_COMBO_HISTORY (history));
	g_return_if_fail (history->priv->id != NULL);

	gn_combo_free_items (history);
	history->priv->items = NULL;

	items = g_settings_get_strv (history->priv->settings, history->priv->id);

	for (i = 0; items[i] && i < history->priv->max_history; i++)
		history->priv->items = g_slist_append (history->priv->items, g_strdup (items[i]));
	g_strfreev (items);
}

static void
gn_combo_history_settings_save (GnComboHistory *history)
{
	const gchar **items;
	GSList *item;
	gint i;

	g_return_if_fail (GN_IS_COMBO_HISTORY (history));
	g_return_if_fail (history->priv->id != NULL);

	items = g_malloc (sizeof (gchar *) * (g_slist_length (history->priv->items) + 1));
	for (item = history->priv->items, i = 0; item; item = item->next, i++)
		items[i] = item->data;
	items[i] = NULL;
	g_settings_set_strv (history->priv->settings, history->priv->id, items);
	g_free (items);
}

static void
gn_combo_history_set_popdown_strings (GnComboHistory *history)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GSList *items;
	gchar  *text;
	gint    text_column, i;

	g_return_if_fail (GN_IS_COMBO_HISTORY (history));
	g_return_if_fail (GTK_IS_COMBO_BOX (history->priv->combo));

	model = gtk_combo_box_get_model (history->priv->combo);

	if (!model)
		return;

	text_column = gtk_combo_box_get_entry_text_column (
							   GTK_COMBO_BOX (history->priv->combo));

	gtk_list_store_clear (GTK_LIST_STORE (model));
	   
	if (! history->priv->items) {
             
		gtk_combo_box_set_active (GTK_COMBO_BOX (history->priv->combo), -1);
			 
		return;
	}

	i = 0;
	for (items = history->priv->items; items; items = items->next) {
		text = items->data;

		gtk_list_store_insert (GTK_LIST_STORE (model), &iter, i);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    text_column, text,
				    -1);

		i ++;
	}

	/* At this point the current selection always be at the first place in the model */
	gtk_combo_box_set_active (GTK_COMBO_BOX (history->priv->combo), 0);
}

void
gn_combo_history_set_combo (GnComboHistory *history, GtkComboBox *combo)
{   
	g_return_if_fail (GN_IS_COMBO_HISTORY (history));
	g_return_if_fail (GTK_IS_COMBO_BOX (combo));

	if (history->priv->combo)
		g_object_unref (G_OBJECT (history->priv->combo));

	history->priv->combo = combo;

	gn_combo_history_settings_load (history);
	   
	gn_combo_history_set_popdown_strings (history);

	gtk_combo_box_set_active (GTK_COMBO_BOX (history->priv->combo),
				  -1);

	gtk_entry_set_text (
			    GTK_ENTRY (gtk_bin_get_child (
							  GTK_BIN (history->priv->combo))), "");

	g_object_ref (G_OBJECT (history->priv->combo));
}

GtkComboBox *
gn_combo_history_get_combo (GnComboHistory *history)
{
	g_return_val_if_fail (GN_IS_COMBO_HISTORY (history), NULL);

	return history->priv->combo;
}

static void
gn_on_settings_history_changed (GSettings *settings, gchar *key, gpointer gdata)
{
	GnComboHistory *history;

	history = GN_COMBO_HISTORY (gdata);

	if (!g_str_equal (key, history->priv->id))
		return;

	gn_combo_history_settings_load (history);
	gn_combo_history_set_popdown_strings (history);
}

static void
gn_combo_history_settings_register_id (GnComboHistory *history)
{
	g_return_if_fail (GN_IS_COMBO_HISTORY (history));
	g_signal_connect (history->priv->settings, "changed", G_CALLBACK (gn_on_settings_history_changed), history);
}

void
gn_combo_history_set_id (GnComboHistory *history, const gchar *history_id)
{
	g_return_if_fail (GN_IS_COMBO_HISTORY (history));
	g_return_if_fail (history_id != NULL);

	if (history->priv->id)
		g_free (history->priv->id);

	history->priv->id = g_strdup (history_id);

	gn_combo_history_settings_register_id (history);
}

const gchar *
gn_combo_history_get_id (GnComboHistory *history)
{
	g_return_val_if_fail (GN_IS_COMBO_HISTORY (history), NULL);

	return history->priv->id;
}

static gint
compare (gconstpointer a, gconstpointer b)
{
	return (g_ascii_strcasecmp (a, b));
}

void
gn_combo_history_add (GnComboHistory *history, const gchar *text)
{
	GSList *item;

	g_return_if_fail (GN_IS_COMBO_HISTORY (history));
	g_return_if_fail (text != NULL);

	if ((item = g_slist_find_custom (history->priv->items, (gpointer) text, compare))) {
		/* item is already in list, remove them */
		history->priv->items = g_slist_remove (history->priv->items, item->data);
	}

	if (g_slist_length (history->priv->items) >= history->priv->max_history) {
		item = g_slist_last (history->priv->items);
		history->priv->items = g_slist_remove (history->priv->items, item->data);
	}

	history->priv->items = g_slist_prepend (history->priv->items,
						g_strdup (text));

	gn_combo_history_set_popdown_strings (history);
	   
	gn_combo_history_settings_save (history);
}

void
gn_combo_history_clear (GnComboHistory *history)
{
	g_return_if_fail (GN_IS_COMBO_HISTORY (history));

	if (history->priv->items) {
		gn_combo_free_items (history);
		gn_combo_history_settings_save (history);
	}
}

guint
gn_combo_history_get_max_history (GnComboHistory *history)
{
	g_return_val_if_fail (GN_IS_COMBO_HISTORY (history), 0);

	return history->priv->max_history;
}

void
gn_combo_history_set_max_history (GnComboHistory *history,
				  guint max_history)
{
	g_return_if_fail (GN_IS_COMBO_HISTORY (history));

	history->priv->max_history = max_history;
}
