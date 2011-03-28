/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2005 Francisco Moraes
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <windows.h>
#include <shellapi.h>
#include <stdio.h>

#include "planner-util.h"

void
planner_util_show_url (GtkWindow *parent,
		       const gchar *url)
{
	GtkWidget *dialog;
	int res;

	res = (int) ShellExecute (NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);

	if (res <= 32) {
		dialog = gtk_message_dialog_new (parent,
		                                 GTK_DIALOG_DESTROY_WITH_PARENT,
		                                 GTK_MESSAGE_ERROR,
		                                 GTK_BUTTONS_CLOSE,
		                                 _("Unable to open '%s'"), url);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}
}

void
planner_util_show_help (GtkWindow *parent)
{
	GtkWidget *dialog;
	int    res;
	gchar *path;
	gchar *file;

	// should use HtmlHelp but it is not part of MingW yet
	path = g_win32_get_package_installation_subdirectory (NULL, NULL, ".");
	file = g_build_filename (path, "planner.chm", NULL);

	res = (int) ShellExecute (NULL, "open", file, NULL, NULL, SW_SHOWNORMAL);
	g_free (file);
	g_free (path);

	if (res <= 32) {
		dialog = gtk_message_dialog_new (parent,
		                                 GTK_DIALOG_DESTROY_WITH_PARENT,
		                                 GTK_MESSAGE_ERROR,
		                                 GTK_BUTTONS_CLOSE,
		                                 _("Unable to open help file"));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}
}
