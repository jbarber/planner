/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003 Imendio HB
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
#include <glib.h> 
#include <string.h>
#include <bonobo/bonobo-ui-component.h>
#include <bonobo/bonobo-ui-util.h>
#include <glade/glade.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkstock.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-file-entry.h>
#include "planner-window.h"
#include "planner-plugin.h"

struct _PlannerPluginPriv {
	PlannerWindow *main_window;
	GtkWidget     *dialog;
	GtkWidget     *local_rbutton;
	GtkWidget     *local_fileentry;
	GtkWidget     *server_rbutton;
	GtkWidget     *server_entry;
};

static void html_plugin_export                (BonoboUIComponent *component,
					       gpointer           user_data,
					       const gchar       *cname);
static void html_plugin_ok_button_clicked     (GtkButton         *button,
					       PlannerPlugin     *plugin);
static void html_plugin_cancel_button_clicked (GtkButton         *button,
					       PlannerPlugin     *plugin);
static void html_plugin_local_toggled         (GtkToggleButton   *button,
					       PlannerPlugin     *plugin);
static void html_plugin_server_toggled        (GtkToggleButton   *button,
					       PlannerPlugin     *plugin);
static void html_plugin_do_local_export       (PlannerPlugin     *plugin,
					       const gchar       *path);


void        plugin_init                       (PlannerPlugin     *plugin,
					       PlannerWindow     *main_window);
void        plugin_exit                       (PlannerPlugin     *plugin);


static BonoboUIVerb verbs[] = {
	BONOBO_UI_VERB ("HTML Export", html_plugin_export),
	BONOBO_UI_VERB_END
};

static void
html_plugin_export (BonoboUIComponent *component,
		    gpointer           user_data,
		    const gchar       *cname)
{
	PlannerPluginPriv *priv = PLANNER_PLUGIN (user_data)->priv;
	GladeXML          *glade;
	GtkWidget         *ok_button;
	GtkWidget         *cancel_button;

	glade = glade_xml_new (GLADEDIR"/html-output.glade",
			       NULL, NULL);

	priv->dialog = glade_xml_get_widget (glade, "html_dialog");

	gtk_window_set_transient_for (GTK_WINDOW (priv->dialog),
				      GTK_WINDOW (priv->main_window));
	priv->local_rbutton = glade_xml_get_widget (glade,
						     "local_radiobutton");
	priv->local_fileentry = glade_xml_get_widget (glade, 
						      "local_fileentry");
	priv->server_rbutton = glade_xml_get_widget (glade,
						     "server_radiobutton");
	priv->server_entry = glade_xml_get_widget (glade, "server_entry");
	ok_button = glade_xml_get_widget (glade, "ok_button");
	cancel_button = glade_xml_get_widget (glade, "cancel_button");
	
	g_signal_connect (ok_button, "clicked",
			  G_CALLBACK (html_plugin_ok_button_clicked),
			  user_data);
	
	g_signal_connect (cancel_button, "clicked",
			  G_CALLBACK (html_plugin_cancel_button_clicked),
			  user_data);
	
	g_signal_connect (priv->local_rbutton, "toggled",
			  G_CALLBACK (html_plugin_local_toggled),
			  user_data);
	g_signal_connect (priv->server_rbutton, "toggled",
			  G_CALLBACK (html_plugin_server_toggled),
			  user_data);

	gtk_widget_show (priv->dialog);
}

static void 
html_plugin_ok_button_clicked (GtkButton *button, PlannerPlugin *plugin)
{
	PlannerPluginPriv *priv = plugin->priv;
	GtkWidget         *dialog;
	gint               res;
	const gchar       *path;
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->local_rbutton))) {
		path = gnome_file_entry_get_full_path (GNOME_FILE_ENTRY (priv->local_fileentry), FALSE);

		if (!path || strlen (path) == 0) {
			return;
		}
		
		if (g_file_test (path, G_FILE_TEST_IS_DIR)) {
			dialog = gtk_message_dialog_new (GTK_WINDOW (priv->dialog),
							 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							 GTK_MESSAGE_WARNING,
							 GTK_BUTTONS_CLOSE,
							 _("\"%s\" is a directory.\nEnter a filename and try again."),
							 path);
			res = gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
		}
		else if (g_file_test (path, G_FILE_TEST_EXISTS)) {
			dialog = gtk_message_dialog_new (GTK_WINDOW (priv->dialog),
							 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							 GTK_MESSAGE_WARNING,
							 GTK_BUTTONS_YES_NO,
							 _("File \"%s\" exists, do you want to overwrite it?"),
							 path);
			res = gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);

			switch (res) {
			case GTK_RESPONSE_YES:
				html_plugin_do_local_export (plugin,
								    path);
				gtk_widget_destroy (priv->dialog);
				return;
				break;
			case GTK_RESPONSE_NO:
			case GTK_RESPONSE_DELETE_EVENT:
				break;
			default:
				g_assert_not_reached ();
			}
		} else {
			html_plugin_do_local_export (plugin, path);
			gtk_widget_destroy (priv->dialog);
		}
	} else {
		GtkEntry *entry;
		
		entry = GTK_ENTRY (gnome_entry_gtk_entry (GNOME_ENTRY (priv->server_entry)));
			
		path = gtk_entry_get_text (entry);
		if (strlen (path) > 0) {
			html_plugin_do_local_export (plugin, path);
			gtk_widget_destroy (priv->dialog);
		}
	}
}

static void
html_plugin_cancel_button_clicked (GtkButton *button, PlannerPlugin *plugin)
{
	PlannerPluginPriv *priv = plugin->priv;
	
	gtk_widget_destroy (priv->dialog);
}

static void
html_plugin_local_toggled (GtkToggleButton *button, PlannerPlugin *plugin)
{
	PlannerPluginPriv *priv   = plugin->priv;
	gboolean           active = FALSE;
	
	if (gtk_toggle_button_get_active (button)) {
		active = TRUE;
	}
	
	gtk_widget_set_sensitive (priv->local_fileentry, active);
}

static void
html_plugin_server_toggled (GtkToggleButton *button, PlannerPlugin *plugin)
{
	PlannerPluginPriv *priv   = plugin->priv;
	gboolean           active = FALSE;
	
	if (gtk_toggle_button_get_active (button)) {
		active = TRUE;
	}
	
	gtk_widget_set_sensitive (priv->server_entry, active);
}

static void
html_plugin_do_local_export (PlannerPlugin *plugin, const gchar *path)
{
	PlannerPluginPriv *priv = plugin->priv;
	MrpProject        *project;
	GError            *error = NULL;

	project = planner_window_get_project (priv->main_window);
	
	if (!mrp_project_export (project, path,
				 "Planner HTML",
				 TRUE,
				 &error)) {
		g_warning ("Error while export to HTML: %s", error->message);
	}
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
			       "/planner/ui/html-plugin.ui",
			       "htmlplugin",
			       NULL);
	
	bonobo_ui_component_thaw (ui_component, NULL);
}

G_MODULE_EXPORT void 
plugin_exit (PlannerPlugin *plugin) 
{
	/*g_message ("Test exit");*/
}
