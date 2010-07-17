#include <glib.h>
#include <gtk/gtk.h>
typedef void* (* NetinfoActivateFn) (GtkWidget *widget, gpointer data);

void on_ping_activate (GtkWidget *editable, gpointer data);

void on_ping_toggled (GtkToggleButton *button, gpointer data);

void on_traceroute_activate (GtkWidget *editable, gpointer data);

void on_netstat_activate (GtkWidget *widget, gpointer data);

void on_info_nic_changed (GtkEntry *entry, gpointer output);

void on_scan_activate (GtkWidget *widget, gpointer data);

void on_lookup_activate (GtkWidget *editable, gpointer data);

void on_finger_activate (GtkWidget *editable, gpointer data);

void on_whois_activate (GtkWidget *editable, gpointer data);

void on_configure_button_clicked (GtkButton *widget, gpointer data);

/* General stuff */
gboolean gn_quit_app (GtkWidget * widget, gpointer data);

void on_beep_activate (GtkWidget *menu_item, gpointer data);

void on_copy_activate (gpointer notebook, GtkWidget *menu_item);

void on_clear_history_activate (gpointer notebook, GtkWidget *menu_item);

void on_page_switch (GtkNotebook     *notebook,
                     gpointer         page,
                     guint            page_num,
                     gpointer         data);

void on_about_activate (gpointer window, GtkWidget *menu_item);

void on_help_activate (gpointer window, GtkWidget *menu_item);
