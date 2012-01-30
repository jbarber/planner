/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2005 Alvaro del Castillo - acs@barrapunto.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gconf/gconf-client.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <e-util/e-config.h>
#include <calendar/gui/e-cal-config.h>
#include <libedataserver/e-source.h>
#include <libebook/e-book.h>
#include <libecal/e-cal.h>
#include <libedataserver/e-source.h>
#include <libedataserver/e-source-list.h>
#include <string.h>

static GConfClient *conf_client;
static GMainLoop   *main_loop;

void org_gnome_planner_source_add (EPlugin                    *epl,
				   EConfigHookItemFactoryData *data);

/* From mrp-project.c */
static const gchar *
uri_peek_file_name (const gchar *uri)
{
	const gchar *name;
	size_t       len;

	len = strlen (uri);
	if (len > 3 && !strstr (uri, ":/")) {
		/* No protocol. */
	} else {
		if (len > 7 && !strncmp (uri, "file:/", 6)) {
			/* Naively strip method. */
			name = uri + 7;
		} else {
			name = NULL;
		}
	}
	return name;
}

static void
add_planner_file (const gchar *uri)
{
	ESourceList  *source_list;
	ESourceGroup *group;
	ESource      *source;
	const gchar  *file_name;
	gchar        *planner_name;
	const gchar  *conf_key      = "/apps/evolution/tasks/sources";
	const gchar  *planner_group = "Projects";
	gchar        *source_name;
	GConfClient  *conf_client;

	g_message ("Adding the new Task source to evolution: %s\n", uri);
	file_name = uri_peek_file_name (uri);
	if (file_name == NULL) {
		g_warning ("Can't add non file uri: %s", uri);
	}
	/* ideally we would get the planner project name but we need then to
	 depend in libplanner here */
	planner_name = g_path_get_basename (file_name);

	conf_client = gconf_client_get_default ();
	source_list = e_source_list_new_for_gconf (conf_client, conf_key);

	g_message ("Creating the new source ...\n");

	group = e_source_list_peek_group_by_name (source_list, planner_group);
	if (!group) {
		group = e_source_group_new (planner_group, "planner://");
		e_source_list_add_group (source_list, group, -1);
	}
	source_name = g_strdup_printf ("%s", uri);
	source = e_source_new (planner_name, source_name);
	g_free (planner_name);
	e_source_group_add_source (group, source, -1);

	e_source_list_sync (source_list, NULL);

	g_free (source_name);
	g_object_unref (source);
	/* g_object_unref (group); */
	g_object_unref (source_list);
}

void
org_gnome_planner_source_add (EPlugin                    *epl,
			      EConfigHookItemFactoryData *data)
{
	GtkWidget   *widget;
	ESource     *source;
	GtkWidget   *dialog      = NULL;
	gchar       *dest_uri    = NULL;
	gchar       *tmp;
	const gchar *planner_ext = "planner";
	gboolean     proceed     = FALSE;

	//book_target = (EABConfigTargetSource *) data->target;
	//source = book_target->source;

	dialog = gtk_file_chooser_dialog_new (_("Select planner file"),
					      NULL,
					      GTK_FILE_CHOOSER_ACTION_SAVE,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OPEN, GTK_RESPONSE_OK,
		 			      NULL);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK) {
		dest_uri = gtk_file_chooser_get_uri
			(GTK_FILE_CHOOSER (dialog));
	}

	/* cancel */
	if (dest_uri == NULL) {
		gtk_widget_destroy (dialog);
		return;
	}

	tmp = strstr (dest_uri, planner_ext);

	if (tmp && *(tmp + strlen (planner_ext)) == '\0') {
		proceed = TRUE;

	} else {

		GtkWidget *warning =
			gtk_message_dialog_new (NULL,
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_QUESTION,
						GTK_BUTTONS_YES_NO,
						_("The filename extension of this file isn't"
						  " the planner usual file extension (%s) "
						  "Do you want to continue?"), planner_ext);

		if (gtk_dialog_run (GTK_DIALOG (warning)) == GTK_RESPONSE_YES) {
			proceed = TRUE;
		}
		gtk_widget_destroy (warning);
	}

	/* Time to add the new source */
	if (proceed) {
		add_planner_file (dest_uri);
	}
	gtk_widget_destroy (dialog);
	g_free (dest_uri);
}
