/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003-2004 Imendio HB
 * Copyright (C) 2003 CodeFactory AB
 * Copyright (C) 2003 Richard Hult <richard@imendio.com>
 * Copyright (C) 2003 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2003 Alvaro del Castillo <acs@barrapunto.com>
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
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <bonobo/bonobo-ui-component.h>
#include <bonobo/bonobo-ui-util.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-entry.h>
#include <glade/glade.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <libgda/libgda.h>
#include "planner-window.h"
#include "planner-application.h"
#include "planner-plugin.h"

#define d(x)

#define SERVER     "sql-plugin-server"
#define DATABASE   "sql-plugin-database"
#define REVISION   "sql-plugin-revision"
#define LOGIN      "sql-plugin-login"
#define PASSWORD   "sql-plugin-password"
#define PROJECT_ID "sql-plugin-project-id"

#define GCONF_PATH "/apps/planner/plugins/sql"

#define STOP_ON_ERR GDA_COMMAND_OPTION_STOP_ON_ERRORS

typedef struct {
	GtkWidget *open_dialog;
} SQLPluginPriv; 

static gint     sql_plugin_retrieve_project_id (PlannerPlugin      *plugin,
						gchar              *server,
						gchar              *port,
						gchar              *database,
						gchar              *login,
						gchar              *password);
static gboolean sql_plugin_retrieve_db_values  (PlannerPlugin      *plugin,
						const gchar        *title,
						gchar             **server,
						gchar             **port,
						gchar             **db,
						gchar             **user,
						gchar             **password);
static void     sql_plugin_open                (BonoboUIComponent  *component,
						gpointer            user_data,
						const gchar        *cname);
static void     sql_plugin_save                (BonoboUIComponent  *component,
						gpointer            user_data,
						const gchar        *cname);
static GdaDataModel * 
                sql_execute_query              (GdaConnection      *con, 
					        gchar              *query);
void            plugin_init                    (PlannerPlugin      *plugin,
						PlannerWindow      *main_window);
void            plugin_exit                    (void);



enum {
	COL_ID,
	COL_NAME,
	COL_PHASE,
	COL_REVISION
};

static BonoboUIVerb verbs[] = {
	BONOBO_UI_VERB ("Open", sql_plugin_open),
	BONOBO_UI_VERB ("Save", sql_plugin_save),
	BONOBO_UI_VERB_END
};


/**
 * Helper to execute a SQL query using GDA
 */
static GdaDataModel *
sql_execute_query (GdaConnection *con, gchar *query)
{
	GdaCommand   *cmd;
	GdaDataModel *res;

	cmd = gda_command_new (query, GDA_COMMAND_TYPE_SQL, STOP_ON_ERR);
	res = gda_connection_execute_single_command  (con, cmd, NULL);
	gda_command_free (cmd);
	return res;
}


/**
 * Helper to get an int.
 */
static gint
get_int (GdaDataModel *res, gint row, gint column)
{
	gchar    *str;
	GdaValue *value;
	gint      i;

	value = (GdaValue *) gda_data_model_get_value_at (res, column, row);
	if (value == NULL) {
		g_warning ("Failed to get a value: (%d,%d)", column, row);
		return INT_MAX;
	}
	
	str = gda_value_stringify (value);
	i = strtol (str, NULL, 10);
	g_free (str);

	return i;
}

/**
 * Helper to get an UTF-8 string.
 */
static gchar *
get_string (GdaDataModel *res, gint row, gint column)
{
	gchar    *str;
	gchar    *ret;
	gsize     len;
	GdaValue *value;
	
	value = (GdaValue *) gda_data_model_get_value_at (res, column, row);
	if (value == NULL) {
		g_warning ("Failed to get a value: (%d,%d)", column, row);
		return "";
	}
	
	str = gda_value_stringify (value);
	len = strlen (str);
	
	if (g_utf8_validate (str, len, NULL)) {
		return str;
	}

	/* First, try to convert to UTF-8 from the current locale. */
	ret = g_locale_to_utf8 (str, len, NULL, NULL, NULL);

	if (!ret) {
		/* If that fails, try to convert to UTF-8 from ISO-8859-1. */
		ret = g_convert (str, len, "UTF-8", "ISO-8859-1", NULL, NULL, NULL);
	}

	if (!ret) {
		/* Give up. */
		ret = g_strdup (_("Invalid Unicode"));
	}

	g_free (str);
	
	return ret;
}

/**
 * Helper that copies a string or returns NULL for strings only containing
 * whitespace.
 */
static gchar *
strdup_null_if_empty (const gchar *str)
{
	gchar *tmp;

	if (!str) {
		return NULL;
	}

	tmp = g_strstrip (g_strdup (str));
	if (tmp[0] == 0) {
		g_free (tmp);
		return NULL;
	}
		
	return tmp;
}

/**
 * Creates an SQL URI.
 */
static gchar *
create_sql_uri (const gchar *server,
		const gchar *port,
		const gchar *database,
		const gchar *login,
		const gchar *password,
		gint         project_id)
{
	GString *string;
	gchar   *str;

	string = g_string_new ("sql://");
	
	if (server) {
		if (login) {
			g_string_append (string, login);
			
			if (password) {
				g_string_append_c (string, ':');
				g_string_append (string, password);
			}
			
			g_string_append_c (string, '@');
		}
		
		g_string_append (string, server);
		
		if (port) {
			g_string_append_c (string, ':');
			g_string_append (string, port);
		}
	}

	g_string_append_c (string, '#');

	g_string_append_printf (string, "db=%s", database);
	
	if (project_id != -1) {
		g_string_append_printf (string, "&id=%d", project_id);
	}
	
	str = string->str;
	g_string_free (string, FALSE);
	
	return str;
}

static void
show_error_dialog (PlannerPlugin *plugin,
		   const gchar   *str)
{
	GtkWindow *window;
	GtkWidget *dialog;
	gint       response;

	window = GTK_WINDOW (plugin->main_window);
	
	dialog = gtk_message_dialog_new (window,
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_CLOSE,
					 str);

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

static void
selection_changed_cb (GtkTreeSelection *selection, GtkWidget *ok_button)
{
	gboolean sensitive = FALSE;
	
	if (gtk_tree_selection_count_selected_rows (selection) > 0) {
		sensitive = TRUE;
	}
	
	gtk_widget_set_sensitive (ok_button, sensitive);
}

static void
row_activated_cb (GtkWidget         *tree_view,
		  GtkTreePath       *path,
		  GtkTreeViewColumn *column,
		  GtkWidget         *ok_button)
{
	gtk_widget_activate (ok_button);
}

/**
 * Display a list with projects and let the user select one. Returns the project
 * id of the selected one.
 */
static gint
sql_plugin_retrieve_project_id (PlannerPlugin *plugin,
				gchar         *server,
				gchar         *port,
				gchar         *database,
				gchar         *login,
				gchar         *password)
{
	GdaConnection     *conn;
	GdaDataModel      *res;
	GdaClient         *client;
	gchar             *str;
	GladeXML          *gui;
	GtkWidget         *dialog;
	GtkWidget         *treeview;
	GtkWidget         *ok_button;
	GtkListStore      *liststore;
	GtkCellRenderer   *cell;
	GtkTreeViewColumn *col;
	gint               i;
	gint               response;
	gint               project_id;
	GtkTreeSelection  *selection;
	GtkTreeIter        iter;
	gchar             *db_txt;
	const gchar       *dsn_name = "planner-auto";
	const gchar       *provider = "PostgreSQL";

	db_txt = g_strdup_printf ("DATABASE=%s",database);
	gda_config_save_data_source (dsn_name, 
                                     provider, 
                                     db_txt,
                                     "planner project", login, password);
	g_free (db_txt);

	client = gda_client_new ();
	
	conn = gda_client_open_connection (client, dsn_name, NULL, NULL, 0);

	if (!GDA_IS_CONNECTION (conn)) {
		str = g_strdup_printf (_("Connection to database '%s' failed."), database);
		show_error_dialog (plugin, str);
		g_free (str);
		return -1;
	}

	res = sql_execute_query (conn, "BEGIN");
	if (res == NULL) {
		g_warning ("BEGIN command failed.");
		return -1;
	}
	g_object_unref (res);
	res = NULL;

	res = sql_execute_query (conn,
				 "DECLARE mycursor CURSOR FOR SELECT proj_id, name,"
				 "phase, revision FROM project ORDER by proj_id ASC");

	if (res == NULL) {
		g_warning ("DECLARE CURSOR command failed (project).");
		return -1;
	}
	g_object_unref (res);
	

	res = sql_execute_query (conn, "FETCH ALL in mycursor");

	if (res == NULL) {
		g_warning ("FETCH ALL failed.");
		return -1;
	}

	gui = glade_xml_new (GLADEDIR"/sql.glade", "select_dialog", NULL);
	dialog = glade_xml_get_widget (gui, "select_dialog");
	treeview = glade_xml_get_widget (gui, "project_treeview");
	ok_button = glade_xml_get_widget (gui, "ok_button");

	g_object_unref (gui);
	
	liststore = gtk_list_store_new (4, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
	gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (liststore));

	cell = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (_("ID"),
							cell,
							"text", COL_ID,
							NULL);
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);
	
	cell = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (_("Project"),
							cell,
							"text", COL_NAME,
							NULL);
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);
	
	cell = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (_("Phase"),
							cell,
							"text", COL_PHASE,
							NULL);
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);
	
	cell = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (_("Revision"),
							cell,
							"text", COL_REVISION,
							NULL);
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);
	
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW (treeview));
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE); 
	
	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (selection_changed_cb),
			  ok_button);

	g_signal_connect (treeview,
			  "row_activated",
			  G_CALLBACK (row_activated_cb),
			  ok_button);
	
	for (i = 0; i < gda_data_model_get_n_rows (res); i++) {
		gint   id;
		gchar *name;
		gchar *phase;
		gint   revision;
		
		id = get_int (res, i, 0);
		name = get_string (res, i, 1);
		phase = get_string (res, i, 2);
		revision = get_int (res, i, 3);

		/* FIXME: needs fixing in the database backend. */
		if (strcmp (phase, "NULL") == 0) {
			g_free (phase);
			phase = g_strdup ("");
		}
		
		gtk_list_store_append (GTK_LIST_STORE (liststore), &iter);
		gtk_list_store_set (GTK_LIST_STORE (liststore), 
				    &iter,
				    COL_ID, id,
				    COL_NAME, name,
				    COL_PHASE, phase,
			            COL_REVISION, revision,
				    -1);

		g_free (name);
		g_free (phase);
	}

	if (gda_data_model_get_n_columns (res) == 0) {
		gtk_widget_set_sensitive (ok_button, FALSE);
	}
	
	g_object_unref (res);

	res = sql_execute_query (conn,"CLOSE mycursor");
	
	g_object_unref (res);
	
	gtk_widget_show_all (dialog);
	response = gtk_dialog_run (GTK_DIALOG (dialog));

	project_id = -1;
	
	switch (response) {
	case GTK_RESPONSE_CANCEL:
	case GTK_RESPONSE_DELETE_EVENT:
		break;
	case GTK_RESPONSE_OK:
		if (!gtk_tree_selection_get_selected (selection, NULL, &iter)) {
			break;
		}

		gtk_tree_model_get (GTK_TREE_MODEL (liststore),
				    &iter, 
				    COL_ID, &project_id,
				    -1);
		
		break;
	};

	gtk_widget_destroy (dialog);

	return project_id;
}

static gboolean
sql_plugin_retrieve_db_values (PlannerPlugin  *plugin,
			       const gchar    *title,
			       gchar         **server,
			       gchar         **port,
			       gchar         **database,
			       gchar         **login,
			       gchar         **password)
{
	GladeXML           *gui;
	GtkWidget          *dialog;
	gchar              *str;
	gint                response;
	GtkWidget          *server_entry;
	GtkWidget          *db_entry;
	GtkWidget          *user_entry;
	GtkWidget          *password_entry;
	gboolean            ret;
	PlannerApplication *application;
	GConfClient        *gconf_client;

	application = planner_window_get_application (plugin->main_window);
	
	gconf_client = planner_application_get_gconf_client ();

	gui = glade_xml_new (GLADEDIR"/sql.glade", "open_dialog" , NULL);
	dialog = glade_xml_get_widget (gui, "open_dialog");

	gtk_window_set_title (GTK_WINDOW (dialog), title);

	server_entry = gnome_entry_gtk_entry (
		GNOME_ENTRY (glade_xml_get_widget (gui, "server_entry")));
	db_entry = gnome_entry_gtk_entry (
		GNOME_ENTRY (glade_xml_get_widget (gui, "db_entry")));
	user_entry = gnome_entry_gtk_entry (
		GNOME_ENTRY (glade_xml_get_widget (gui, "user_entry")));
	password_entry = glade_xml_get_widget (gui, "password_entry");

	str = gconf_client_get_string (gconf_client, GCONF_PATH "/server", NULL);
	if (str) {
		gtk_entry_set_text (GTK_ENTRY (server_entry), str);
		g_free (str);
	}

	str = gconf_client_get_string (gconf_client, GCONF_PATH "/database", NULL);
	if (str) {
		gtk_entry_set_text (GTK_ENTRY (db_entry), str);
		g_free (str);
	}

	str = gconf_client_get_string (gconf_client, GCONF_PATH "/username", NULL);
	if (str) {
		gtk_entry_set_text (GTK_ENTRY (user_entry), str);
		g_free (str);
	}
	
	g_object_unref (gui);

	response = gtk_dialog_run (GTK_DIALOG (dialog));

	switch (response) {
	case GTK_RESPONSE_OK:
		*server = strdup_null_if_empty (gtk_entry_get_text (GTK_ENTRY (server_entry)));
		*port = NULL;
		*database = strdup_null_if_empty (gtk_entry_get_text (GTK_ENTRY (db_entry)));
		*login = strdup_null_if_empty (gtk_entry_get_text (GTK_ENTRY (user_entry)));
		*password = strdup_null_if_empty (gtk_entry_get_text (GTK_ENTRY (password_entry)));

		gconf_client_set_string (gconf_client,
					 GCONF_PATH "/server", *server ? *server : "",
					 NULL);
		gconf_client_set_string (gconf_client,
					 GCONF_PATH "/database", *database ? *database : "",
					 NULL);
		gconf_client_set_string (gconf_client,
					 GCONF_PATH "/username", *login ? *login : "",
					 NULL);

		ret = TRUE;
		break;

	default:
		ret = FALSE;
		break;
	}

	gtk_widget_destroy (dialog);
		
	return ret;
}

static void
sql_plugin_open (BonoboUIComponent *component,
		 gpointer           user_data,
		 const gchar       *cname)
{
	PlannerPlugin      *plugin = user_data;
	PlannerApplication *application;
	GtkWidget          *window;
	MrpProject         *project;
	gchar              *server = NULL;
	gchar              *port = NULL;
	gchar              *database = NULL;
	gchar              *login = NULL;
	gchar              *password = NULL;
	gint                project_id = -1;
	gchar              *uri = NULL;
	GError             *error = NULL;

	if (!sql_plugin_retrieve_db_values (plugin,
					    _("Open from Database"),
					    &server,
					    &port,
					    &database,
					    &login,
					    &password)) {
		return;
	}
	
	project_id = sql_plugin_retrieve_project_id (plugin,
						     server,
						     port,
						     database,
						     login,
						     password);
	if (project_id == -1) {
		goto fail;
	}
	
	/* Note: The project can change or disappear between the call above and
	 * below. We handle that case though.
	 */

	uri = create_sql_uri (server, port, database, login, password, project_id);

	project = planner_window_get_project (plugin->main_window);
	window = GTK_WIDGET (plugin->main_window);
	application = planner_window_get_application (plugin->main_window);
	
	if (mrp_project_is_empty (project)) {
		GObject *object = G_OBJECT (window);
		
		if (!mrp_project_load (project, uri, &error)) {
			show_error_dialog (plugin, error->message);
			g_clear_error (&error);
			goto fail;
		}

		/* Note: Those aren't used for anything right now. */
		g_object_set_data_full (object, SERVER, server, g_free);
		g_object_set_data_full (object, DATABASE, database, g_free);
		g_object_set_data_full (object, LOGIN, login, g_free);
		g_object_set_data_full (object, PASSWORD, password, g_free);

		g_free (uri);
		
		return;
	} else {
		GObject *object;
		
		window = planner_application_new_window (application);
		project = planner_window_get_project (PLANNER_WINDOW (window));
		
		object = G_OBJECT (window);
		
		/* We must get the new plugin object for the new window,
		 * otherwise we'll pass the wrong window around... a bit
		 * hackish.
		 */
		plugin = g_object_get_data (G_OBJECT (window), "sql-plugin");
		
		if (!mrp_project_load (project, uri, &error)) {
			g_warning ("Error: %s", error->message);
			g_clear_error (&error);
			gtk_widget_destroy (window);
			goto fail;
		}
		
		g_object_set_data_full (object, SERVER, server, g_free);
		g_object_set_data_full (object, DATABASE, database, g_free);
		g_object_set_data_full (object, LOGIN, login, g_free);
		g_object_set_data_full (object, PASSWORD, password, g_free);

		g_free (uri);

		gtk_widget_show_all (window);
		return;
	}

 fail:
	g_free (server);
	g_free (port);
	g_free (database);
	g_free (login);
	g_free (password);
	g_free (uri);
}

static void
sql_plugin_save (BonoboUIComponent *component,
		 gpointer           user_data,
		 const gchar       *cname)
{
	PlannerPlugin  *plugin = user_data;
	MrpProject    *project;
	GObject       *object;
	gchar         *server = NULL;
	gchar         *port = NULL;
	gchar         *database = NULL;
	gchar         *login = NULL;
	gchar         *password = NULL;
	gchar         *uri = NULL;
	GError        *error = NULL;
		
	project = planner_window_get_project (plugin->main_window);

	if (!sql_plugin_retrieve_db_values (plugin,
					    _("Save to Database"),
					    &server,
					    &port,
					    &database,
					    &login,
					    &password)) {
		return;
	}

	/* This code is prepared for getting support for selecting a project to
	 * save over. Needs finishing though. Pass project id -1 for now (always
	 * create a new project).
	 */
	uri = create_sql_uri (server, port, database, login, password, -1);

	if (!mrp_project_save_as (project, uri, FALSE, &error)) {
		show_error_dialog (plugin, error->message);
		g_clear_error (&error);
		goto fail;
	}
	
	g_free (uri);
		
	object = G_OBJECT (plugin->main_window);
	
	g_object_set_data_full (object, SERVER, server, g_free);
	g_object_set_data_full (object, DATABASE, database, g_free);
	g_object_set_data_full (object, LOGIN, login, g_free);
	g_object_set_data_full (object, PASSWORD, password, g_free);
	
	return;

fail:
	g_free (server);
	g_free (port);
	g_free (database);
	g_free (login);
	g_free (password);
	g_free (uri);
}

G_MODULE_EXPORT void 
plugin_exit (void) 
{
}

G_MODULE_EXPORT void 
plugin_init (PlannerPlugin *plugin,
	     PlannerWindow *main_window)
{
	BonoboUIContainer *ui_container;
	BonoboUIComponent *ui_component;
	SQLPluginPriv     *priv;
	gint               i = -1;
	
	priv = g_new0 (SQLPluginPriv, 1);

	g_object_set_data (G_OBJECT (main_window), 
			   PROJECT_ID,
			   GINT_TO_POINTER (i));
	g_object_set_data (G_OBJECT (main_window), 
			   "sql-plugin-revision",
			   GINT_TO_POINTER (i));

	g_object_set_data (G_OBJECT (main_window), 
			   "sql-plugin",
			   plugin);
	
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
			       "/planner/ui/sql-plugin.ui",
			       "sqlplugin",
			       NULL);
	
	bonobo_ui_component_thaw (ui_component, NULL);
}

