#include <gnome.h>

typedef void* (* NetinfoActivateFn) (GtkWidget *widget, gpointer data);

void on_ping_activate (GtkWidget * editable, gpointer data);

void on_traceroute_activate (GtkWidget * editable, gpointer data);

void on_netstat_activate (GtkWidget * widget, gpointer data);

void on_info_nic_changed (GtkEntry *entry, gpointer output);

void on_scan_activate (GtkWidget * widget, gpointer data);

void on_lookup_activate (GtkWidget * editable, gpointer data);

void on_finger_activate (GtkWidget * editable, gpointer data);

void on_whois_activate (GtkWidget * editable, gpointer data);

/* General stuff */
void gn_quit_app (GtkWidget * widget, gpointer data);

void on_about_activate (GtkWidget *menu_item, gpointer data);

void on_copy_activate (GtkWidget * notebook, gpointer data);

void on_clear_history_activate (GtkWidget * notebook, gpointer data);

void on_page_switch (GtkNotebook     * notebook,
                     GtkNotebookPage * page,
                     guint             page_num,
                     gpointer          data);
