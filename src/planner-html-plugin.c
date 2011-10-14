/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2005 Imendio AB
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
#include <glib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "libplanner/mrp-paths.h"
#include "planner-window.h"
#include "planner-plugin.h"
#include "planner-util.h"

struct _PlannerPluginPriv {
	GtkActionGroup    *actions;
};


static void html_plugin_export (GtkAction     *action,
				gpointer       user_data);
void        plugin_init        (PlannerPlugin *plugin);
void        plugin_exit        (PlannerPlugin *plugin);



static const GtkActionEntry action_entries[] = {
	{ "HTML Export", NULL,
	  N_("HTML"), NULL,
	  N_("Export project to HTML"),
	  G_CALLBACK (html_plugin_export)
	}
};


static void
html_plugin_show_url (PlannerPlugin *plugin, const char *path)
{
	gchar *url;

	url = g_filename_to_uri (path, NULL, NULL);
	planner_util_show_url (NULL, url);
	g_free (url);
}

static void
html_plugin_export_do (PlannerPlugin *plugin,
		       const gchar   *path,
		       gboolean       show_in_browser)
{
	MrpProject        *project;
	GtkWidget         *dialog;

	project = planner_window_get_project (plugin->main_window);

	if (!mrp_project_export (project, path, "Planner HTML", TRUE, NULL)) {
		dialog = gtk_message_dialog_new (GTK_WINDOW (plugin->main_window),
						 GTK_DIALOG_MODAL |
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("Could not export to HTML"));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}
	else if (show_in_browser) {
		html_plugin_show_url (plugin, path);
	}
}

static void
html_plugin_export (GtkAction *action,
		    gpointer   user_data)
{
	PlannerPlugin     *plugin;
	MrpProject        *project;
	const gchar       *uri;
	gchar             *filename;
	gchar             *basename;
	gint               res;
	GtkWidget         *filechooser;
	GtkWidget         *dialog;
	GtkWidget         *show_button;
	gboolean           show;

	plugin = PLANNER_PLUGIN (user_data);

	filechooser = gtk_file_chooser_dialog_new (_("Export to HTML"),
						   GTK_WINDOW (plugin->main_window),
						   GTK_FILE_CHOOSER_ACTION_SAVE,
						   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						   GTK_STOCK_SAVE, GTK_RESPONSE_OK,
						   NULL);

	gtk_dialog_set_default_response (GTK_DIALOG (filechooser), GTK_RESPONSE_OK);

	project = planner_window_get_project (plugin->main_window);
	uri = mrp_project_get_uri (project);
	if (!uri) {
		gchar *cwd, *tmp;

		cwd = g_get_current_dir ();
		tmp = g_strconcat (_("Unnamed"), ".html", NULL);

		filename = g_build_filename (cwd, tmp, NULL);

		g_free (cwd);
		g_free (tmp);
	} else {
		gchar *tmp;

		if (g_str_has_suffix (uri, ".planner")) {
			tmp = g_strndup (uri,  strlen (uri) - strlen (".planner"));
		}
		else if (g_str_has_suffix (uri, ".mrproject")) {
			tmp = g_strndup (uri,  strlen (uri) - strlen (".mrproject"));
		} else {
			tmp = g_strdup (uri);
		}

		filename = g_strconcat (tmp, ".html", NULL);
		g_free (tmp);
	}

	gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (filechooser),
				       filename);

	basename = g_path_get_basename (filename);

	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (filechooser), basename);

	show_button = gtk_check_button_new_with_label (_("Show result in browser"));
	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (filechooser), show_button);

	g_free (basename);
	g_free (filename);

 try_again:

	res = gtk_dialog_run (GTK_DIALOG (filechooser));
	switch (res) {
	case GTK_RESPONSE_OK:
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (filechooser));

		if (g_file_test (filename, G_FILE_TEST_EXISTS)) {
			dialog = gtk_message_dialog_new (GTK_WINDOW (filechooser),
							 GTK_DIALOG_MODAL |
							 GTK_DIALOG_DESTROY_WITH_PARENT,
							 GTK_MESSAGE_WARNING,
							 GTK_BUTTONS_YES_NO,
							 _("File \"%s\" exists, do you want to overwrite it?"),
							 filename);
			gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);
			res = gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);

			switch (res) {
			case GTK_RESPONSE_YES:
				break;

			case GTK_RESPONSE_NO:
			case GTK_RESPONSE_DELETE_EVENT:
				g_free (filename);
				goto try_again;

			default:
				g_assert_not_reached ();
			}
		}

		show = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (show_button));
		gtk_widget_destroy (filechooser);

		html_plugin_export_do (plugin, filename, show);
		g_free (filename);

		break;
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (filechooser);
		break;
	}
}

G_MODULE_EXPORT void
plugin_init (PlannerPlugin *plugin)
{
	PlannerPluginPriv *priv;
	GtkUIManager      *ui;
	gchar             *filename;

	priv = g_new0 (PlannerPluginPriv, 1);
	plugin->priv = priv;

	priv->actions = gtk_action_group_new ("HTML plugin actions");
	gtk_action_group_set_translation_domain (priv->actions, GETTEXT_PACKAGE);

	gtk_action_group_add_actions (priv->actions,
				      action_entries,
				      G_N_ELEMENTS (action_entries),
				      plugin);

	ui = planner_window_get_ui_manager (plugin->main_window);
	gtk_ui_manager_insert_action_group (ui, priv->actions, 0);

	filename = mrp_paths_get_ui_dir ("html-plugin.ui");
	gtk_ui_manager_add_ui_from_file (ui, filename, NULL);
	g_free (filename);

	gtk_ui_manager_ensure_update (ui);
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
