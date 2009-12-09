/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003-2004 Imendio AB
 * Copyright (C) 2004 Lincoln Phipps <lincoln.phipps@openmutual.net>
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
#include <glib/gi18n.h>
#include <libplanner/mrp-paths.h>
#include "planner-conf.h"
#include "planner-window.h"
#include "planner-plugin.h"

#define CONF_MAIN_LAST_XML_EXPORT_DIR "/plugins/xml_export/last_dir"


struct _PlannerPluginPriv {
	GtkWidget      *dialog;
	GtkWidget      *local_rbutton;
	GtkWidget      *local_fileentry;
	GtkWidget      *server_rbutton;
	GtkWidget      *server_entry;

	GtkActionGroup *actions;
};

static void xml_planner_plugin_export (GtkAction         *action,
				       gpointer           user_data);
void        plugin_init               (PlannerPlugin     *plugin);
void        plugin_exit               (PlannerPlugin     *plugin);


static const GtkActionEntry entries[] = {
	{ "XML Planner Export", NULL,
	  N_("Planner 0.11 Format"), NULL,
	  N_("Export project to a file suitable for Planner 0.11"),
	  G_CALLBACK (xml_planner_plugin_export) }
};

static gchar *
get_last_dir (void)
{
	gchar *last_dir;

	last_dir = planner_conf_get_string (CONF_MAIN_LAST_XML_EXPORT_DIR, NULL);
	if (last_dir == NULL) {
		last_dir = g_strdup (g_get_home_dir ());
	}

	return last_dir;
}

static void
xml_planner_plugin_export (GtkAction *action,
			   gpointer   user_data)
{
	PlannerPlugin     *plugin;
	MrpProject        *project;
	GError            *error = NULL;
	GtkWidget         *file_chooser;
	GtkWidget         *dialog;
	gint               response;
	gchar             *filename = NULL;
	gchar             *real_filename;
	gchar             *last_dir;

	plugin = PLANNER_PLUGIN (user_data);

 try_again:

	file_chooser = gtk_file_chooser_dialog_new (_("Export"),
						    GTK_WINDOW (plugin->main_window),
						    GTK_FILE_CHOOSER_ACTION_SAVE,
						    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						    GTK_STOCK_SAVE, GTK_RESPONSE_OK,
						    NULL);
	gtk_window_set_modal (GTK_WINDOW (file_chooser), TRUE);

	last_dir = get_last_dir ();
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (file_chooser), last_dir);
	g_free (last_dir);

	response = gtk_dialog_run (GTK_DIALOG (file_chooser));
	if (response == GTK_RESPONSE_OK) {
		filename = gtk_file_chooser_get_filename (
			GTK_FILE_CHOOSER (file_chooser));
	}

	gtk_widget_destroy (file_chooser);

	if (filename) {
		if (!g_str_has_suffix (filename, ".mrproject") && !g_str_has_suffix (filename, ".planner")) {
			/* Add the old extension for old format files. */
			real_filename = g_strconcat (filename, ".mrproject", NULL);
		} else {
			real_filename = g_strdup (filename);
		}

		if (g_file_test (real_filename, G_FILE_TEST_EXISTS)) {
			dialog = gtk_message_dialog_new (GTK_WINDOW (plugin->main_window),
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
				goto try_again;
			}
		}
	}

	if (!filename) {
		return;
	}

	project = planner_window_get_project (plugin->main_window);

	if (!mrp_project_export (project, real_filename,
				 "Planner XML pre-0.12",
				 TRUE,
				 &error)) {
		g_warning ("Error while export to Planner XML: %s", error->message);
	}

	last_dir = g_path_get_dirname (real_filename);
	planner_conf_set_string (CONF_MAIN_LAST_XML_EXPORT_DIR, last_dir, NULL);
	g_free (last_dir);

	g_free (real_filename);
	g_free (filename);
}

G_MODULE_EXPORT void
plugin_init (PlannerPlugin *plugin)
{
	PlannerPluginPriv *priv;
	GtkUIManager      *ui;
	gchar		  *filename;

	priv = g_new0 (PlannerPluginPriv, 1);
	plugin->priv = priv;

	/* Create the actions, get the ui manager and merge the whole */
	priv->actions = gtk_action_group_new ("XML plugin actions");
	gtk_action_group_set_translation_domain (priv->actions, GETTEXT_PACKAGE);

	gtk_action_group_add_actions (priv->actions,
				      entries,
				      G_N_ELEMENTS (entries),
				      plugin);

	ui = planner_window_get_ui_manager (plugin->main_window);
	gtk_ui_manager_insert_action_group (ui, priv->actions, 0);

	filename = mrp_paths_get_ui_dir ("xml-planner-plugin.ui");
	gtk_ui_manager_add_ui_from_file (ui, filename, NULL);
	g_free (filename);

	gtk_ui_manager_ensure_update(ui);
}

G_MODULE_EXPORT void
plugin_exit (PlannerPlugin *plugin)
{
	PlannerPluginPriv *priv;
	GtkUIManager      *ui;

	priv = plugin->priv;

	ui = planner_window_get_ui_manager (plugin->main_window);
	gtk_ui_manager_remove_action_group (ui, priv->actions);
	g_object_unref (priv->actions);

	g_free (priv);
}
