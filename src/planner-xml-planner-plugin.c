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
#include <bonobo/bonobo-ui-component.h>
#include <bonobo/bonobo-ui-util.h>
#include <glade/glade.h>
#include <gtk/gtk.h>
#include <libgnome/gnome-i18n.h>
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

static void xml_planner_plugin_export (BonoboUIComponent *component,
				       gpointer           user_data,
				       const gchar       *cname);
void        plugin_init               (PlannerPlugin     *plugin,
				       PlannerWindow     *main_window);
void        plugin_exit               (PlannerPlugin     *plugin);

static BonoboUIVerb verbs[] = {
	BONOBO_UI_VERB ("XML Planner Export", xml_planner_plugin_export),
	BONOBO_UI_VERB_END
};

static gchar *
get_last_dir (void)
{
	GConfClient *gconf_client;
	gchar       *last_dir;
	
	gconf_client = planner_application_get_gconf_client ();
	
	last_dir = gconf_client_get_string (gconf_client,
					    GCONF_PATH "/general/last_dir",
					    NULL);
	
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
xml_planner_plugin_export (BonoboUIComponent *component,
			   gpointer           user_data,
			   const gchar       *cname)
{
	PlannerPluginPriv *priv = PLANNER_PLUGIN (user_data)->priv;
	MrpProject        *project;
	GError            *error = NULL;
	GtkWidget         *file_sel;
	GtkWidget         *dialog;
	gint               response;
	const gchar       *filename = NULL;
	gchar             *last_dir;
	GConfClient       *gconf_client; 

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

		if (g_file_test (filename, G_FILE_TEST_EXISTS)) {
			dialog = gtk_message_dialog_new (GTK_WINDOW (priv->main_window),
							 GTK_DIALOG_MODAL |
							 GTK_DIALOG_DESTROY_WITH_PARENT,
							 GTK_MESSAGE_WARNING,
							 GTK_BUTTONS_YES_NO,
							 _("File \"%s\" exists, "
							   "do you want to overwrite it?"),
							 filename);
			
			response = gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
			
			switch (response) {
			case GTK_RESPONSE_YES:
				break;
			default:
				continue;
			}
		}

		gtk_widget_hide (file_sel);
		break;
	} 

	project = planner_window_get_project (priv->main_window);
	
	if (!mrp_project_export (project, filename,
				 "Planner XML pre-0.12",
				 TRUE,
				 &error)) {
		g_warning ("Error while export to Planner XML: %s", error->message);
	}

	gconf_client = planner_application_get_gconf_client ();
	
	last_dir = g_path_get_dirname (filename);
	gconf_client_set_string (gconf_client,
				 GCONF_PATH "/general/last_dir",
				 last_dir,
				 NULL);
	g_free (last_dir);

	gtk_widget_destroy (file_sel);
}

G_MODULE_EXPORT void 
plugin_init (PlannerPlugin *plugin, PlannerWindow *main_window)
{
	PlannerPluginPriv *priv;
	BonoboUIContainer *ui_container;
	BonoboUIComponent *ui_component;
	
	priv = g_new0 (PlannerPluginPriv, 1);
	plugin->priv = priv;
	priv->main_window = main_window;
	
	ui_container = planner_window_get_ui_container (main_window);
	ui_component = bonobo_ui_component_new_default ();
	
	bonobo_ui_component_set_container (ui_component, 
					   BONOBO_OBJREF (ui_container),
					   NULL);
	bonobo_ui_component_freeze (ui_component, NULL);
	bonobo_ui_component_add_verb_list_with_data (ui_component, 
						     verbs,
						     plugin);
	bonobo_ui_util_set_ui (ui_component,
			       DATADIR,
			       "/planner/ui/xml-planner-plugin.ui",
			       "xmlplannerplugin",
			       NULL);
	
	bonobo_ui_component_thaw (ui_component, NULL);
}

G_MODULE_EXPORT void 
plugin_exit (PlannerPlugin *plugin) 
{
}
