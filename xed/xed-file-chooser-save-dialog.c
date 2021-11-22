/*
 * xed-file-chooser-save-dialog.c
 * This file is part of xed
 *
 * Copyright (C) 2021 - Oliver Kästner
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

#include "xed-file-chooser-save-dialog.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (XedFileChooserSaveDialog, xed_file_chooser_save_dialog, XED_TYPE_FILE_CHOOSER_DIALOG)

static void
xed_file_chooser_save_dialog_init (XedFileChooserSaveDialog* self)
{
}

static void
xed_file_chooser_save_dialog_class_init (XedFileChooserSaveDialogClass* klass)
{
}

XedFileChooserSaveDialog *
xed_file_chooser_save_dialog_new (GtkWindow *window)
{
    XedFileChooserSaveDialog *dialog;
    XedFileChooserDialogClass *klass;

    dialog = g_object_new (XED_TYPE_FILE_CHOOSER_SAVE_DIALOG, NULL);
    klass = XED_FILE_CHOOSER_DIALOG_GET_CLASS (&dialog->parent_instance);

    klass->setup (XED_FILE_CHOOSER_DIALOG (dialog),
                  _("Save As\342\200\246"),
                  window,
                  GTK_FILE_CHOOSER_ACTION_SAVE,
                  NULL,
                  _("_Cancel"), GTK_RESPONSE_CANCEL,
                  _("_Save"), GTK_RESPONSE_OK,
                  NULL);

    return dialog;
}