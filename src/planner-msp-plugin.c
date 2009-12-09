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
#include <string.h>
#include <unistd.h>
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libexslt/exslt.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "libplanner/mrp-paths.h"
#include "planner-conf.h"
#include "planner-window.h"
#include "planner-application.h"
#include "planner-plugin.h"

#define CONF_MSP_PLUGIN_LAST_DIR "/plugins/msp_plugin/last_dir"

struct _PlannerPluginPriv {
	GtkActionGroup    *actions;
};

static void msp_plugin_open (GtkAction     *action,
			     gpointer       user_data);
void        plugin_init     (PlannerPlugin *plugin);
void        plugin_exit     (PlannerPlugin *plugin);


static const GtkActionEntry entries[] = {
	{ "MspOpen",  NULL,  N_("MS Project XML..."), NULL, N_("Import an MS Project XML file"),
	  G_CALLBACK (msp_plugin_open)
	}
};

static gboolean
xml_validate (xmlDoc *doc, const gchar *dtd_path)
{
	xmlValidCtxt  cvp;
	xmlDtd       *dtd;
	gboolean      ret_val;

	memset (&cvp, 0, sizeof (cvp));
	dtd = xmlParseDTD (NULL, dtd_path);
	ret_val = xmlValidateDtd (&cvp, doc, dtd);
	xmlFreeDtd (dtd);

        return ret_val;
}

static gboolean
msp_plugin_transform (PlannerPlugin *plugin,
		      const gchar   *input_filename)
{
        xsltStylesheet *stylesheet;
        xmlDoc         *doc;
        xmlDoc         *final_doc;
	gint            fd;
	FILE           *file;
	gchar          *tmp_name, *uri;
	MrpProject     *project;
	gchar          *filename;

        /* libxml housekeeping */
        xmlSubstituteEntitiesDefault (1);
        xmlLoadExtDtdDefaultValue = 1;
        exsltRegisterAll ();

	filename = mrp_paths_get_stylesheet_dir ("msp2planner.xsl");
        stylesheet = xsltParseStylesheetFile (filename);
	g_free (filename);

	doc = xmlParseFile (input_filename);
	if (!doc) {
		xsltFreeStylesheet (stylesheet);
		return FALSE;
	}

        final_doc = xsltApplyStylesheet (stylesheet, doc, NULL);
        xmlFree (doc);

	if (!final_doc) {
		xsltFreeStylesheet (stylesheet);
		return FALSE;
	}

	filename = mrp_paths_get_dtd_dir ("mrproject-0.6.dtd");
	if (!xml_validate (final_doc, filename)) {
		GtkWidget *dialog;

		xsltFreeStylesheet (stylesheet);
		xmlFree (final_doc);

		dialog = gtk_message_dialog_new (GTK_WINDOW (plugin->main_window),
						 GTK_DIALOG_MODAL |
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("Couldn't import file."));
		gtk_widget_show (dialog);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		g_free (filename);

		return FALSE;
	}

	g_free (filename);

	fd = g_file_open_tmp ("planner-msp-XXXXXX", &tmp_name, NULL);
	if (fd == -1) {
		xsltFreeStylesheet (stylesheet);
		xmlFree (final_doc);
		return FALSE;
	}

	file = fdopen (fd, "w");
	if (file == NULL) {
		xsltFreeStylesheet (stylesheet);
		xmlFree (final_doc);
		close (fd);
		g_free (tmp_name);
		return FALSE;
	}

	if (xsltSaveResultToFile (file, final_doc, stylesheet) == -1) {
		xsltFreeStylesheet (stylesheet);
		xmlFree (final_doc);
		fclose (file);
		g_free (tmp_name);
		return FALSE;
	}

	xsltFreeStylesheet (stylesheet);
        xmlFree (final_doc);

	uri = g_filename_to_uri (tmp_name, NULL, NULL);
	if (uri) {
		planner_window_open_in_existing_or_new (plugin->main_window, uri, TRUE);

		project = planner_window_get_project (plugin->main_window) ;
		mrp_project_set_uri (project, NULL);
	}

	unlink (tmp_name);
	fclose (file);

	g_free (tmp_name);
	g_free (uri);

	return TRUE;
}

static gchar *
msp_plugin_get_last_dir (PlannerPlugin *plugin)
{
	gchar *dir;

	dir = planner_conf_get_string (CONF_MSP_PLUGIN_LAST_DIR, NULL);
	if (dir == NULL) {
		dir = g_strdup (g_get_home_dir ());
	}

	return dir;
}

static void
msp_plugin_set_last_dir (PlannerPlugin *plugin,
			 const gchar   *dir)
{
	planner_conf_set_string (CONF_MSP_PLUGIN_LAST_DIR, dir, NULL);
}

static gchar *
msp_plugin_get_filename (PlannerPlugin *plugin)
{
	PlannerWindow     *window;
	GtkWidget         *file_chooser;
	GtkFileFilter     *filter;
	gint               response;
	gchar             *dir;
	gchar             *filename;

	window = plugin->main_window;

	file_chooser = gtk_file_chooser_dialog_new (_("Import a File"),
						    GTK_WINDOW (window),
						    GTK_FILE_CHOOSER_ACTION_OPEN,
						    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						    GTK_STOCK_OPEN, GTK_RESPONSE_OK,
						    NULL);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("XML Files"));
	gtk_file_filter_add_pattern (filter, "*.xml");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_chooser), filter);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All Files"));
	gtk_file_filter_add_pattern (filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (file_chooser), filter);

	dir = msp_plugin_get_last_dir (plugin);
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (file_chooser), dir);
	g_free (dir);

	gtk_window_set_modal (GTK_WINDOW (file_chooser), TRUE);
	gtk_widget_show (file_chooser);

	response = gtk_dialog_run (GTK_DIALOG (file_chooser));
	gtk_widget_hide (file_chooser);

	filename = NULL;
	if (response == GTK_RESPONSE_OK) {
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser));
	}

	gtk_widget_destroy (file_chooser);

	if (filename) {
		dir = g_path_get_dirname (filename);
		msp_plugin_set_last_dir (plugin, dir);
		g_free (dir);
	}

	return filename;
}

static void
msp_plugin_open (GtkAction *action,
		 gpointer   user_data)
{
	PlannerPlugin *plugin = user_data;
	gchar         *filename;

	filename = msp_plugin_get_filename (plugin);
	if (filename) {
		msp_plugin_transform (plugin, filename);
	}
	g_free (filename);
}

G_MODULE_EXPORT void
plugin_exit (PlannerPlugin *plugin)
{
	PlannerPluginPriv *priv;
	GtkUIManager   *ui;

	priv = plugin->priv;

	ui = planner_window_get_ui_manager (plugin->main_window);
	gtk_ui_manager_remove_action_group (ui, priv->actions);
	g_object_unref (priv->actions);

	g_free (priv);
}

G_MODULE_EXPORT void
plugin_init (PlannerPlugin *plugin)
{
	PlannerPluginPriv *priv;
	GtkUIManager      *ui;
	gchar             *filename;

	priv = g_new0 (PlannerPluginPriv, 1);
	plugin->priv = priv;

	priv->actions = gtk_action_group_new ("MSP plugin actions");
	gtk_action_group_set_translation_domain (priv->actions, GETTEXT_PACKAGE);

	gtk_action_group_add_actions (priv->actions,
				      entries,
				      G_N_ELEMENTS (entries),
				      plugin);

	ui = planner_window_get_ui_manager (plugin->main_window);
	gtk_ui_manager_insert_action_group (ui, priv->actions, 0);

	filename = mrp_paths_get_ui_dir ("msp-plugin.ui");
	gtk_ui_manager_add_ui_from_file (ui, filename, NULL);
	g_free (filename);

	gtk_ui_manager_ensure_update (ui);
}

