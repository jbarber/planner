/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003-2004 Imendio HB
 * Copyright (C) 2004 Lincoln Phipps <lincoln.phipps@openmutual.net>
 * Copyright (C) 2003 CodeFactory AB
 * Copyright (C) 2003 Richard Hult <richard@imendio.com>
 * Copyright (C) 2003 Mikael Hallendal <micke@imendio.com>
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
#include <string.h>
#include <glib.h>
#include <glade/glade.h>
#include <gtk/gtk.h>
#include <libgnome/gnome-i18n.h>
#include "planner-conf.h"
#include "planner-window.h"
#include "planner-plugin.h"

#define GCONF_PATH "/apps/planner"

struct _PlannerPluginPriv {
	PlannerWindow *main_window;
	GtkWidget     *dialog;
	GtkWidget     *local_rbutton;
	GtkWidget     *local_fileentry;
	GtkWidget     *server_rbutton;
	GtkWidget     *server_entry;
};

static void xml_planner_plugin_export (GtkAction         *action,
				       gpointer           user_data);
void        plugin_init               (PlannerPlugin     *plugin,
				       PlannerWindow     *main_window);
void        plugin_exit               (PlannerPlugin     *plugin);


static GtkActionEntry action_entries[] = {
	{ "XML Planner Export", NULL,
	  N_("Planner 0.11 Format"), NULL,
	  N_("Export project to a file suitable for Planner 0.11"),
	  G_CALLBACK (xml_planner_plugin_export) },
};
static guint n_action_entries = G_N_ELEMENTS (action_entries);



static gchar *
get_last_dir (void)
{
	gchar       *last_dir;
	
	last_dir = planner_conf_get_string ("/general/last_dir", NULL);
	
	if (last_dir == NULL) {
		last_dir = g_strdup (g_get_home_dir ());
	}
	
	if (last_dir[strlen (last_dir)] != G_DIR_SEPARATOR) {
		gchar *tmp;
		
		tmp = g_strconcat (last_dir, G_DIR_SEPARATOR_S, NULL);
		g_free (last_dir);
		last_dir = tmp;
	}

	return last_dir;
}

static void
xml_planner_plugin_export (GtkAction         *action,
			   gpointer           user_data)
{
	PlannerPluginPriv *priv = PLANNER_PLUGIN (user_data)->priv;
	MrpProject        *project;
	GError            *error = NULL;
	GtkWidget         *file_sel;
	GtkWidget         *dialog;
	gint               response;
	const gchar       *filename = NULL;
	gchar             *real_filename;
	gchar             *last_dir;

	file_sel = gtk_file_selection_new (_("Export"));

	last_dir = get_last_dir ();
	gtk_file_selection_set_filename (GTK_FILE_SELECTION (file_sel), last_dir);
	g_free (last_dir);

	gtk_window_set_modal (GTK_WINDOW (file_sel), TRUE);

	while (TRUE) {
		response = gtk_dialog_run (GTK_DIALOG (file_sel));
		if (response != GTK_RESPONSE_OK) {
			gtk_widget_destroy (file_sel);
			return;
		}
		
		filename = gtk_file_selection_get_filename (
			GTK_FILE_SELECTION (file_sel));


		if (!strstr (filename, ".mrproject") || !strstr (filename, ".planner")) {
			real_filename = g_strconcat (filename, ".mrproject", NULL);
		} else {
			real_filename = g_strdup (filename);
		}
	
		if (g_file_test (real_filename, G_FILE_TEST_EXISTS)) {
			dialog = gtk_message_dialog_new (GTK_WINDOW (priv->main_window),
							 GTK_DIALOG_MODAL |
							 GTK_DIALOG_DESTROY_WITH_PARENT,
							 GTK_MESSAGE_WARNING,
							 GTK_BUTTONS_YES_NO,
							 _("File \"%s\" exists, "
							   "do you want to overwrite it?"),
							 real_filename);
			
			response = gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
			
			switch (response) {
			case GTK_RESPONSE_YES:
				break;
			default:
				g_free (real_filename);
				continue;
			}
		}

		gtk_widget_hide (file_sel);
		break;
	} 

	project = planner_window_get_project (priv->main_window);

	if (!mrp_project_export (project, real_filename,
				 "Planner XML pre-0.12",
				 TRUE,
				 &error)) {
		g_warning ("Error while export to Planner XML: %s", error->message);
	}

	last_dir = g_path_get_dirname (real_filename);
	planner_conf_set_string ("/general/last_dir", last_dir, NULL);
	g_free (last_dir);

	g_free (real_filename);

	gtk_widget_destroy (file_sel);
}

G_MODULE_EXPORT void 
plugin_init (PlannerPlugin *plugin, PlannerWindow *main_window)
{
	PlannerPluginPriv *priv;
	GtkUIManager      *ui;
	GtkActionGroup    *actions;
	GError            *error = NULL;
	
	priv = g_new0 (PlannerPluginPriv, 1);
	plugin->priv = priv;
	priv->main_window = main_window;

	/* Create the actions, get the ui manager and merge the whole */
	actions = gtk_action_group_new ("XML plugin actions");
	gtk_action_group_set_translation_domain (actions, GETTEXT_PACKAGE);

	gtk_action_group_add_actions (actions, action_entries, n_action_entries, plugin);

	ui = planner_window_get_ui_manager (main_window);
	gtk_ui_manager_insert_action_group (ui, actions, 0);

	if (!gtk_ui_manager_add_ui_from_file (ui, DATADIR"/planner/ui/xml-planner-plugin.ui", &error)) {
		g_message ("Building menu failed: %s", error->message);
		g_message ("Couldn't load: %s",DATADIR"/planner/ui/xml-planner-plugin.ui");
		g_error_free (error);
	}
	gtk_ui_manager_ensure_update(ui);
}

G_MODULE_EXPORT void 
plugin_exit (PlannerPlugin *plugin) 
{
}
