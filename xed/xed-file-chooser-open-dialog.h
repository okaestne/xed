/*
 * xed-file-chooser-open-dialog.h
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

#ifndef XED_FILE_CHOOSER_OPEN_DIALOG_H
#define XED_FILE_CHOOSER_OPEN_DIALOG_H

#include <glib-object.h>
#include "xed-file-chooser-dialog.h"

G_BEGIN_DECLS

#define XED_TYPE_FILE_CHOOSER_OPEN_DIALOG xed_file_chooser_open_dialog_get_type ()
G_DECLARE_FINAL_TYPE (XedFileChooserOpenDialog, xed_file_chooser_open_dialog, XED, FILE_CHOOSER_OPEN_DIALOG, XedFileChooserDialog)

struct _XedFileChooserOpenDialog
{
    XedFileChooserDialog parent_instance;
};

XedFileChooserOpenDialog *xed_file_chooser_open_dialog_new (GtkWindow *window);



G_END_DECLS

#endif // XED_FILE_CHOOSER_OPEN_DIALOG_H