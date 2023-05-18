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

// This function retrieves the window title of a VteTerminal widget.
// It returns the window title as a string.
static const gchar* get_new_window_title(VteTerminal* terminal) {
    return vte_terminal_get_window_title(terminal);
}

// This function sets the window title of a GTK window.
// It takes a GtkWidget* representing the window and a const gchar* representing the new title.
static void set_window_title(GtkWidget* window, const gchar* new_title) {
    // Call the gtk_window_set_title function to set the window title.
    // It takes a GtkWindow* as the first argument, so we cast the GtkWidget* to GtkWindow* using GTK_WINDOW macro.
    gtk_window_set_title(GTK_WINDOW(window), new_title);
}

// This function is a signal callback that is triggered when the window title of a VteTerminal widget changes.
// It takes a GtkWidget* representing the widget that emitted the signal (VteTerminal) and a gpointer representing the window.
static void window_title_changed(GtkWidget* widget, gpointer window) {
    // Retrieve the new window title by calling the get_new_window_title function.
    // We cast the GtkWidget* to VteTerminal* using VTE_TERMINAL macro and then call get_new_window_title.
    const gchar* new_title = get_new_window_title(VTE_TERMINAL(widget));
    
    // Set the new window title by calling the set_window_title function.
    // We pass the window as a gpointer and cast it to GtkWidget* when calling set_window_title.
    set_window_title(GTK_WIDGET(window), new_title);
}

// This function sets the exit status of a GApplicationCommandLine object.
// It takes a GApplicationCommandLine* representing the command line object and a gint representing the exit status.
static void set_exit_status(GApplicationCommandLine* cli, gint status) {
    // Check if the command line object is not NULL.
    if (cli) {
        // Set the exit status of the command line object using the specified status.
        g_application_command_line_set_exit_status(cli, status);
        
        // Decrease the reference count of the command line object to free the associated resources.
        g_object_unref(cli);
    }
}

// This function is used as a callback to destroy a GTK window widget.
// It takes a GtkWidget* representing the widget to be destroyed and a gpointer representing user data (optional).
static void destroy_window(GtkWidget* widget, gpointer data) {
    // Destroy the specified widget, which effectively removes it from the UI and frees associated resources.
    gtk_widget_destroy(widget);
}

// This function is used to destroy a GTK window widget and quit the application with a specified exit status.
// It takes a GtkWidget* representing the window to be destroyed and a gint representing the exit status.
static void destroy_and_quit(GtkWidget* window, gint status) {
    // Retrieve the GApplicationCommandLine object associated with the window.
    // This is stored as user data with the key "cli" on the window object.
    GApplicationCommandLine* cli = g_object_get_data(G_OBJECT(window), "cli");

    // Set the exit status of the GApplicationCommandLine object.
    set_exit_status(cli, status);

    // Destroy the window.
    destroy_window(window, NULL);
}

// This function is a callback for handling the exit of a child process.
// It is typically connected to the "child-exited" signal of a VteTerminal widget.
// It takes a GtkWidget* representing the window associated with the child process and a gint representing the exit status.
static void handle_child_exit(GtkWidget* window, gint status) {
    // Call the destroy_and_quit function to destroy the window and quit the application with the specified exit status.
    destroy_and_quit(window, status);
}

static gboolean child_exited(VteTerminal* term, gint status, gpointer data) {
    // Cast the data pointer to GtkWidget* since it represents the window
    GtkWidget* window = data;

    // Call the handle_child_exit function, passing the window and the child process status
    handle_child_exit(window, status);

    // Return TRUE to indicate that the child exit event has been handled
    return TRUE;
}

static void get_terminal_dimensions(VteTerminal *terminal, glong *rows, glong *columns, glong *char_width, glong *char_height) {
    // Retrieve the number of rows in the terminal and store it in the provided 'rows' variable
    *rows = vte_terminal_get_row_count(terminal);

    // Retrieve the number of columns in the terminal and store it in the provided 'columns' variable
    *columns = vte_terminal_get_column_count(terminal);

    // Retrieve the width of a character in the terminal and store it in the provided 'char_width' variable
    *char_width = vte_terminal_get_char_width(terminal);

    // Retrieve the height of a character in the terminal and store it in the provided 'char_height' variable
    *char_height = vte_terminal_get_char_height(terminal);
}

static void get_container_dimensions(GtkWidget *widget, GtkWindow *window, gint *owidth, gint *oheight, glong char_width, glong char_height, glong columns, glong rows) {
    // Get the size of the window
    gtk_window_get_size(window, owidth, oheight);
    
    // Subtract the space occupied by the terminal content from the window size
    *owidth -= char_width * columns;
    *oheight -= char_height * rows;
}

static void adjust_font_scale(VteTerminal *terminal, gdouble factor) {
    // Get the current font scale of the terminal
    gdouble scale = vte_terminal_get_font_scale(terminal);
    
    // Set the new font scale by multiplying the current scale with the factor
    vte_terminal_set_font_scale(terminal, scale * factor);
}

static void adjust_terminal_size(GtkWindow *window, glong rows, glong columns, glong char_width, glong char_height, gint owidth, gint oheight) {
    // Calculate the new width and height of the terminal window
    gint new_width = columns * char_width + owidth;
    gint new_height = rows * char_height + oheight;

    // Resize the window to the new dimensions
    gtk_window_resize(window, new_width, new_height);
}

static void adjust_font_size(GtkWidget *widget, GtkWindow *window, gdouble factor)
{
    // Get the VteTerminal widget
    VteTerminal *terminal = VTE_TERMINAL(widget);

    // Retrieve the dimensions of the terminal
    glong rows, columns, char_width, char_height;
    get_terminal_dimensions(terminal, &rows, &columns, &char_width, &char_height);

    // Retrieve the container dimensions
    gint owidth, oheight;
    get_container_dimensions(widget, window, &owidth, &oheight, char_width, char_height, columns, rows);

    // Adjust the font scale
    adjust_font_scale(terminal, factor);

    // Get the updated terminal dimensions after font adjustment
    get_terminal_dimensions(terminal, &rows, &columns, &char_width, &char_height);

    // Adjust the terminal size based on the updated dimensions and container offsets
    adjust_terminal_size(window, rows, columns, char_width, char_height, owidth, oheight);
}

static void increase_font_size(GtkWidget *widget, gpointer window)
{
    // Convert the widget pointer to VteTerminal type
    VteTerminal *terminal = VTE_TERMINAL(widget);

    // Increase the font size by a factor of 1.125
    adjust_font_size(widget, GTK_WINDOW(window), 1.125);
}

static void decrease_font_size(GtkWidget *widget, gpointer window)
{
    // Convert the widget pointer to VteTerminal type
    VteTerminal *terminal = VTE_TERMINAL(widget);

    // Decrease the font size by dividing it by a factor of 1.125
    adjust_font_size(widget, GTK_WINDOW(window), 1.0 / 1.125);
}

static void reset_font_scale(VteTerminal *terminal)
{
    // Reset the font scale of the VteTerminal widget to 1.0
    vte_terminal_set_font_scale(terminal, 1.0);
}

static void reset_font_description_size(VteTerminal *terminal, gint size)
{
    // Create a copy of the current font description of the VteTerminal widget
    PangoFontDescription *font_desc = pango_font_description_copy(vte_terminal_get_font(terminal));

    // Set the font size of the copied font description to the specified size
    pango_font_description_set_size(font_desc, size);

    // Set the font of the VteTerminal widget to the updated font description
    vte_terminal_set_font(terminal, font_desc);

    // Free the memory occupied by the copied font description
    pango_font_description_free(font_desc);
}

static void resize_terminal_window(GtkWindow *window, VteTerminal *terminal, gint char_width, gint char_height)
{
    // Get the number of rows and columns in the terminal
    gint rows = vte_terminal_get_row_count(terminal);
    gint columns = vte_terminal_get_column_count(terminal);

    // Variables to store the original width and height of the window
    gint owidth, oheight;

    // Get the current size of the window
    gtk_window_get_size(window, &owidth, &oheight);

    // Calculate the new width and height of the window by subtracting the space occupied by characters
    owidth -= char_width * columns;
    oheight -= char_height * rows;

    // Resize the window to fit the adjusted terminal size
    gtk_window_resize(window, columns * char_width + owidth, rows * char_height + oheight);
}

static void reset_font_size(GtkWidget *widget, GtkWindow *window)
{
    // Get a pointer to the VteTerminal widget
    VteTerminal *terminal = VTE_TERMINAL(widget);

    // Get the default font size from the current font description of the terminal
    gint default_font_size = pango_font_description_get_size(vte_terminal_get_font(terminal));

    // Reset the font scale of the terminal to 1.0
    reset_font_scale(terminal);

    // Reset the font size of the terminal to the default font size
    reset_font_description_size(terminal, default_font_size);

    // Resize the terminal window to fit the adjusted terminal size
    resize_terminal_window(window, terminal, vte_terminal_get_char_width(terminal), vte_terminal_get_char_height(terminal));
}

static void reset_window_size(GtkWidget *widget, GtkWindow *window)
{
    // Set the default width and height for the window
    gint default_width = 640;
    gint default_height = 460;

    // Resize the window to the default width and height
    gtk_window_resize(window, default_width, default_height);
}

static gboolean key_press_event(GtkWidget *widget, GdkEvent *event, gpointer window)
{
    // Get the default modifier mask for accelerators
    GdkModifierType modifiers = gtk_accelerator_get_default_mod_mask();

    // Ensure the event type is GDK_KEY_PRESS
    g_assert(event->type == GDK_KEY_PRESS);

    // Check if the key press event matches the specified modifiers
    if ((event->key.state & modifiers) == (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) {
        // Handle key press events with control and shift modifiers
        switch (event->key.hardware_keycode) {
            // Key press event for increasing font size (Ctrl+Shift+Page Up)
            case 21:
                increase_font_size(widget, window);
                return TRUE;
            // Key press event for decreasing font size (Ctrl+Shift+Page Down)
            case 20:
                decrease_font_size(widget, window);
                return TRUE;
            // Key press event for resetting font size and window size (Ctrl+Shift+Home)
            case 19:
                reset_font_size(widget, window);
                reset_window_size(widget, window);
                return TRUE;
        }
        // Handle key press events without hardware keycode (e.g., letters, symbols)
        switch (gdk_keyval_to_lower(event->key.keyval)) {
            // Key press event for copying text to clipboard (Ctrl+Shift+C)
            case GDK_KEY_c:
                vte_terminal_copy_clipboard_format((VteTerminal *)widget, VTE_FORMAT_TEXT);
                return TRUE;
            // Key press event for pasting text from clipboard (Ctrl+Shift+V)
            case GDK_KEY_v:
                vte_terminal_paste_clipboard((VteTerminal *)widget);
                return TRUE;
        }
    }
    // Return FALSE to indicate that the key press event was not handled
    return FALSE;
}

static GtkWidget* create_dialog_buttons() {
    // Create a new button box widget with horizontal orientation
    GtkWidget* buttons_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);

    // Set the spacing between buttons in the button box
    gtk_box_set_spacing(GTK_BOX(buttons_box), 10);

    // Create "No" button widget
    GtkWidget* no_button = gtk_button_new_with_label("No");

    // Create "Yes" button widget
    GtkWidget* yes_button = gtk_button_new_with_label("Yes");

    // Add the "No" button widget to the button box container
    gtk_container_add(GTK_CONTAINER(buttons_box), no_button);

    // Add the "Yes" button widget to the button box container
    gtk_container_add(GTK_CONTAINER(buttons_box), yes_button);

    // Return the created button box widget
    return buttons_box;
}

static GtkWidget* create_dialog_message() {
    // Create a new label widget for the message
    GtkWidget* message_label = gtk_label_new(NULL);

    // Set the horizontal alignment of the label to the left
    gtk_label_set_xalign(GTK_LABEL(message_label), 0);

    // Set the vertical alignment of the label to the top
    gtk_label_set_yalign(GTK_LABEL(message_label), 0);

    // Enable line wrapping for the label
    gtk_label_set_line_wrap(GTK_LABEL(message_label), TRUE);

    // Set the line wrap mode to wrap at word or character boundaries
    gtk_label_set_line_wrap_mode(GTK_LABEL(message_label), PANGO_WRAP_WORD_CHAR);

    // Make the label non-selectable
    gtk_label_set_selectable(GTK_LABEL(message_label), FALSE);

    // Set the justification of the label to center
    gtk_label_set_justify(GTK_LABEL(message_label), GTK_JUSTIFY_CENTER);

    // Return the created label widget
    return message_label;
}

static GtkWidget* create_confirm_dialog() {
    // Create a new dialog widget
    GtkWidget* dialog = gtk_dialog_new();

    // Set the title of the dialog window
    gtk_window_set_title(GTK_WINDOW(dialog), "Confirm close");

    // Set the dialog window as modal
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

    // Disable resizing of the dialog window
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

    // Disable the ability to close the dialog window
    gtk_window_set_deletable(GTK_WINDOW(dialog), FALSE);

    // Enable decoration for the dialog window
    gtk_window_set_decorated(GTK_WINDOW(dialog), TRUE);

    // Get the content area of the dialog
    GtkWidget* content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    // Set the border width of the content area
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 20);

    // Create a message label using the create_dialog_message() function
    GtkWidget* message_label = create_dialog_message();

    // Add the message label to the content area
    gtk_container_add(GTK_CONTAINER(content_area), message_label);

    // Create dialog buttons using the create_dialog_buttons() function
    GtkWidget* buttons_box = create_dialog_buttons();

    // Pack the buttons box at the end of the content area
    gtk_box_pack_end(GTK_BOX(content_area), buttons_box, FALSE, FALSE, 0);

    // Add "No" and "Yes" buttons to the dialog
    gtk_dialog_add_button(GTK_DIALOG(dialog), "No", GTK_RESPONSE_NO);
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Yes", GTK_RESPONSE_YES);

    // Return the created dialog widget
    return dialog;
}

static void destroy_confirm_dialog(GtkWidget* dialog) {
    // Destroy the dialog widget
    gtk_widget_destroy(dialog);
}

static gboolean get_confirm_response(GtkWidget* dialog) {
    // Run the dialog and retrieve the response
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));

    // Destroy the dialog widget
    destroy_confirm_dialog(dialog);

    // Return TRUE if the response is GTK_RESPONSE_NO, otherwise FALSE
    return (response == GTK_RESPONSE_NO) ? TRUE : FALSE;
}

static gboolean confirm_exit(GtkWidget* widget, GdkEvent* event, gpointer data) {
    // Create a confirm dialog
    GtkWidget* dialog = create_confirm_dialog();

    // Get the user's response from the confirm dialog
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
    // Check if the button pressed is not the secondary (right) button
    if (event->button != GDK_BUTTON_SECONDARY) {
        return FALSE;
    }

    // Create a context menu
    GtkWidget *menu = create_context_menu();

    // Display the context menu at the pointer's position
    gtk_menu_popup_at_pointer(GTK_MENU(menu), NULL);

    // Signal that the event has been handled
    return TRUE;
}

static gchar** get_environment(GApplicationCommandLine* cli) {
    // Get the environment from the GApplicationCommandLine
    const gchar* const* environment = g_application_command_line_get_environ(cli);

    // Calculate the number of environment variables
    guint num = g_strv_length((gchar**) environment);

    // Allocate memory for the result array with an additional NULL element
    gchar** result = g_new(gchar*, num + 1);

    // Copy each environment variable to the result array
    for (guint i = 0; i < num; ++i) {
        // Duplicate the environment variable string
        result[i] = g_strdup(environment[i]);
    }

    // Set the last element of the result array to NULL
    result[num] = NULL;

    // Return the resulting array
    return result;
}

static void child_ready(VteTerminal* terminal, GPid pid, GError* error, gpointer user_data) {
    // Check if the terminal widget is valid
    if (!terminal) {
        return;
    }

    // Check if the child process identifier is 0, indicating an error
    if (pid == 0) {
        // Retrieve the window widget from the user data
        GtkWidget* window = GTK_WIDGET(user_data);

        // Destroy the window and quit the application, passing the error code
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
    // Connect the child-exited signal of the VteTerminal widget to the corresponding handler
    connect_child_exited_signal(widget, window);

    // Connect the key-press-event signal of the VteTerminal widget to the corresponding handler
    connect_key_press_event_signal(widget, window);

    // Connect the window-title-changed signal of the VteTerminal widget to the corresponding handler
    connect_window_title_changed_signal(widget, window);

    // Connect the button-press-event signal of the VteTerminal widget to the corresponding handler
    connect_button_press_event_signal(widget);

    // Connect the delete-event signal of the window widget to the corresponding handler
    connect_delete_event_signal(window);
}

static void spawn_vte_terminal(GApplicationCommandLine *cli, GtkWidget *window, GtkWidget *widget) {
    // Get the options dictionary from the command line
    GVariantDict *options = g_application_command_line_get_options_dict(cli);
    
    // Retrieve the value of the "cmd" option from the options dictionary
    const gchar *command = NULL;
    g_variant_dict_lookup(options, "cmd", "&s", &command);
    
    // Get the environment variables for the terminal
    gchar **environment = get_environment(cli);
    
    // Define the command and command line based on the presence of "cmd" option
    gchar **cmd;
    gchar *cmdline = NULL;
    cmd = command ?
        (gchar *[]){"/bin/sh", cmdline = g_strdup(command), NULL} :
        (gchar *[]){cmdline = g_strdup(g_application_command_line_getenv(cli, "SHELL")), NULL};
    
    // Connect the VteTerminal signals to their corresponding handlers
    connect_vte_signals(widget, window);
    
    // Set word char exceptions for the VteTerminal widget
    vte_terminal_set_word_char_exceptions(VTE_TERMINAL(widget), "-./?%&_=+@~:");
    
    // Set the number of scrollback lines for the VteTerminal widget
    vte_terminal_set_scrollback_lines(VTE_TERMINAL(widget), -1);
    
    // Enable scroll on output for the VteTerminal widget
    vte_terminal_set_scroll_on_output(VTE_TERMINAL(widget), TRUE);
    
    // Enable scroll on keystroke for the VteTerminal widget
    vte_terminal_set_scroll_on_keystroke(VTE_TERMINAL(widget), TRUE);
    
    // Enable mouse autohide for the VteTerminal widget
    vte_terminal_set_mouse_autohide(VTE_TERMINAL(widget), TRUE);
    
    // Set bold text to be displayed as bright for the VteTerminal widget
    vte_terminal_set_bold_is_bright(VTE_TERMINAL(widget), TRUE);
    
    // Enable audible bell for the VteTerminal widget
    vte_terminal_set_audible_bell(VTE_TERMINAL(widget), TRUE);
    
    // Enable cursor blink mode for the VteTerminal widget
    vte_terminal_set_cursor_blink_mode(VTE_TERMINAL(widget), TRUE);
    
    // Spawn the VteTerminal asynchronously
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
    
    // Free the environment and cmdline strings
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
    // Cast the user data parameter to GtkWindow* type
    GtkWindow* window = GTK_WINDOW(user_data);
    
    // Call the create_about_window function to create and show the About window
    create_about_window(window);
}

static GtkWidget* create_about_menu_item() {
    // Create a new menu item with the label "About"
    GtkWidget* about_menu_item = gtk_menu_item_new_with_label("About");
    
    // Connect the "activate" signal of the menu item to the on_about_activate callback function
    g_signal_connect(about_menu_item, "activate", G_CALLBACK(on_about_activate), NULL);
    
    // Return the created menu item
    return about_menu_item;
}

static GtkWidget* create_help_menu() {
    // Create a new menu widget to hold the help menu items
    GtkWidget* help_menu = gtk_menu_new();
    
    // Create the "About" menu item using the create_about_menu_item function
    GtkWidget* about_menu_item = create_about_menu_item();
    
    // Append the "About" menu item to the help menu
    gtk_menu_shell_append(GTK_MENU_SHELL(help_menu), about_menu_item);
    
    // Return the help menu
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

void set_notebook_show_tabs(GtkWidget* notebook) {
    // Set the property to hide the tabs in the notebook
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
    // Create a new top-level window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    
    // Create a vertical box container to hold the menu bar and notebook
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    
    // Create a scrolled window to contain the notebook
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);

    // Add the notebook widget to the scrolled window
    gtk_container_add(GTK_CONTAINER(scrolled_window), notebook);
    
    // Set the scrolling policies of the scrolled window to automatic
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    // Set the window icon from a file
    gtk_window_set_icon_from_file(GTK_WINDOW(window), "/usr/share/icons/hicolor/48x48/apps/illumiterm.png", NULL);
    
    // Pack the menu bar at the top of the vertical box container
    gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, FALSE, 0);
    
    // Pack the scrolled window, which contains the notebook, to fill the remaining space in the vertical box container
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
    
    // Add the vertical box container to the window
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    // Set the window title to NULL (empty)
    gtk_window_set_title(GTK_WINDOW(window), NULL);
    
    // Set the default size of the window to 640x460 pixels
    gtk_window_set_default_size(GTK_WINDOW(window), 640, 460);
    
    // Clear the window icon name
    gtk_window_set_icon_name(GTK_WINDOW(window), NULL);
    
    // Show all the widgets within the window
    gtk_widget_show_all(window);

    // Return the created window
    return window;
}

void command_line(GApplication *application, GApplicationCommandLine *cli, gpointer data) {
    // Create a new VteTerminal widget
    GtkWidget *widget = vte_terminal_new();
    
    // Create the menu bar
    GtkWidget* menu_bar = create_menu();
    
    // Create the notebook
    GtkWidget* notebook = create_notebook(widget);
    
    // Create the main window
    GtkWidget* window = create_window(menu_bar, notebook);

    // Hold a reference to the application
    g_application_hold(application);
    
    // Set the application as data associated with the command-line object, and release it when done
    g_object_set_data_full(G_OBJECT(cli), "application", application, (GDestroyNotify)g_application_release);
    
    // Set the command-line object as data associated with the window
    g_object_set_data_full(G_OBJECT(window), "cli", cli, NULL);
    
    // Increase the reference count of the command-line object
    g_object_ref(cli);

    // Spawn a new VteTerminal process asynchronously
    spawn_vte_terminal(cli, window, widget);
}

static void connect_signals(GtkApplication* application) {
    // Connect the "command-line" signal of the GtkApplication to the "command_line" callback function
    g_signal_connect(application, "command-line", G_CALLBACK(command_line), NULL);
}

static int run_application(int argc, char **argv) {
    // Create a new GtkApplication with the specified application ID
    GtkApplication *application = gtk_application_new("SLcK.IllumiTerm", G_APPLICATION_HANDLES_COMMAND_LINE | G_APPLICATION_SEND_ENVIRONMENT);
    
    // Connect signals and set up event handlers for the application
    connect_signals(application);
    
    // Run the GtkApplication main loop and store the exit status in the 'status' variable
    int status = g_application_run(G_APPLICATION(application), argc, argv);
    
    // Unreference and free resources associated with the GtkApplication
    g_object_unref(application);
    
    // Return the exit status of the application
    return status;
}

int main(int argc, char **argv) {
    // Run the application and store the exit status in the 'status' variable
    int status = run_application(argc, argv);

    // Return the exit status of the application
    return status;
}
