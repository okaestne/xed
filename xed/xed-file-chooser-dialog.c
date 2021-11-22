/*
 * xed-file-chooser-dialog.c
 * This file is part of xed
 *
 * Copyright (C) 2005-2007 - Paolo Maggi
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/*
 * Modified by the xed Team, 2005-2007. See the AUTHORS file for a
 * list of people on the xed Team.
 * See the ChangeLog files for a list of changes.
 *
 * $Id$
 */

/* TODO: Override set_extra_widget */
/* TODO: add encoding property */

#include <config.h>
#include <string.h>
#include <glib/gi18n.h>

#include "xed-file-chooser-dialog.h"
#include "xed-encodings-combo-box.h"
#include "xed-debug.h"
#include "xed-enum-types.h"
#include "xed-settings.h"

#define ALL_FILES _("All Files")
#define ALL_TEXT_FILES _("All Text Files")

static void
xed_file_chooser_dialog_setup (XedFileChooserDialog    *dialog,
                               const gchar             *title,
                               GtkWindow               *parent,
                               GtkFileChooserAction     action,
                               const GtkSourceEncoding *encoding,
                               const gchar             *first_button_text,
                               ...);

typedef struct _XedFileChooserDialogPrivate
{
    GtkFileChooserNative *gtk_chooser_native;

    GSettings *filter_settings;

    GtkWidget *option_menu;
    GtkWidget *extra_widget;

    GtkWidget *newline_label;
    GtkWidget *newline_combo;
    GtkListStore *newline_store;
} XedFileChooserDialogPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (XedFileChooserDialog, xed_file_chooser_dialog, G_TYPE_OBJECT)

static void
xed_file_chooser_dialog_dispose (GObject *object)
{
    XedFileChooserDialog *dialog = XED_FILE_CHOOSER_DIALOG (object);
    XedFileChooserDialogPrivate *priv = xed_file_chooser_dialog_get_instance_private (dialog);

    if (priv->gtk_chooser_native != NULL) {
        g_object_unref (priv->gtk_chooser_native);
    }

    priv->gtk_chooser_native = NULL;

    g_clear_object (&priv->filter_settings);

    G_OBJECT_CLASS (xed_file_chooser_dialog_parent_class)->dispose (object);
}

static void
xed_file_chooser_dialog_class_init (XedFileChooserDialogClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = xed_file_chooser_dialog_dispose;
    klass->setup = xed_file_chooser_dialog_setup;
}

static void
create_option_menu (XedFileChooserDialog *dialog)
{
    XedFileChooserDialogPrivate *priv = xed_file_chooser_dialog_get_instance_private (dialog);
    GtkWidget *label;
    GtkWidget *menu;

    label = gtk_label_new_with_mnemonic (_("C_haracter Encoding:"));
    gtk_widget_set_halign (label, GTK_ALIGN_START);

    menu = xed_encodings_combo_box_new (
        gtk_file_chooser_get_action (GTK_FILE_CHOOSER (priv->gtk_chooser_native)) == GTK_FILE_CHOOSER_ACTION_SAVE);

    gtk_label_set_mnemonic_widget (GTK_LABEL (label), menu);

    gtk_box_pack_start (GTK_BOX (priv->extra_widget), label, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (priv->extra_widget), menu, TRUE, TRUE, 0);

    gtk_widget_show (label);
    gtk_widget_show (menu);

    priv->option_menu = menu;
}

static void
update_newline_visibility (XedFileChooserDialog *dialog)
{
    XedFileChooserDialogPrivate *priv = xed_file_chooser_dialog_get_instance_private (dialog);

    if (gtk_file_chooser_get_action (GTK_FILE_CHOOSER (priv->gtk_chooser_native)) == GTK_FILE_CHOOSER_ACTION_SAVE)
    {
        gtk_widget_show (priv->newline_label);
        gtk_widget_show (priv->newline_combo);
    }
    else
    {
        gtk_widget_hide (priv->newline_label);
        gtk_widget_hide (priv->newline_combo);
    }
}

static void
newline_combo_append (GtkComboBox          *combo,
                      GtkListStore         *store,
                      GtkTreeIter          *iter,
                      const gchar          *label,
                      GtkSourceNewlineType  newline_type)
{
    gtk_list_store_append (store, iter);
    gtk_list_store_set (store, iter, 0, label, 1, newline_type, -1);

    if (newline_type == GTK_SOURCE_NEWLINE_TYPE_DEFAULT)
    {
        gtk_combo_box_set_active_iter (combo, iter);
    }
}

static void
create_newline_combo (XedFileChooserDialog *dialog)
{
    XedFileChooserDialogPrivate *priv = xed_file_chooser_dialog_get_instance_private (dialog);
    GtkWidget *label, *combo;
    GtkListStore *store;
    GtkCellRenderer *renderer;
    GtkTreeIter iter;

    label = gtk_label_new_with_mnemonic (_("L_ine Ending:"));
    gtk_widget_set_halign (label, GTK_ALIGN_START);

    store = gtk_list_store_new (2, G_TYPE_STRING, GTK_SOURCE_TYPE_NEWLINE_TYPE);
    combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
    renderer = gtk_cell_renderer_text_new ();

    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);

    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (combo), renderer, "text", 0);

    newline_combo_append (GTK_COMBO_BOX (combo), store, &iter, _("Unix/Linux"), GTK_SOURCE_NEWLINE_TYPE_LF);
    newline_combo_append (GTK_COMBO_BOX (combo), store, &iter, _("Mac OS Classic"), GTK_SOURCE_NEWLINE_TYPE_CR);
    newline_combo_append (GTK_COMBO_BOX (combo), store, &iter, _("Windows"), GTK_SOURCE_NEWLINE_TYPE_CR_LF);

    gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

    gtk_box_pack_start (GTK_BOX (priv->extra_widget), label, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (priv->extra_widget), combo, TRUE, TRUE, 0);

    priv->newline_combo = combo;
    priv->newline_label = label;
    priv->newline_store = store;

    update_newline_visibility (dialog);
}

static void
create_extra_widget (XedFileChooserDialog *dialog)
{
    XedFileChooserDialogPrivate *priv = xed_file_chooser_dialog_get_instance_private (dialog);

    priv->extra_widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

    gtk_widget_show (priv->extra_widget);

    create_option_menu (dialog);
    create_newline_combo (dialog);

    gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (priv->gtk_chooser_native), priv->extra_widget);

}

static void
action_changed (XedFileChooserDialog *dialog,
                GParamSpec           *pspec,
                gpointer              data)
{
    XedFileChooserDialogPrivate *priv = xed_file_chooser_dialog_get_instance_private (dialog);
    GtkFileChooserAction action;

    action = gtk_file_chooser_get_action (GTK_FILE_CHOOSER (priv->gtk_chooser_native));

    switch (action)
    {
        case GTK_FILE_CHOOSER_ACTION_OPEN:
            g_object_set (priv->option_menu, "save_mode", FALSE, NULL);
            gtk_widget_show (priv->option_menu);
            break;
        case GTK_FILE_CHOOSER_ACTION_SAVE:
            g_object_set (priv->option_menu, "save_mode", TRUE, NULL);
            gtk_widget_show (priv->option_menu);
            break;
        default:
            gtk_widget_hide (priv->option_menu);
    }

    update_newline_visibility (dialog);
}

static void
filter_changed (XedFileChooserDialog *dialog,
                GParamSpec           *pspec,
                gpointer              data)
{
    XedFileChooserDialogPrivate *priv = xed_file_chooser_dialog_get_instance_private (dialog);
    GtkFileFilter *filter;

    filter = gtk_file_chooser_get_filter (GTK_FILE_CHOOSER (priv->gtk_chooser_native));
    if (filter != NULL)
    {
        const gchar *name;
        gint id = 0;

        name = gtk_file_filter_get_name (filter);
        g_return_if_fail (name != NULL);

        if (strcmp (name, ALL_TEXT_FILES) == 0)
        {
            id = 1;
        }

        xed_debug_message (DEBUG_COMMANDS, "Active filter: %s (%d)", name, id);

        g_settings_set_int (priv->filter_settings, XED_SETTINGS_ACTIVE_FILE_FILTER, id);
    }
}

/* FIXME: use globs too - Paolo (Aug. 27, 2007) */
static gboolean
all_text_files_filter (const GtkFileFilterInfo *filter_info,
                       gpointer                 data)
{
    static GSList *known_mime_types = NULL;
    GSList *mime_types;

    if (known_mime_types == NULL)
    {
        GtkSourceLanguageManager *lm;
        const gchar * const *languages;

        lm = gtk_source_language_manager_get_default ();
        languages = gtk_source_language_manager_get_language_ids (lm);

        while ((languages != NULL) && (*languages != NULL))
        {
            gchar **mime_types;
            gint i;
            GtkSourceLanguage *lang;

            lang = gtk_source_language_manager_get_language (lm, *languages);
            g_return_val_if_fail (GTK_SOURCE_IS_LANGUAGE (lang), FALSE);
            ++languages;

            mime_types = gtk_source_language_get_mime_types (lang);
            if (mime_types == NULL)
            {
                continue;
            }

            for (i = 0; mime_types[i] != NULL; i++)
            {
                if (!g_content_type_is_a (mime_types[i], "text/plain"))
                {
                    xed_debug_message (DEBUG_COMMANDS,
                                       "Mime-type %s is not related to text/plain",
                                       mime_types[i]);

                    known_mime_types = g_slist_prepend (known_mime_types, g_strdup (mime_types[i]));
                }
            }

            g_strfreev (mime_types);
        }

        /* known_mime_types always has "text/plain" as first item" */
        known_mime_types = g_slist_prepend (known_mime_types, g_strdup ("text/plain"));
    }

    /* known mime_types contains "text/plain" and then the list of mime-types unrelated to "text/plain"
     * that xed recognizes */

    if (filter_info->mime_type == NULL)
    {
        return FALSE;
    }

    /*
     * The filter is matching:
     * - the mime-types beginning with "text/"
     * - the mime-types inheriting from a known mime-type (note the text/plain is
     *   the first known mime-type)
     */

    if (strncmp (filter_info->mime_type, "text/", 5) == 0)
    {
        return TRUE;
    }

    mime_types = known_mime_types;
    while (mime_types != NULL)
    {
        if (g_content_type_is_a (filter_info->mime_type, (const gchar*)mime_types->data))
        {
            return TRUE;
        }

        mime_types = g_slist_next (mime_types);
    }

    return FALSE;
}

static void
xed_file_chooser_dialog_init (XedFileChooserDialog *dialog)
{
    XedFileChooserDialogPrivate *priv = xed_file_chooser_dialog_get_instance_private (dialog);
    priv = xed_file_chooser_dialog_get_instance_private (dialog);

    priv->filter_settings = g_settings_new ("org.x.editor.state.file-filter");
}

static void
xed_file_chooser_dialog_setup_valist (XedFileChooserDialog    *dialog,
                                      const gchar             *title,
                                      GtkWindow               *parent,
                                      GtkFileChooserAction     action,
                                      const GtkSourceEncoding *encoding,
                                      const gchar             *first_button_text,
                                      va_list                  varargs)
{
    XedFileChooserDialogPrivate *priv;
    const char *button_text = first_button_text;
    gint response_id;
    GtkFileFilter *filter;
    gint active_filter;

    g_return_if_fail (parent != NULL);

    priv = xed_file_chooser_dialog_get_instance_private (dialog);

    priv->gtk_chooser_native = gtk_file_chooser_native_new (
            title, parent, action, first_button_text, first_button_text);
    //                       "title", title,
    //                       "local-only", FALSE,
    //                       "action", action,
    //                       "select-multiple", action == GTK_FILE_CHOOSER_ACTION_OPEN,
    //                       NULL);

    create_extra_widget (XED_FILE_CHOOSER_DIALOG (dialog));

    g_signal_connect (priv->gtk_chooser_native, "notify::action",
                      G_CALLBACK (action_changed), NULL);

    if (encoding != NULL)
    {
        xed_encodings_combo_box_set_selected_encoding (
                XED_ENCODINGS_COMBO_BOX (priv->option_menu), encoding);
    }

    active_filter = g_settings_get_int (priv->filter_settings,
                                        XED_SETTINGS_ACTIVE_FILE_FILTER);
    xed_debug_message (DEBUG_COMMANDS, "Active filter: %d", active_filter);

    /* Filters */
    filter = gtk_file_filter_new ();

    gtk_file_filter_set_name (filter, ALL_FILES);
    gtk_file_filter_add_pattern (filter, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (priv->gtk_chooser_native), filter);
    //gtk_file_chooser_set_action (GTK_FILE_CHOOSER (priv->gtk_chooser_native), action);

    if (active_filter != 1)
    {
        /* Make this filter the default */
        gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (priv->gtk_chooser_native), filter);
    }

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, ALL_TEXT_FILES);
    gtk_file_filter_add_custom (filter, GTK_FILE_FILTER_MIME_TYPE, all_text_files_filter, NULL, NULL);
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (priv->gtk_chooser_native), filter);

    if (active_filter == 1)
    {
        /* Make this filter the default */
        gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (priv->gtk_chooser_native), filter);
    }

    g_signal_connect (priv->gtk_chooser_native, "notify::filter",
                      G_CALLBACK (filter_changed), NULL);

    gtk_native_dialog_set_transient_for (GTK_NATIVE_DIALOG (priv->gtk_chooser_native), parent);
    //gtk_window_set_destroy_with_parent (GTK_WINDOW (priv->gtk_chooser_native), TRUE);
/*
    while (button_text)
    {
        response_id = va_arg (varargs, gint);

        gtk_dialog_add_button (GTK_DIALOG (priv->gtk_chooser_native), button_text, response_id);
        if ((response_id == GTK_RESPONSE_OK) ||
            (response_id == GTK_RESPONSE_ACCEPT) ||
            (response_id == GTK_RESPONSE_YES) ||
            (response_id == GTK_RESPONSE_APPLY))
        {
            gtk_dialog_set_default_response (GTK_DIALOG (priv->gtk_chooser_native), response_id);
        }

        button_text = va_arg (varargs, const gchar *);
    }*/
}

/**
 * xed_file_chooser_dialog_new:
 * @title: (allow-none): Title of the dialog, or %NULL
 * @parent: (allow-none): Transient parent of the dialog, or %NULL
 * @action: Open or save mode for the dialog
 * @first_button_text: (allow-none): stock ID or text to go in
 * the first button, or %NULL
 * @...: (allow-none): response ID for the first button, then
 * additional (button, id) pairs, ending with %NULL
 *
 * Creates a new #XedFileChooserDialog.  This function is analogous to
 * gtk_dialog_new_with_buttons().
 *
 * Return value: a new #XedFileChooserDialog
 *
 **/
static void
xed_file_chooser_dialog_setup (XedFileChooserDialog    *dialog,
                               const gchar             *title,
                               GtkWindow               *parent,
                               GtkFileChooserAction     action,
                               const GtkSourceEncoding *encoding,
                               const gchar             *first_button_text,
                               ...)
{
    va_list varargs;

    //result = g_object_new (XED_TYPE_FILE_CHOOSER_DIALOG, NULL);

    va_start (varargs, first_button_text);
    xed_file_chooser_dialog_setup_valist (dialog, title, parent, action, encoding, first_button_text, varargs);
    va_end (varargs);
}

void
xed_file_chooser_dialog_show (XedFileChooserDialog *dialog) {
    XedFileChooserDialogPrivate *priv = xed_file_chooser_dialog_get_instance_private (dialog);

    g_return_if_fail (XED_IS_FILE_CHOOSER_DIALOG (dialog));

    gtk_native_dialog_show (GTK_NATIVE_DIALOG (priv->gtk_chooser_native));
}

void
xed_file_chooser_dialog_set_encoding (XedFileChooserDialog    *dialog,
                                      const GtkSourceEncoding *encoding)
{
    XedFileChooserDialogPrivate *priv = xed_file_chooser_dialog_get_instance_private (dialog);

    g_return_if_fail (XED_IS_FILE_CHOOSER_DIALOG (priv->gtk_chooser_native));
    g_return_if_fail (XED_IS_ENCODINGS_COMBO_BOX (priv->option_menu));

    xed_encodings_combo_box_set_selected_encoding (XED_ENCODINGS_COMBO_BOX (priv->option_menu), encoding);
}

const GtkSourceEncoding *
xed_file_chooser_dialog_get_encoding (XedFileChooserDialog *dialog)
{
    XedFileChooserDialogPrivate *priv = xed_file_chooser_dialog_get_instance_private (dialog);

    g_return_val_if_fail (XED_IS_FILE_CHOOSER_DIALOG (priv->gtk_chooser_native), NULL);
    g_return_val_if_fail (XED_IS_ENCODINGS_COMBO_BOX (priv->option_menu), NULL);
    g_return_val_if_fail ((gtk_file_chooser_get_action (GTK_FILE_CHOOSER (priv->gtk_chooser_native)) == GTK_FILE_CHOOSER_ACTION_OPEN ||
                          gtk_file_chooser_get_action (GTK_FILE_CHOOSER (priv->gtk_chooser_native)) == GTK_FILE_CHOOSER_ACTION_SAVE), NULL);

    return xed_encodings_combo_box_get_selected_encoding (XED_ENCODINGS_COMBO_BOX (priv->option_menu));
}

static void
set_enum_combo (GtkComboBox *combo,
                gint         value)
{
    GtkTreeIter iter;
    GtkTreeModel *model;

    model = gtk_combo_box_get_model (combo);

    if (!gtk_tree_model_get_iter_first (model, &iter))
    {
        return;
    }

    do
    {
        gint nt;

        gtk_tree_model_get (model, &iter, 1, &nt, -1);

        if (value == nt)
        {
            gtk_combo_box_set_active_iter (combo, &iter);
            break;
        }
    } while (gtk_tree_model_iter_next (model, &iter));
}

void
xed_file_chooser_dialog_set_newline_type (XedFileChooserDialog *dialog,
                                          GtkSourceNewlineType  newline_type)
{
    XedFileChooserDialogPrivate *priv = xed_file_chooser_dialog_get_instance_private (dialog);
    g_return_if_fail (XED_IS_FILE_CHOOSER_DIALOG (priv->gtk_chooser_native));
    g_return_if_fail (gtk_file_chooser_get_action (GTK_FILE_CHOOSER (priv->gtk_chooser_native)) == GTK_FILE_CHOOSER_ACTION_SAVE);

    set_enum_combo (GTK_COMBO_BOX (priv->newline_combo), newline_type);
}

GtkSourceNewlineType
xed_file_chooser_dialog_get_newline_type (XedFileChooserDialog *dialog)
{
    XedFileChooserDialogPrivate *priv = xed_file_chooser_dialog_get_instance_private (dialog);
    GtkTreeIter iter;
    GtkSourceNewlineType newline_type;

    g_return_val_if_fail (XED_IS_FILE_CHOOSER_DIALOG (priv->gtk_chooser_native), GTK_SOURCE_NEWLINE_TYPE_DEFAULT);
    g_return_val_if_fail (gtk_file_chooser_get_action (GTK_FILE_CHOOSER (priv->gtk_chooser_native)) == GTK_FILE_CHOOSER_ACTION_SAVE,
                          GTK_SOURCE_NEWLINE_TYPE_DEFAULT);

    gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->newline_combo), &iter);

    gtk_tree_model_get (GTK_TREE_MODEL (priv->newline_store), &iter, 1, &newline_type, -1);

    return newline_type;
}
