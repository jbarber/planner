/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003 Imendio HB
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
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

#include <config.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <libgnome/libgnome.h>
#include <libgnome/gnome-i18n.h>
#include <gtk/gtk.h>
#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-job.h>
#include <libgnomeprintui/gnome-print-job-preview.h>
#include <libgnomeprintui/gnome-print-paper-selector.h>
#include <libgnomeprintui/gnome-print-dialog.h>
#include "planner-view.h"
#include "planner-print-dialog.h"

#define PLANNER_PRINT_CONFIG_FILE "planner-print-config"

static GtkWidget *   print_dialog_create_page  (PlannerWindow *window,
						GtkWidget     *dialog,
						GList         *views);
static GtkNotebook * print_dialog_get_notebook (GtkWidget     *dialog);


/*
 * Load printer configuration from a file.
 * Return a GnomePrintConfig object containing the configuration.
 */
GnomePrintConfig *
planner_print_dialog_load_config (void)
{
	gchar            *filename;
	gboolean          res;
	gchar            *contents;
	GnomePrintConfig *config;
	
	filename = gnome_util_home_file (PLANNER_PRINT_CONFIG_FILE);
	
	res = g_file_get_contents (filename, &contents, NULL, NULL);
	g_free (filename);
	
	if (res) {
		config = gnome_print_config_from_string (contents, 0);
		g_free (contents);
	} else {
		config = gnome_print_config_default ();
	}
	
	return config;
}

/*
 * Save printer configuration into a file in the .gnome2 user directory
 */
void
planner_print_dialog_save_config (GnomePrintConfig *config)
{
	gint   fd;
	gchar *str;
	gint   bytes;
	gchar *filename;
	
	g_return_if_fail (config != NULL);
	
	str = gnome_print_config_to_string (config, 0);
	
 	filename = gnome_util_home_file (PLANNER_PRINT_CONFIG_FILE);
	fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	g_free (filename);
	
	if (fd >= 0) {
		bytes = strlen (str);
		
		write (fd, str, bytes);
		close (fd);
	}
	g_free (str);
}

GtkWidget *
planner_print_dialog_new (PlannerWindow  *window,
			  GnomePrintJob  *job,
			  GList          *views)
{
	GtkWidget *dialog;
	GtkWidget *page;
	
	dialog = gnome_print_dialog_new (job, _("Print Project"), 0);

	page = print_dialog_create_page (window, dialog, views);
	gtk_widget_show (page);

	gtk_notebook_prepend_page (print_dialog_get_notebook (dialog),
				   page,
				   gtk_label_new (_("Select views")));

	g_object_set_data (G_OBJECT (dialog), "window", window);

	return dialog;
}	

static GtkWidget *
print_dialog_create_page (PlannerWindow *window,
			  GtkWidget     *dialog,
			  GList         *views)
{
	GtkWidget   *outer_vbox, *vbox;
	GtkWidget   *hbox;
	GtkWidget   *w;
	GList       *l;
	GList       *buttons = NULL;
	gchar       *str;
	gboolean     state;
	GConfClient *gconf_client;
	
	outer_vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (outer_vbox), 8);
	
	str = g_strconcat ("<b>", _("Select the views to print:"), "</b>", NULL); 
	w = gtk_label_new (str);
	g_free (str);
	gtk_box_pack_start (GTK_BOX (outer_vbox), w, FALSE, FALSE, 0);
	gtk_label_set_use_markup (GTK_LABEL (w), TRUE);
	gtk_misc_set_alignment (GTK_MISC (w), 0, 0.5);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (outer_vbox), hbox, TRUE, TRUE, 0);

	w = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 0);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

/*	w = gtk_check_button_new_with_mnemonic (_("Project summary"));
	gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);
	g_object_set_data (G_OBJECT (dialog), "summary-button", w);
*/
	
	gconf_client = planner_application_get_gconf_client ();

	for (l = views; l; l = l->next) {
		w = gtk_check_button_new_with_label (planner_view_get_label (l->data));
		gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);
		g_object_set_data (G_OBJECT (w), "view", l->data);

		str = g_strdup_printf ("/apps/planner/views/%s/print_enabled", 
				       planner_view_get_name (l->data));
		state = gconf_client_get_bool (gconf_client, str, NULL);
		g_free (str);

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), state);
		
		buttons = g_list_prepend (buttons, w);
	}

	buttons = g_list_reverse (buttons);
	g_object_set_data (G_OBJECT (dialog), "buttons", buttons);
	
	gtk_widget_show_all (outer_vbox);
	
	return outer_vbox;
}

GList *
planner_print_dialog_get_print_selection (GtkDialog *dialog,
					  gboolean  *summary)
{
	GtkToggleButton *button;
	GList           *buttons, *l;
	GList           *views = NULL;
	PlannerView     *view;
	GConfClient     *gconf_client;
	gchar           *str;
	PlannerWindow   *window;

	g_return_val_if_fail (GTK_IS_DIALOG (dialog), NULL);
	
/*	button = g_object_get_data (G_OBJECT (dialog), "summary-button");
	if (summary) {
		*summary = gtk_toggle_button_get_active (button);
	}
*/

	window = g_object_get_data (G_OBJECT (dialog), "window");
	
	gconf_client = planner_application_get_gconf_client ();
	
	buttons = g_object_get_data (G_OBJECT (dialog), "buttons");
	for (l = buttons; l; l = l->next) {
		button = l->data;

		view = g_object_get_data (G_OBJECT (l->data), "view");

		if (gtk_toggle_button_get_active (button)) {
			views = g_list_prepend (views, view);
		}

		str = g_strdup_printf ("/apps/planner/views/%s/print_enabled", 
				       planner_view_get_name (view));
		gconf_client_set_bool (gconf_client,
				       str,
				       gtk_toggle_button_get_active (button),
				       NULL);
		g_free (str);
	}

	return views;
}
		
/*
 * Eek! Hack alert! Hopefully we'll get custom pages in libgnomeprintui soon.
 */
static GtkNotebook *
print_dialog_get_notebook (GtkWidget *container)
{
	GList       *children, *l;
	GtkNotebook *notebook;

	children = gtk_container_get_children (GTK_CONTAINER (container));

	for (l = children; l; l = l->next) {
		if (GTK_IS_NOTEBOOK (l->data)) {
			notebook = l->data;

			g_list_free (children);

			return notebook;
		}
		else if (GTK_IS_CONTAINER (l->data)) {
			notebook = print_dialog_get_notebook (l->data);
			if (notebook) {
				return notebook;
			}
		}
	}

	g_list_free (children);
	
	return NULL;
}

