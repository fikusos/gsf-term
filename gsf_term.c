#include <gtk/gtk.h>
#include <vte/vte.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    THEME_CLASSIC,
    THEME_HOME,
    THEME_COLD,
    THEME_CUTE,
    THEME_LIGHT,
    NUM_THEMES
} Theme;

typedef struct {
    Theme current_theme;
    gdouble font_scale;
} Config;

Config config = { THEME_CLASSIC, 1.2 };
GtkWidget *notebook;

const gchar *config_dir = "/.config/gsf-term";
const gchar *config_file = "/config.ini";

gchar *get_config_path() {
    const gchar *home_dir = g_get_home_dir();
    gchar *config_path = g_build_filename(home_dir, config_dir, config_file, NULL);
    return config_path;
}

void save_config() {
    gchar *config_path = get_config_path();
    gchar *config_dir_path = g_build_filename(g_get_home_dir(), config_dir, NULL);

    if (g_mkdir_with_parents(config_dir_path, 0755) == -1) {
        g_printerr("Failed to create config directory.\n");
        return;
    }

    FILE *file = fopen(config_path, "w");
    if (file) {
        fprintf(file, "[GSF-Term]\n");
        fprintf(file, "theme=%d\n", config.current_theme);
        fprintf(file, "font_scale=%.2f\n", config.font_scale);
        fclose(file);
    } else {
        g_printerr("Failed to save config.\n");
    }

    g_free(config_path);
    g_free(config_dir_path);
}

void load_config() {
    gchar *config_path = get_config_path();
    FILE *file = fopen(config_path, "r");
    if (file) {
        char line[256];
        while (fgets(line, sizeof(line), file)) {
            if (strncmp(line, "theme=", 6) == 0) {
                config.current_theme = atoi(line + 6);
            } else if (strncmp(line, "font_scale=", 11) == 0) {
                config.font_scale = atof(line + 11);
            }
        }
        fclose(file);
    } else {
        g_printerr("Config not found, using defaults.\n");
    }

    g_free(config_path);
}

void apply_theme(Theme theme, VteTerminal *terminal) {
    GdkRGBA background, foreground, cursor;

    switch (theme) {
        case THEME_CLASSIC:
            background = (GdkRGBA){ 0.0, 0.0, 0.0, 1.0 };
            foreground = (GdkRGBA){ 1.0, 1.0, 1.0, 1.0 };
            cursor = (GdkRGBA){ 1.0, 1.0, 1.0, 1.0 };
            break;
        case THEME_HOME:
            background = (GdkRGBA){ 0.20, 0.15, 0.12, 1.0 };
            foreground = (GdkRGBA){ 0.96, 0.91, 0.84, 1.0 };
            cursor = (GdkRGBA){ 0.96, 0.91, 0.84, 1.0 };
            break;
        case THEME_COLD:
            background = (GdkRGBA){ 0.12, 0.14, 0.25, 1.0 };
            foreground = (GdkRGBA){ 0.67, 0.84, 0.90, 1.0 };
            cursor = (GdkRGBA){ 0.67, 0.84, 0.90, 1.0 };
            break;
        case THEME_CUTE:
            background = (GdkRGBA){ 0.35, 0.12, 0.25, 1.0 };
            foreground = (GdkRGBA){ 1.0, 0.89, 0.88, 1.0 };
            cursor = (GdkRGBA){ 1.0, 0.89, 0.88, 1.0 };
            break;
        case THEME_LIGHT:
            background = (GdkRGBA){ 1.0, 1.0, 1.0, 1.0 };
            foreground = (GdkRGBA){ 0.0, 0.0, 0.0, 1.0 };
            cursor = (GdkRGBA){ 0.0, 0.0, 0.0, 1.0 };
            break;
        default:
            return;
    }

    vte_terminal_set_color_background(terminal, &background);
    vte_terminal_set_color_foreground(terminal, &foreground);
    vte_terminal_set_color_cursor(terminal, &cursor);
}

void change_font_scale(VteTerminal *terminal, gboolean increase) {
    if (increase) {
        config.font_scale += 0.1;
    } else {
        config.font_scale -= 0.1;
    }
    vte_terminal_set_font_scale(terminal, config.font_scale);
    save_config();
}

gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    if ((event->state & GDK_CONTROL_MASK)) {
        VteTerminal *terminal = VTE_TERMINAL(gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook))));
        if (event->keyval == GDK_KEY_w) {
            config.current_theme = (config.current_theme + 1) % NUM_THEMES;
            apply_theme(config.current_theme, terminal);
            save_config();
            return TRUE;
        } else if (event->keyval == GDK_KEY_plus || event->keyval == GDK_KEY_equal) {
            change_font_scale(terminal, TRUE);
            return TRUE;
        } else if (event->keyval == GDK_KEY_minus) {
            change_font_scale(terminal, FALSE);
            return TRUE;
        }
    }
    return FALSE;
}

void close_tab(GtkButton *button, gpointer user_data) {
    GtkWidget *tab = GTK_WIDGET(user_data);
    gint page_num = gtk_notebook_page_num(GTK_NOTEBOOK(notebook), tab);
    if (page_num != -1) {
        gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), page_num);
    }
}

void create_tab() {
    GtkWidget *terminal = vte_terminal_new();
    GtkWidget *label = gtk_label_new("Terminal");

    GtkWidget *close_button = gtk_button_new_with_label("×");
    gtk_widget_set_tooltip_text(close_button, "Close tab");
    g_signal_connect(close_button, "clicked", G_CALLBACK(close_tab), terminal);

    GtkWidget *tab_label = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(tab_label), label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(tab_label), close_button, FALSE, FALSE, 0);
    gtk_widget_show_all(tab_label);

    vte_terminal_set_font_scale(VTE_TERMINAL(terminal), config.font_scale);
    vte_terminal_set_scrollback_lines(VTE_TERMINAL(terminal), 10000);

    char *shell = g_strdup("/bin/bash");
    char **envp = g_get_environ();
    char **command = (char *[]){ shell, NULL };
    vte_terminal_spawn_async(VTE_TERMINAL(terminal),
                             VTE_PTY_DEFAULT,
                             NULL,
                             command,
                             envp,
                             G_SPAWN_SEARCH_PATH,
                             NULL, NULL,
                             NULL, -1,
                             NULL, NULL, NULL);

    apply_theme(config.current_theme, VTE_TERMINAL(terminal));

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), terminal, tab_label);
    gtk_widget_show_all(terminal);

    g_free(shell);
    g_strfreev(envp);
}

GtkWidget *create_new_tab_button() {
    GtkWidget *button = gtk_button_new_with_label("+");
    gtk_widget_set_tooltip_text(button, "New tab");
    g_signal_connect(button, "clicked", G_CALLBACK(create_tab), NULL);
    return button;
}

int main(int argc, char *argv[]) {
    GtkWidget *window;

    gtk_init(&argc, &argv);

    load_config();

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "GSF-Term");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(window), notebook);

    GtkWidget *new_tab_button = create_new_tab_button();
    gtk_notebook_insert_page(GTK_NOTEBOOK(notebook), new_tab_button, gtk_label_new("+"), 0);

    create_tab();

    g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), NULL);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
