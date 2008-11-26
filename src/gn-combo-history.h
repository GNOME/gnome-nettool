/* 
 * Copyright (C) 2004 by Carlos García Campos
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

#ifndef __GN_COMBO_HISTORY_H__
#define __GN_COMBO_HISTORY_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#define GN_TYPE_COMBO_HISTORY            (gn_combo_history_get_type ())
#define GN_COMBO_HISTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GN_TYPE_COMBO_HISTORY, GnComboHistory))
#define GN_COMBO_HISTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GN_TYPE_COMBO_HISTORY, GnComboHistoryClass))
#define GN_IS_COMBO_HISTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GN_TYPE_COMBO_HISTORY))
#define GN_IS_COMBO_HISTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GN_TYPE_COMBO_HISTORY))
#define GN_COMBO_HISTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GN_TYPE_COMBO_HISTORY, GnComboHistoryClass))

typedef struct _GnComboHistory            GnComboHistory;
typedef struct _GnComboHistoryClass       GnComboHistoryClass;
typedef struct _GnComboHistoryPrivate     GnComboHistoryPrivate;

struct _GnComboHistory
{
	   GObject parent;

	   GnComboHistoryPrivate *priv;
};

struct _GnComboHistoryClass
{
	   GObjectClass parent_class;
};

GType               gn_combo_history_get_type        (void);
GnComboHistory     *gn_combo_history_new             (void);

void                gn_combo_history_set_combo       (GnComboHistory *history,
										    GtkComboBox    *combo);
GtkComboBox        *gn_combo_history_get_combo       (GnComboHistory *history);
void                gn_combo_history_set_id          (GnComboHistory *history,
										    const gchar *history_id);
const gchar        *gn_combo_history_get_id          (GnComboHistory *history);
void                gn_combo_history_add             (GnComboHistory *history,
										    const gchar *text);
void                gn_combo_history_clear           (GnComboHistory *history);
guint               gn_combo_history_get_max_history (GnComboHistory *history);
void                gn_combo_history_set_max_history (GnComboHistory *history,
										    guint max_history);


#endif /* __GN_COMBO_HISTORY_H__ */
