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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <config.h>

#include "planner-util.h"

void
planner_util_show_help (GtkWindow *parent)
{
	planner_util_show_url(parent, "ghelp:planner");
}

void
planner_util_show_url (GtkWindow *parent, const gchar *url)
{
	GtkWidget *dialog;
	GError    *error = NULL;

	gtk_show_uri (NULL, url, gtk_get_current_event_time (), &error);
	if (error != NULL) {
		dialog = gtk_message_dialog_new (parent,
		                                 GTK_DIALOG_DESTROY_WITH_PARENT,
		                                 GTK_MESSAGE_ERROR,
		                                 GTK_BUTTONS_CLOSE,
		                                 "%s", error->message);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_error_free (error);
	}
}
