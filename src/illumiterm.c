/* Copyright 2023 Elijah Gordon (SLcK) <braindisassemblue@gmail.com>

*  This program is free software; you can redistribute it and/or
*  modify it under the terms of the GNU General Public License
*  as published by the Free Software Foundation; either version 2
*  of the License, or (at your option) any later version.

*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.

*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <vte/vte.h>
#include <gtk/gtk.h>

static const gchar* get_new_window_title(VteTerminal* terminal) {
    return vte_terminal_get_window_title(terminal);
}

static void set_window_title(GtkWidget* window, const gchar* new_title) {
    gtk_window_set_title(GTK_WINDOW(window), new_title);
}

static void window_title_changed(GtkWidget* widget, gpointer window) {
    const gchar* new_title = get_new_window_title(VTE_TERMINAL(widget));
    set_window_title(window, new_title);
}

static void set_exit_status(GApplicationCommandLine* cli, gint status) {
    if (cli) {
        g_application_command_line_set_exit_status(cli, status);
        g_object_unref(cli);
    }
}

static void destroy_window(GtkWidget* widget, gpointer data) {
    gtk_widget_destroy(widget);
}

static void destroy_and_quit(GtkWidget* window, gint status) {
    GApplicationCommandLine* cli = g_object_get_data(G_OBJECT(window), "cli");
    set_exit_status(cli, status);
    destroy_window(window, NULL);
}

static void handle_child_exit(GtkWidget* window, gint status) {
    destroy_and_quit(window, status);
}

static gboolean child_exited(VteTerminal* term, gint status, gpointer data) {
    GtkWidget* window = data;
    handle_child_exit(window, status);
    return TRUE;
}

static void get_terminal_dimensions(VteTerminal *terminal, glong *rows, glong *columns, glong *char_width, glong *char_height) {
    *rows = vte_terminal_get_row_count(terminal);
    *columns = vte_terminal_get_column_count(terminal);
    *char_width = vte_terminal_get_char_width(terminal);
    *char_height = vte_terminal_get_char_height(terminal);
}

static void get_container_dimensions(GtkWidget *widget, GtkWindow *window, gint *owidth, gint *oheight, glong char_width, glong char_height, glong columns, glong rows) {
    gtk_window_get_size(window, owidth, oheight);
    *owidth -= char_width * columns;
    *oheight -= char_height * rows;
}

static void adjust_font_scale(VteTerminal *terminal, gdouble factor) {
    gdouble scale = vte_terminal_get_font_scale(terminal);
    vte_terminal_set_font_scale(terminal, scale * factor);
}

static void adjust_terminal_size(GtkWindow *window, glong rows, glong columns, glong char_width, glong char_height, gint owidth, gint oheight) {
    gtk_window_resize(window, columns * char_width + owidth, rows * char_height + oheight);
}

static void adjust_font_size(GtkWidget *widget, GtkWindow *window, gdouble factor)
{
    VteTerminal *terminal = VTE_TERMINAL(widget);
    glong rows, columns, char_width, char_height;
    get_terminal_dimensions(terminal, &rows, &columns, &char_width, &char_height);
    gint owidth, oheight;
    get_container_dimensions(widget, window, &owidth, &oheight, char_width, char_height, columns, rows);
    adjust_font_scale(terminal, factor);
    get_terminal_dimensions(terminal, &rows, &columns, &char_width, &char_height);
    adjust_terminal_size(window, rows, columns, char_width, char_height, owidth, oheight);
}

static void increase_font_size(GtkWidget *widget, gpointer window)
{
    adjust_font_size(widget, GTK_WINDOW(window), 1.125);
}

static void decrease_font_size(GtkWidget *widget, gpointer window)
{
    adjust_font_size(widget, GTK_WINDOW(window), 1.0 / 1.125);
}

static void reset_font_scale(VteTerminal *terminal)
{
    vte_terminal_set_font_scale(terminal, 1.0);
}

static void reset_font_description_size(VteTerminal *terminal, gint size)
{
    PangoFontDescription *font_desc = pango_font_description_copy(vte_terminal_get_font(terminal));
    pango_font_description_set_size(font_desc, size);
    vte_terminal_set_font(terminal, font_desc);
    pango_font_description_free(font_desc);
}

static void resize_terminal_window(GtkWindow *window, VteTerminal *terminal, gint char_width, gint char_height)
{
    gint rows = vte_terminal_get_row_count(terminal);
    gint columns = vte_terminal_get_column_count(terminal);
    gint owidth, oheight;
    gtk_window_get_size(window, &owidth, &oheight);
    owidth -= char_width * columns;
    oheight -= char_height * rows;
    gtk_window_resize(window, columns * char_width + owidth, rows * char_height + oheight);
}

static void reset_font_size(GtkWidget *widget, GtkWindow *window)
{
    VteTerminal *terminal = VTE_TERMINAL(widget);
    gint default_font_size = pango_font_description_get_size(vte_terminal_get_font(terminal));

    reset_font_scale(terminal);
    reset_font_description_size(terminal, default_font_size);
    resize_terminal_window(window, terminal, vte_terminal_get_char_width(terminal), vte_terminal_get_char_height(terminal));
}

static void reset_window_size(GtkWidget *widget, GtkWindow *window)
{
    gint default_width = 640;
    gint default_height = 460;
    gtk_window_resize(window, default_width, default_height);
}

static gboolean key_press_event(GtkWidget *widget, GdkEvent *event, gpointer window)
{
    GdkModifierType modifiers = gtk_accelerator_get_default_mod_mask();
    g_assert(event->type == GDK_KEY_PRESS);

    if ((event->key.state & modifiers) == (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) {
        switch (event->key.hardware_keycode) {
            case 21:
                increase_font_size(widget, window);
                return TRUE;
            case 20:
                decrease_font_size(widget, window);
                return TRUE;
            case 19:
                reset_font_size(widget, window);
                reset_window_size(widget, window);
                return TRUE;
        }
        switch (gdk_keyval_to_lower(event->key.keyval)) {
            case GDK_KEY_c:
                vte_terminal_copy_clipboard_format((VteTerminal *)widget, VTE_FORMAT_TEXT);
                return TRUE;
            case GDK_KEY_v:
                vte_terminal_paste_clipboard((VteTerminal *)widget);
                return TRUE;
        }
    }

    return FALSE;
}

static GtkWidget* create_dialog_buttons() {
    GtkWidget* buttons_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_set_spacing(GTK_BOX(buttons_box), 10);

    GtkWidget* no_button = gtk_button_new_with_label("No");
    GtkWidget* yes_button = gtk_button_new_with_label("Yes");

    gtk_container_add(GTK_CONTAINER(buttons_box), no_button);
    gtk_container_add(GTK_CONTAINER(buttons_box), yes_button);

    return buttons_box;
}

static GtkWidget* create_dialog_message() {
    GtkWidget* message_label = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(message_label), 0);
    gtk_label_set_yalign(GTK_LABEL(message_label), 0);
    gtk_label_set_line_wrap(GTK_LABEL(message_label), TRUE);
    gtk_label_set_line_wrap_mode(GTK_LABEL(message_label), PANGO_WRAP_WORD_CHAR);
    gtk_label_set_selectable(GTK_LABEL(message_label), FALSE);
    gtk_label_set_justify(GTK_LABEL(message_label), GTK_JUSTIFY_CENTER);

    return message_label;
}

static GtkWidget* create_confirm_dialog() {
    GtkWidget* dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "Confirm close");
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_deletable(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_decorated(GTK_WINDOW(dialog), TRUE);

    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 20);

    GtkWidget* message_label = create_dialog_message();
    gtk_container_add(GTK_CONTAINER(content_area), message_label);

    GtkWidget* buttons_box = create_dialog_buttons();
    gtk_box_pack_end(GTK_BOX(content_area), buttons_box, FALSE, FALSE, 0);

    gtk_dialog_add_button(GTK_DIALOG(dialog), "No", GTK_RESPONSE_NO);
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Yes", GTK_RESPONSE_YES);

    return dialog;
}

static void destroy_confirm_dialog(GtkWidget* dialog) {
    gtk_widget_destroy(dialog);
}

static gboolean get_confirm_response(GtkWidget* dialog) {
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    destroy_confirm_dialog(dialog);
    return (response == GTK_RESPONSE_NO) ? TRUE : FALSE;
}

static gboolean confirm_exit(GtkWidget* widget, GdkEvent* event, gpointer data) {
    GtkWidget* dialog = create_confirm_dialog();
    return get_confirm_response(dialog);
}

static GtkWidget* create_context_menu() {
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *new_window = gtk_menu_item_new_with_label("New Window");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), new_window);

    GtkWidget *new_tab = gtk_menu_item_new_with_label("New Tab");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), new_tab);

    GtkWidget *separator0 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator0);

    GtkWidget *copy_item = gtk_menu_item_new_with_label("Copy");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), copy_item);

    GtkWidget *paste_item = gtk_menu_item_new_with_label("Paste");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), paste_item);

    GtkWidget *separator1 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator1);

    GtkWidget *clear_scrollback = gtk_menu_item_new_with_label("Clear Scrollback");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), clear_scrollback);

    GtkWidget *separator2 = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator2);

    GtkWidget *preferences = gtk_menu_item_new_with_label("Preferences");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), preferences);

    GtkWidget *name_tab = gtk_menu_item_new_with_label("Name Tab");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), name_tab);

    GtkWidget *previous_tab = gtk_menu_item_new_with_label("Previous Tab");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), previous_tab);

    GtkWidget *next_tab = gtk_menu_item_new_with_label("Next Tab");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), next_tab);

    GtkWidget *move_tab_left = gtk_menu_item_new_with_label("Move Tab Left");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), move_tab_left);

    GtkWidget *move_tab_right = gtk_menu_item_new_with_label("Move Tab Right");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), move_tab_right);

    GtkWidget *close_tab = gtk_menu_item_new_with_label("Close Tab");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), close_tab);

    gtk_widget_show_all(menu);

    return menu;
}

static gboolean button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    if (event->button != GDK_BUTTON_SECONDARY) {
        return FALSE;
    }

    GtkWidget *menu = create_context_menu();
    gtk_menu_popup_at_pointer(GTK_MENU(menu), NULL);    
    return TRUE;
}

static gchar** get_environment(GApplicationCommandLine* cli) {
    const gchar* const* environment = g_application_command_line_get_environ(cli);
    guint num = g_strv_length((gchar**) environment);
    gchar** result = g_new(gchar*, num + 1);
    
    for (guint i = 0; i < num; ++i) {
        result[i] = g_strdup(environment[i]);
    }
    
    result[num] = NULL;
    return result;
}

static void child_ready(VteTerminal* terminal, GPid pid, GError* error, gpointer user_data) {
    if (!terminal) {
        return;
    }

    if (pid == 0) {
        GtkWidget* window = GTK_WIDGET(user_data);
        destroy_and_quit(window, error->code);
    }
}

static void connect_child_exited_signal(GtkWidget* widget, GtkWidget* window) {
    g_signal_connect(widget, "child-exited", G_CALLBACK(child_exited), window);
}

static void connect_key_press_event_signal(GtkWidget* widget, GtkWidget* window) {
    g_signal_connect(widget, "key-press-event", G_CALLBACK(key_press_event), window);
}

static void connect_window_title_changed_signal(GtkWidget* widget, GtkWidget* window) {
    g_signal_connect(widget, "window-title-changed", G_CALLBACK(window_title_changed), window);
}

static void connect_button_press_event_signal(GtkWidget* widget) {
    g_signal_connect(widget, "button_press_event", G_CALLBACK(button_press_event), NULL);
}

static void connect_delete_event_signal(GtkWidget* window) {
    g_signal_connect(window, "delete-event", G_CALLBACK(confirm_exit), NULL);
}

static void connect_vte_signals(GtkWidget* widget, GtkWidget* window) {
    connect_child_exited_signal(widget, window);
    connect_key_press_event_signal(widget, window);
    connect_window_title_changed_signal(widget, window);
    connect_button_press_event_signal(widget);
    connect_delete_event_signal(window);
}

static void spawn_vte_terminal(GApplicationCommandLine *cli, GtkWidget *window, GtkWidget *widget) {
    GVariantDict *options = g_application_command_line_get_options_dict(cli);
    
    const gchar *command = NULL;
    g_variant_dict_lookup(options, "cmd", "&s", &command);
    
    gchar **environment = get_environment(cli);
    gchar **cmd;
    gchar *cmdline = NULL;
    cmd = command ?
        (gchar *[]){"/bin/sh", cmdline = g_strdup(command), NULL} :
        (gchar *[]){cmdline = g_strdup(g_application_command_line_getenv(cli, "SHELL")), NULL};
    
    connect_vte_signals(widget, window);
    vte_terminal_set_word_char_exceptions(VTE_TERMINAL(widget), "-./?%&_=+@~:");
    vte_terminal_set_scrollback_lines(VTE_TERMINAL(widget), -1);
    vte_terminal_set_scroll_on_output(VTE_TERMINAL(widget), TRUE);
    vte_terminal_set_scroll_on_keystroke(VTE_TERMINAL(widget), TRUE);
    vte_terminal_set_mouse_autohide(VTE_TERMINAL(widget), TRUE);
    vte_terminal_set_bold_is_bright(VTE_TERMINAL(widget), TRUE);
    vte_terminal_set_audible_bell(VTE_TERMINAL(widget), TRUE);
    vte_terminal_set_cursor_blink_mode(VTE_TERMINAL(widget), TRUE);
    
    vte_terminal_spawn_async(VTE_TERMINAL(widget),
        VTE_PTY_DEFAULT,
        g_application_command_line_get_cwd(cli), 
        cmd,
        environment,    
        0,      
        NULL,
        NULL,
        NULL,   
        -1,     
        NULL,       
        child_ready,    
        window);        
    
    g_strfreev(environment);
    g_free(cmdline);
}

void on_new_window_activate(GtkMenuItem *menuitem, gpointer user_data) {
    g_print("on_new_window_activate\n");
}

void on_new_tab_activate(GtkMenuItem *menuitem, gpointer user_data) {
    g_print("on_new_tab_activate\n");
}

void on_close_tab_activate(GtkMenuItem *menuitem, gpointer user_data) {
    g_print("on_close_activate\n");
}

void on_close_window_activate(GtkMenuItem *menuitem, gpointer user_data) {
    exit(0);
}

static GtkWidget* create_file_menu() {
    GtkWidget *file_menu = gtk_menu_new();
    char label[50];
    
    g_snprintf(label, sizeof(label), "%-20s %20s", "New Window", "Shift+Ctrl+N");
    GtkWidget *new_window = gtk_menu_item_new_with_label(label);
    g_signal_connect(new_window, "activate", G_CALLBACK(on_new_window_activate), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), new_window);
    
    g_snprintf(label, sizeof(label), "%-20s %27s", "New Tab", "Shift+Ctrl+T");
    GtkWidget *new_tab = gtk_menu_item_new_with_label(label);
    g_signal_connect(new_tab, "activate", G_CALLBACK(on_new_tab_activate), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), new_tab);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), gtk_separator_menu_item_new());
    
    g_snprintf(label, sizeof(label), "%-20s %27s", "Close Tab", "Shift+Ctrl+W");
    GtkWidget *close_tab = gtk_menu_item_new_with_label(label);
    g_signal_connect(close_tab, "activate", G_CALLBACK(on_close_tab_activate), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), close_tab);
    
    g_snprintf(label, sizeof(label), "%-20s %20s", "Close Window", "Shift+Ctrl+Q");
    GtkWidget *close_window = gtk_menu_item_new_with_label(label);
    g_signal_connect(close_window, "activate", G_CALLBACK(on_close_window_activate), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_menu), close_window);

    return file_menu;
}

static void on_copy_activate(GtkMenuItem *menuitem, gpointer user_data) {
    g_print("on_copy_activate\n");
}

static void on_paste_activate(GtkMenuItem *menuitem, gpointer user_data) {
    g_print("on_paste_activate\n");
}

static void on_clear_scrollback_activate(GtkMenuItem *menuitem, gpointer user_data) {
    g_print("on_clear_scrollback_activate\n");
}

static void on_zoom_in_activate(GtkMenuItem *menuitem, gpointer user_data) {
    g_print("on_zoom_in_activate\n");
}

static void on_zoom_out_activate(GtkMenuItem *menuitem, gpointer user_data) {
    g_print("on_zoom_out_activate\n");
}

static void on_zoom_reset_activate(GtkMenuItem *menuitem, gpointer user_data) {
    g_print("on_zoom_reset_activate\n");
}

static void on_preferences_activate(GtkMenuItem *menuitem, gpointer user_data) {
    g_print("on_preferences_activate\n");
}

static GtkWidget* create_edit_menu() {
    GtkWidget *edit_menu = gtk_menu_new();
    char label[50];
    
    g_snprintf(label, sizeof(label), "%-20s %22s", "Copy", "Shift+Ctrl+C");
    GtkWidget *copy_item = gtk_menu_item_new_with_label(label);
    g_signal_connect(copy_item, "activate", G_CALLBACK(on_copy_activate), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), copy_item);

    g_snprintf(label, sizeof(label), "%-20s %21s", "Paste", "Shift+Ctrl+V");
    GtkWidget *paste_item = gtk_menu_item_new_with_label(label);
    g_signal_connect(paste_item, "activate", G_CALLBACK(on_paste_activate), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), paste_item);

    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), gtk_separator_menu_item_new());

    GtkWidget *clear_scrollback = gtk_menu_item_new_with_label("Clear Scrollback");
    g_signal_connect(clear_scrollback, "activate", G_CALLBACK(on_clear_scrollback_activate), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), clear_scrollback);

    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), gtk_separator_menu_item_new());

    g_snprintf(label, sizeof(label), "%-20s %18s", "Zoom In", "Shift+Ctrl++");
    GtkWidget *zoom_in = gtk_menu_item_new_with_label(label);
    g_signal_connect(zoom_in, "activate", G_CALLBACK(on_zoom_in_activate), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), zoom_in);

    g_snprintf(label, sizeof(label), "%-20s %15s", "Zoom Out", "Shift+Ctrl+_");
    GtkWidget *zoom_out = gtk_menu_item_new_with_label(label);
    g_signal_connect(zoom_out, "activate", G_CALLBACK(on_zoom_out_activate), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), zoom_out);

    g_snprintf(label, sizeof(label), "%-20s %13s", "Zoom Reset", "Shift+Ctrl+)");
    GtkWidget *zoom_reset = gtk_menu_item_new_with_label(label);
    g_signal_connect(zoom_reset, "activate", G_CALLBACK(on_zoom_reset_activate), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), zoom_reset);

    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), gtk_separator_menu_item_new());

    GtkWidget *preferences = gtk_menu_item_new_with_label("Preferences");
    g_signal_connect(preferences, "activate", G_CALLBACK(on_preferences_activate), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(edit_menu), preferences);
    
    return edit_menu;
}

void on_name_tab_activate(GtkMenuItem *menuitem, gpointer user_data) {
    g_print("on_name_tab_activate\n");
}

void on_previous_tab_activate(GtkMenuItem *menuitem, gpointer user_data) {
    g_print("on_previous_tab_activate\n");
}

void on_next_tab_activate(GtkMenuItem *menuitem, gpointer user_data) {
    g_print("on_next_tab_activate\n");
}

void on_move_tab_left_activate(GtkMenuItem *menuitem, gpointer user_data) {
    g_print("on_move_tab_left_activate\n");
}

void on_move_tab_right_activate(GtkMenuItem *menuitem, gpointer user_data) {
    g_print("on_move_tab_right_activate\n");
}

static GtkWidget* create_tabs_menu() {
    GtkWidget *tabs_menu = gtk_menu_new();
    char label[50];
    
    g_snprintf(label, sizeof(label), "%-20s %18s", "Name Tab", "Shift+Ctrl+I");
    GtkWidget *name_tab = gtk_menu_item_new_with_label(label);
    g_signal_connect(name_tab, "activate", G_CALLBACK(on_name_tab_activate), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tabs_menu), name_tab);

    gtk_menu_shell_append(GTK_MENU_SHELL(tabs_menu), gtk_separator_menu_item_new());

    g_snprintf(label, sizeof(label), "%-20s %16s", "Previous Tab", "Ctrl+Page Up");
    GtkWidget *previous_tab = gtk_menu_item_new_with_label(label);
    g_signal_connect(previous_tab, "activate", G_CALLBACK(on_previous_tab_activate), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tabs_menu), previous_tab);

    g_snprintf(label, sizeof(label), "%-20s %23s", "Next Tab", "Ctrl+Page Down");
    GtkWidget *next_tab = gtk_menu_item_new_with_label(label);
    g_signal_connect(next_tab, "activate", G_CALLBACK(on_next_tab_activate), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tabs_menu), next_tab);

    gtk_menu_shell_append(GTK_MENU_SHELL(tabs_menu), gtk_separator_menu_item_new());

    g_snprintf(label, sizeof(label), "%-20s %21s", "Move Tab Left", "Shift+Ctrl+Page Up");
    GtkWidget *move_tab_left = gtk_menu_item_new_with_label(label);
    g_signal_connect(move_tab_left, "activate", G_CALLBACK(on_move_tab_left_activate), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tabs_menu), move_tab_left);

    g_snprintf(label, sizeof(label), "%-20s %21s", "Move Tab Right", "Shift+Ctrl+Page Down");
    GtkWidget *move_tab_right = gtk_menu_item_new_with_label(label);
    g_signal_connect(move_tab_right, "activate", G_CALLBACK(on_move_tab_right_activate), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(tabs_menu), move_tab_right);
    
    return tabs_menu;
}

void create_about_window(GtkWindow *parent) {
    GtkWidget *about_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    gtk_window_set_title(GTK_WINDOW(about_window), "About IllumiTerm");
    gtk_window_set_modal(GTK_WINDOW(about_window), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(about_window), GTK_WINDOW(parent));
    gtk_window_set_resizable(GTK_WINDOW(about_window), FALSE);

    GtkWidget *header = gtk_header_bar_new();
    gtk_header_bar_set_title(GTK_HEADER_BAR(header), "About IllumiTerm");
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR(header), "Version 1.0");
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), TRUE);
    gtk_window_set_titlebar(GTK_WINDOW(about_window), header);

    GtkWidget *notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(about_window), notebook);

    GtkWidget *about_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *logo = gtk_image_new_from_file("/usr/share/icons/hicolor/96x96/apps/about.png");
    gtk_box_pack_start(GTK_BOX(about_tab), logo, FALSE, FALSE, 0);
    GtkWidget *label = gtk_label_new(NULL);
    const char* label_text = "<big><b>IllumiTerm</b></big>";
    GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(about_tab), separator, FALSE, FALSE, 0);
    gtk_label_set_markup(GTK_LABEL(label), label_text);
    gtk_box_pack_start(GTK_BOX(about_tab), label, FALSE, FALSE, 0);
    GtkWidget *description = gtk_label_new(NULL);
    const char *mark_up_text = "<b>Programming has always fascinated me, and I have always been interested\n"
	                       "in learning new languages and exploring different software development tools.\n"
      	                       "Recently, I decided to take my programming skills to the next level by learning C\n"
		               "and creating my own custom terminal. It was a challenging yet rewarding experience\n"
		               "that helped me develop my programming skills in many ways.\n"
		               "\n"
		               "My journey started with the decision to learn C. I had heard a lot about\n"
		               "the language's speed, efficiency, and low-level programming capabilities,\n"
		               "and I was excited to explore it. I started with the basics, such as data types,\n"
		               "operators, and control statements, and gradually moved on to more advanced topics,\n"
		               "such as pointers, structures, and file handling.</b>";

							   
    gtk_label_set_markup(GTK_LABEL(description), mark_up_text);

    gtk_label_set_justify(GTK_LABEL(description), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(about_tab), description, TRUE, TRUE, 0);
    GtkWidget *separator1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(about_tab), separator1, FALSE, FALSE, 0);
    GtkWidget *link_button = gtk_link_button_new_with_label("https://illumiterm.blogspot.com", "Visit Website");
    gtk_box_pack_start(GTK_BOX(about_tab), link_button, FALSE, FALSE, 0);
    
    const char* about_label_tab = "<b>About</b>";
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), about_tab, gtk_label_new(NULL));
    gtk_label_set_markup(GTK_LABEL(gtk_notebook_get_tab_label(GTK_NOTEBOOK(notebook), about_tab)), about_label_tab);

    GtkWidget *credits_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    GtkWidget *logo1 = gtk_image_new_from_file("/usr/share/icons/hicolor/96x96/apps/about.png");
    gtk_box_pack_start(GTK_BOX(credits_tab), logo1, FALSE, FALSE, 0);

    GtkWidget *credits_label = gtk_label_new(NULL);
    GtkWidget *separator2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(credits_tab), separator2, FALSE, FALSE, 0);
    const char* credits_label_text = "<big><b>Credits</b></big>";
    gtk_label_set_markup(GTK_LABEL(credits_label), credits_label_text);
    gtk_box_pack_start(GTK_BOX(credits_tab), credits_label, TRUE, TRUE, 0);

    GtkWidget *credits_list = gtk_label_new("<b>Dear GTK and VTE developers,</b>\n\n"
		                            "<b>Your attention to detail and commitment to open-source principles have made a\n"
		                            "significant impact on the software development community as a whole.\n\n"
		                            "Your contributions have helped countless developers around the world\n"
		                            "to create high-quality, reliable applications that are accessible to everyone.</b>\n\n"
		                            "<b>Thank you for all that you do. Your work is greatly appreciated and will continue to\n"
		                            "make a positive difference in the world of software development for years to come.</b>\n\n"
		                            "<b>Sincerely,</b>\n"
		                            "<b>Elijah Gordon</b>");

    gtk_label_set_use_markup(GTK_LABEL(credits_list), TRUE);
    gtk_box_pack_start(GTK_BOX(credits_tab), credits_list, FALSE, FALSE, 0);

    gtk_label_set_use_markup(GTK_LABEL(credits_list), TRUE);
                                            
    gtk_label_set_justify(GTK_LABEL(credits_list), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(credits_tab), credits_list, FALSE, FALSE, 0);

    const char* credits_label_tab = "<b>Credits</b>";
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), credits_tab, gtk_label_new(NULL));
    gtk_label_set_markup(GTK_LABEL(gtk_notebook_get_tab_label(GTK_NOTEBOOK(notebook), credits_tab)), credits_label_tab);

    GtkWidget *separator3 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(credits_tab), separator3, FALSE, FALSE, 0);

    GtkWidget *link_button1 = gtk_link_button_new_with_label("https://illumiterm.blogspot.com", "Visit Website");
    gtk_box_pack_start(GTK_BOX(credits_tab), link_button1, FALSE, FALSE, 0);

    GtkWidget *license_tab = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *license_label = gtk_label_new(NULL);
    
    GtkWidget *copy1 = gtk_label_new(NULL);
    const char *mark_up_text1 = "<b>Copyright (C) 2023 Elijah Gordon (SLcK)</b>";
    gtk_label_set_markup(GTK_LABEL(copy1), mark_up_text1);
    gtk_box_pack_end(GTK_BOX(credits_tab), copy1, FALSE, FALSE, 0);
    
    GtkWidget *copy2 = gtk_label_new(NULL);
    const char *mark_up_text2 = "<b>Copyright (C) 2023 Elijah Gordon (SLcK)</b>";
    gtk_label_set_markup(GTK_LABEL(copy2), mark_up_text2);
    gtk_box_pack_end(GTK_BOX(license_tab), copy2, FALSE, FALSE, 0);
    
    GtkWidget *logo2 = gtk_image_new_from_file("/usr/share/icons/hicolor/96x96/apps/about.png");
    gtk_box_pack_start(GTK_BOX(license_tab), logo2, FALSE, FALSE, 0);

    const char* license_label_tab = "<b>License</b>";
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), license_tab, gtk_label_new(NULL));
    gtk_label_set_markup(GTK_LABEL(gtk_notebook_get_tab_label(GTK_NOTEBOOK(notebook), license_tab)), license_label_tab);
    
    const char* license_label_text = "<big><b>License</b></big>";

    GtkWidget *separator4 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(license_tab), separator4, FALSE, FALSE, 0);

    gtk_label_set_markup(GTK_LABEL(license_label), license_label_text);
    gtk_box_pack_start(GTK_BOX(license_tab), license_label, FALSE, FALSE, 0);
    GtkWidget *license_description = gtk_label_new("<b>This program is free software; you can redistribute it and/or\n"
                                                   "modify it under the terms of the GNU General Public License\n"
                                                   "as published by the Free Software Foundation; either version 2\n"
                                                   "of the License, or (at your option) any later version.</b>\n\n"
                                                   "<b>This program is distributed in the hope that it will be useful,\n"
                                                   "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
                                                   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
                                                   "GNU General Public License for more details.</b>\n\n"
                                                   "<b>You should have received a copy of the GNU General Public License\n"
                                                   "along with this program; if not, write to the Free Software\n"
                                                   "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.</b>");
                                                   
    gtk_label_set_use_markup(GTK_LABEL(license_description), TRUE);

    gtk_label_set_justify(GTK_LABEL(license_description), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(license_tab), license_description, FALSE, FALSE, 0);

    GtkWidget *separator5 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(license_tab), separator5, FALSE, FALSE, 0);
    GtkWidget *link_button2 = gtk_link_button_new_with_label("https://illumiterm.blogspot.com", "Visit Website");
    gtk_box_pack_start(GTK_BOX(license_tab), link_button2, FALSE, FALSE, 0);
    GtkWidget *copy = gtk_label_new(NULL);
    const char *markup_text = "<b>Copyright (C) 2023 Elijah Gordon (SLcK)</b>";
    gtk_label_set_markup(GTK_LABEL(copy), markup_text);
    gtk_box_pack_end(GTK_BOX(about_tab), copy, FALSE, FALSE, 0);

    gtk_widget_show_all(about_window);
}

void on_about_activate(GtkMenuItem *menuitem, gpointer user_data) {
    create_about_window(GTK_WINDOW(user_data));
}

static GtkWidget* create_about_menu_item() {
    GtkWidget* about_menu_item = gtk_menu_item_new_with_label("About");
    g_signal_connect(about_menu_item, "activate", G_CALLBACK(on_about_activate), NULL);
    return about_menu_item;
}

static GtkWidget* create_help_menu() {
    GtkWidget* help_menu = gtk_menu_new();
    GtkWidget* about_menu_item = create_about_menu_item();
    gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), about_menu_item);
    return help_menu;
}

static GtkWidget* create_menu() {
    GtkWidget *menu_bar = gtk_menu_bar_new();

    GtkWidget *file_menu_item = gtk_menu_item_new_with_label("File");
    GtkWidget *file_menu = create_file_menu();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), file_menu_item);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_menu_item), file_menu);

    GtkWidget *edit_menu_item = gtk_menu_item_new_with_label("Edit");
    GtkWidget *edit_menu = create_edit_menu();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(edit_menu_item), edit_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), edit_menu_item);

    GtkWidget *tabs_menu_item = gtk_menu_item_new_with_label("Tabs");
    GtkWidget *tabs_menu = create_tabs_menu();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(tabs_menu_item), tabs_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), tabs_menu_item);

    GtkWidget *help_menu_item = gtk_menu_item_new_with_label("Help");
    GtkWidget *help_menu = create_help_menu();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_menu_item), help_menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), help_menu_item);

    return menu_bar;
}

static void set_notebook_show_tabs(GtkWidget* notebook) {
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
}

static GtkWidget* create_notebook(GtkWidget* widget) {
    GtkWidget *notebook = gtk_notebook_new();
    set_notebook_show_tabs(notebook);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);

    gtk_container_add(GTK_CONTAINER(scrolled_window), notebook);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

    int widget_height = 1000;
    gtk_widget_set_size_request(widget, 0, widget_height);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled_window), widget_height);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, NULL);
    gtk_widget_show_all(scrolled_window);

    return scrolled_window;
}

static GtkWidget* create_window(GtkWidget* menu_bar, GtkWidget* notebook) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);

    gtk_container_add(GTK_CONTAINER(scrolled_window), notebook);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_window_set_icon_from_file(GTK_WINDOW(window), "/usr/share/icons/hicolor/48x48/apps/illumiterm.png", NULL);
    gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_window_set_title(GTK_WINDOW(window), NULL);
    gtk_window_set_default_size(GTK_WINDOW(window), 640, 460);
    gtk_window_set_icon_name(GTK_WINDOW(window), NULL);
    gtk_widget_show_all(window);

    return window;
}

static void command_line(GApplication *application, GApplicationCommandLine *cli, gpointer data) {
    GtkWidget *widget = vte_terminal_new();
    GtkWidget* menu_bar = create_menu();
    GtkWidget* notebook = create_notebook(widget);
    GtkWidget* window = create_window(menu_bar, notebook);

    g_application_hold(application);
    g_object_set_data_full(G_OBJECT(cli), "application", application, (GDestroyNotify)g_application_release);
    g_object_set_data_full(G_OBJECT(window), "cli", cli, NULL);
    g_object_ref(cli);

    spawn_vte_terminal(cli, window, widget);
}

static void connect_signals(GtkApplication* application) {
    g_signal_connect(application, "command-line", G_CALLBACK(command_line), NULL);
}

static int run_application(int argc, char **argv) {
    GtkApplication *application = gtk_application_new("SLcK.IllumiTerm", G_APPLICATION_HANDLES_COMMAND_LINE | G_APPLICATION_SEND_ENVIRONMENT);
    connect_signals(application);
    int status = g_application_run(G_APPLICATION(application), argc, argv);
    g_object_unref(application);
    return status;
}

int main(int argc, char **argv) {
    int status = run_application(argc, argv);

    return status;
}
