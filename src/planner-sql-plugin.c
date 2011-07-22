/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003-2005 Imendio AB
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
#include <glib/gi18n.h>
#include <libgnomeui/gnome-entry.h>
#include <glade/glade.h>
#include <gtk/gtk.h>
#include <libgda/libgda.h>
#include <libplanner/mrp-paths.h>

#include "planner-conf.h"
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

#define CONF_SERVER "/plugins/sql/server"
#define CONF_DATABASE "/plugins/sql/database"
#define CONF_USERNAME "/plugins/sql/username"

#define CONNECTION_FORMAT_STRING "HOST=%s;DB_NAME=%s"

struct _PlannerPluginPriv {
	GtkActionGroup *actions;
};

static gint          sql_plugin_retrieve_project_id (PlannerPlugin  *plugin,
						     gchar          *server,
						     gchar          *port,
						     gchar          *database,
						     gchar          *login,
						     gchar          *password);
static gboolean      sql_plugin_retrieve_db_values  (PlannerPlugin  *plugin,
						     const gchar    *title,
						     gchar         **server,
						     gchar         **port,
						     gchar         **db,
						     gchar         **user,
						     gchar         **password);
static void          sql_plugin_open                (GtkAction      *action,
						     gpointer        user_data);
static void          sql_plugin_save                (GtkAction      *action,
						     gpointer        user_data);
static GdaDataModel *sql_execute_query              (GdaConnection  *con,
						     gchar          *query);
static gboolean      sql_execute_command            (GdaConnection  *con,
						     gchar          *command);
static const gchar * sql_get_last_error             (GdaConnection  *connection);
void                 plugin_init                    (PlannerPlugin  *plugin);
void                 plugin_exit                    (PlannerPlugin  *plugin);


enum {
	COL_ID,
	COL_NAME,
	COL_PHASE,
	COL_REVISION
};

static const GtkActionEntry entries[] = {
	{ "Open",  NULL,  N_("Open from Database..."),
	  NULL, N_("Open a project from a database"),
	  G_CALLBACK (sql_plugin_open) },
	{ "Save",  NULL,  N_("Save to Database"),
	  NULL, N_("Save the current project to a database"),
	  G_CALLBACK (sql_plugin_save) }
};

static gboolean
sql_execute_command (GdaConnection *con, gchar *command)
{
	GdaCommand *cmd;
	GList      *list;
	GError     *error = NULL;

	cmd = gda_command_new (command, GDA_COMMAND_TYPE_SQL, GDA_COMMAND_OPTION_STOP_ON_ERRORS);
	list = gda_connection_execute_command (con, cmd, NULL, &error);
	gda_command_free (cmd);

	while (list) {
		if (list->data) {
			g_object_unref (list->data);
		}
		list = list->next;
	}
	g_list_free (list);

	if(error != NULL) {
		g_error_free (error);
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

static GdaDataModel *
sql_execute_query (GdaConnection *con, gchar *query)
{
	GdaCommand    *cmd;
	GdaDataModel  *result;

	cmd = gda_command_new (query, GDA_COMMAND_TYPE_SQL, GDA_COMMAND_OPTION_STOP_ON_ERRORS);
	result = gda_connection_execute_select_command (con, cmd, NULL, NULL);
	gda_command_free (cmd);

	return result;
}

static const gchar *
sql_get_last_error (GdaConnection *connection)
{
	GList              *list;
	GdaConnectionEvent *error;
	const gchar        *error_txt;

	g_return_val_if_fail (GDA_IS_CONNECTION (connection),
			      _("Can't connect to database server"));

	list = (GList *) gda_connection_get_events (connection);

	if (list == NULL) {
		return _("No errors reported.");
	}

	error = (GdaConnectionEvent *) g_list_last (list)->data;

	/* FIXME: Poor user, she won't get localized messages */
	error_txt = gda_connection_event_get_description (error);

	return error_txt;
}


/**
 * Helper to get an int.
 */
static gint
get_int (GdaDataModel *res, gint row, gint column)
{
	gchar    *str;
	GValue   *value;
	gint      i;

	value = (GValue *) gda_data_model_get_value_at (res, column, row);
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
	GValue   *value;

	value = (GValue *) gda_data_model_get_value_at (res, column, row);
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
					 "%s", str);

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

/* Planner versions:
   1.x is always lower than 2.x.
   0.6 is lower than 0.11
   If 0.11.90 we don't look ".90".
*/
static gboolean
is_newer_version (const gchar *version_new_txt,
		  const gchar *version_old_txt)
{
	guint   subversion_old, subversion_new;
	guint   version_old, version_new;
	gchar **versionv_new, **versionv_old;

	g_return_val_if_fail (version_new_txt != NULL &&
			      version_old_txt != NULL, FALSE);

	version_old = g_ascii_strtod (version_old_txt, NULL);
	version_new = g_ascii_strtod (version_new_txt, NULL);

	if (version_new > version_old) {
		return TRUE;
	}
	else if (version_old > version_new) {
		return FALSE;
	}

	/* Need to check subversion */
	versionv_old = g_strsplit (version_old_txt,".",-1);
	versionv_new = g_strsplit (version_new_txt,".",-1);

	subversion_old = g_ascii_strtod (versionv_old[1], NULL);
	subversion_new = g_ascii_strtod (versionv_new[1], NULL);

	g_strfreev (versionv_new);
	g_strfreev (versionv_old);

	if (subversion_new > subversion_old) {
		return TRUE;
	}
	return FALSE;
}

static gboolean
check_database_tables (GdaConnection *conn,
		       PlannerPlugin *plugin)
{
	GtkWindow    *window;
	GdaDataModel *model;
	GtkWidget    *dialog;
	gint          result;
	gboolean      success;
	GDir*         dir;
	const gchar  *name;
	gboolean      upgradable = FALSE;
	gboolean      create_tables;
	gboolean      can_create_tables = FALSE;
	gchar        *max_version_database;
	gchar        *max_version_upgrade;
	gchar        *upgrade_file = NULL;
	gchar        *database_file = NULL;
	gchar        *database_version = VERSION;
	const gchar  *database_name;
	gboolean      retval = FALSE;
	gchar        *sql_dir = mrp_paths_get_sql_dir ();

	max_version_database = g_strdup ("0.0");
	max_version_upgrade = g_strdup ("0.0");
	database_name = gda_connection_get_database (conn);

	window = GTK_WINDOW (plugin->main_window);

	/* Try to get the database version */
	model = sql_execute_query (conn, "SELECT value FROM property_global WHERE prop_name='database_version'");
	if (model == NULL) {
		create_tables = TRUE;
	} else {
		create_tables = FALSE;
		database_version = get_string (model, 0, 0);
		g_message ("Database version : %s", database_version);
		if (database_version == NULL) {
			database_version = VERSION;
		}
		g_object_unref (model);
	}

	/* Check for tables */
	dir = g_dir_open (sql_dir, 0, NULL);
	while ((name = g_dir_read_name (dir)) != NULL) {
		gchar **namev = NULL, **versionv = NULL;
		gchar  *version;
		gchar  *sql_file;

		if (!g_str_has_suffix (name, ".sql")) {
			continue;
		}

		sql_file = g_build_filename (sql_dir, name, NULL);

		/* Find version between "-" and ".sql" */
		namev = g_strsplit (sql_file,"-",-1);
		/* Upgrade: 2 versions in file */
		if (namev[1] && namev[2]) {
			versionv = g_strsplit (namev[2],".sql",-1);
			if (is_newer_version (versionv[0], namev[1])) {
				if (!strcmp (namev[1], database_version)) {
					upgradable = TRUE;
					if (is_newer_version (versionv[0],
							      max_version_upgrade)) {
						if (upgrade_file) {
							g_free (upgrade_file);
						}
						upgrade_file = g_strdup (sql_file);
						g_free (max_version_upgrade);
						max_version_upgrade = g_strdup (versionv[0]);
					}
				}
			} else {
				g_warning ("Incorrect upgrade file name: %s", sql_file);
			}
		}
		/* Create tables */
		else if (namev[1]) {
			versionv = g_strsplit (namev[1],".sql",-1);
			if (is_newer_version (versionv[0], max_version_database)) {
				if (database_file) {
					g_free (database_file);
				}
				database_file = g_strdup (sql_file);
				g_free (max_version_database);
				max_version_database = g_strdup (versionv[0]);
			}

			can_create_tables = TRUE;
			version = g_strdup (versionv[0]);
			g_free (version);

		} else {
			if (!database_file) {
				database_file = g_strdup (sql_file);
			}
			g_message ("File with no version: %s", sql_file);
			can_create_tables = TRUE;
		}
		if (versionv) {
			g_strfreev (versionv);
		}
		if (namev) {
			g_strfreev (namev);
		}
		g_free (sql_file);
	}

	/* With each database change we need the new complete database description
	   and the upgrades files from different versions */
	if ((is_newer_version (max_version_upgrade, max_version_database) ||
	    is_newer_version (max_version_database, max_version_upgrade))
	    && upgradable) {
		g_warning ("Database file version %s (%s) is different from upgrade file version %s (%s)",
			   max_version_database,
			   database_file,
			   max_version_upgrade,
			   upgrade_file);
		retval = FALSE;
		upgradable = FALSE;
		can_create_tables = FALSE;
	}

	if (!upgradable && !create_tables) {
		retval = TRUE;
	}
	else if (upgradable && !create_tables) {
		gchar *contents;

		dialog = gtk_message_dialog_new (window,
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_QUESTION,
						 GTK_BUTTONS_NONE,
						 _("Database %s needs to be upgraded "
						   "from version %s to version %s.\n"
						   "Please backup database before the upgrade."),
						 database_name, database_version,
						 max_version_upgrade);
		gtk_dialog_add_buttons ((GtkDialog *) dialog,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					_("Upgrade"), GTK_RESPONSE_YES,
					NULL);

		result = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		if (result == GTK_RESPONSE_YES) {
			g_file_get_contents (upgrade_file, &contents, NULL, NULL);
			success = sql_execute_command (conn, contents);
			g_free (contents);
			if (!success) {
				dialog = gtk_message_dialog_new (window,
								 GTK_DIALOG_DESTROY_WITH_PARENT,
								 GTK_MESSAGE_WARNING,
								 GTK_BUTTONS_CLOSE,
								 _("Could not upgrade database %s.\n Upgrade file used: %s."
								   "\n\nDatabase error: \n%s"),
								 database_name, upgrade_file,
								 sql_get_last_error (conn));

				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
				retval = FALSE;
			} else {
				gchar *query;

				sql_execute_command (conn, "DELETE * FROM property_global WHERE prop_name='database_version'");
				query = g_strdup_printf ("INSERT INTO property_global (prop_name, value) VALUES ('database_version','%s')", max_version_upgrade);

				sql_execute_command (conn, query);
				g_free (query);
				retval = TRUE;
			}
		} else {
			retval = FALSE;
		}
		g_free (upgrade_file);
	}

	else if (create_tables && !can_create_tables) {
		g_warning ("Need to create tables but no database file");
		retval = FALSE;
	}

	else if (create_tables && can_create_tables) {
		gchar  *contents;

		g_file_get_contents (database_file, &contents, NULL, NULL);
		success = sql_execute_command (conn, contents);
		g_free (contents);
		if (!success) {
			dialog = gtk_message_dialog_new (window,
							 GTK_DIALOG_DESTROY_WITH_PARENT,
							 GTK_MESSAGE_WARNING,
							 GTK_BUTTONS_CLOSE,
							 _("Can't create tables in database %s"),
							 database_name);

			result = gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
			retval = FALSE;
		} else {
			gchar *query;

			query = g_strdup_printf ("INSERT INTO property_global (prop_name, value) VALUES ('database_version','%s')", max_version_database);

			sql_execute_command (conn, query);
			g_free (query);
			retval = TRUE;
		}
		g_free (database_file);
	}

	g_free (max_version_upgrade);
	g_free (max_version_database);
	return retval;
}

/* Try to create the database */
static gboolean
create_database (const gchar   *dsn_name,
		 const gchar   *host,
		 const gchar   *db_name,
		 PlannerPlugin *plugin)
{
	GtkWidget         *dialog;
	GtkWindow         *window;
	guint              result;
	gboolean           retval;
	GdaConnection     *conn;
	GdaClient         *client;
	GdaDataSourceInfo *dsn;
	gchar             *cnc_string_orig;
	/* FIXME: In postgresql we use template1 as the connection database */
	gchar             *init_database = "template1";
	gchar             *query;
	GError            *error;

	dsn = gda_config_find_data_source (dsn_name);
	cnc_string_orig = dsn->cnc_string;
	retval = FALSE;

	window = GTK_WINDOW (plugin->main_window);

	/* Use same data but changing the database */
	dsn->cnc_string = g_strdup_printf (CONNECTION_FORMAT_STRING, host, init_database);
	gda_config_save_data_source_info (dsn);

	client = gda_client_new ();
	conn = gda_client_open_connection (client, dsn_name, NULL, NULL, 0, &error);
	if (conn == NULL) {
		g_warning ("Can't connect to database server in order to check/create the database: %s", cnc_string_orig);
	} else {
		dialog = gtk_message_dialog_new (window,
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_QUESTION,
						 GTK_BUTTONS_YES_NO,
						 _("Database %s is not setup for Planner. "
						   "Do you want to do that?"),
						 db_name);

		result = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		if (result == GTK_RESPONSE_YES) {
			query = g_strdup_printf ("CREATE DATABASE %s WITH ENCODING = 'UTF8'",
						 db_name);
			sql_execute_command (conn, query);
			g_free (query);
			retval = TRUE;
		} else {
			retval = FALSE;
		}
		gda_connection_close (conn);
		g_object_unref (client);
	}
	g_free (dsn->cnc_string);
	dsn->cnc_string = cnc_string_orig;
	gda_config_save_data_source_info (dsn);

	return retval;
}

/* Test database status: database exists, correct tables, correct version */
static GdaConnection *
sql_get_tested_connection (const gchar   *dsn_name,
			   const gchar   *host,
			   const gchar   *db_name,
			   GdaClient     *client,
			   PlannerPlugin *plugin)
{
	GdaConnection *conn = NULL;
	gboolean       success;
	gchar         *str;
	GError        *error;

	conn = gda_client_open_connection (client, dsn_name, NULL, NULL, 0, &error);

	if (conn == NULL) {
		if (!create_database (dsn_name, host, db_name, plugin)) {
			str = g_strdup_printf (_("Connection to database '%s@%s' failed."),
					       db_name, host);
			show_error_dialog (plugin, str);
			conn = NULL;
		} else {
			conn = gda_client_open_connection (client, dsn_name, NULL, NULL, 0, &error);
		}
	}

	if (conn != NULL) {

		success = sql_execute_command (conn, "SET TIME ZONE 'UTC'");
		if (!success) {
			g_warning ("SET TIME ZONE command failed: %s.",
					sql_get_last_error (conn));
			goto out;
		}

		if (!check_database_tables (conn, plugin)) {
			str = g_strdup_printf (_("Test to tables in database '%s' failed."),
					       db_name);
			show_error_dialog (plugin, str);
			g_free (str);
			goto out;
		}
	}

	/* g_object_unref (client); */
	return conn;

out:
	if (conn) {
		gda_connection_close (conn);
	}

	return NULL;
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
	GdaDataModel      *model;
	gboolean           success;
	GdaClient         *client;
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
	gchar             *filename;

	db_txt = g_strdup_printf (CONNECTION_FORMAT_STRING,server,database);
	gda_config_save_data_source (dsn_name,
                                     provider,
                                     db_txt,
                                     "planner project", login, password, FALSE);
	g_free (db_txt);

	client = gda_client_new ();
	conn = sql_get_tested_connection (dsn_name, server, database, client, plugin);

	if (conn == NULL) {
		return -1;
	}

	success = sql_execute_command (conn, "BEGIN");
	if (!success) {
		g_warning ("BEGIN command failed.");
		return -1;
	}

	success = sql_execute_command (conn,
				       "DECLARE mycursor CURSOR FOR SELECT proj_id, name,"
				       "phase, revision FROM project ORDER by proj_id ASC");

	if (!success) {
		g_warning ("DECLARE CURSOR command failed (project).");
		return -1;
	}

	model = sql_execute_query (conn, "FETCH ALL in mycursor");

	if (model == NULL) {
		g_warning ("FETCH ALL failed.");
		return -1;
	}

	filename = mrp_paths_get_glade_dir ("sql.glade");
	gui = glade_xml_new (filename, "select_dialog", NULL);
	g_free (filename);

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

	for (i = 0; i < gda_data_model_get_n_rows (model); i++) {
		gint   id;
		gchar *name;
		gchar *phase;
		gint   revision;

		id = get_int (model, i, 0);
		name = get_string (model, i, 1);
		phase = get_string (model, i, 2);
		revision = get_int (model, i, 3);

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

	if (gda_data_model_get_n_columns (model) == 0) {
		gtk_widget_set_sensitive (ok_button, FALSE);
	}

	g_object_unref (model);

	sql_execute_command (conn,"CLOSE mycursor");

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
	PlannerApplication *application;
	gchar              *filename;
	GladeXML           *gui;
	GtkWidget          *dialog;
	gchar              *str;
	gint                response;
	GtkWidget          *server_entry;
	GtkWidget          *db_entry;
	GtkWidget          *user_entry;
	GtkWidget          *password_entry;
	gboolean            ret;

	application = planner_window_get_application (plugin->main_window);

	filename = mrp_paths_get_glade_dir ("sql.glade");
	gui = glade_xml_new (filename, "open_dialog" , NULL);
	g_free (filename);

	dialog = glade_xml_get_widget (gui, "open_dialog");

	gtk_window_set_title (GTK_WINDOW (dialog), title);

	server_entry = gnome_entry_gtk_entry (
		GNOME_ENTRY (glade_xml_get_widget (gui, "server_entry")));
	db_entry = gnome_entry_gtk_entry (
		GNOME_ENTRY (glade_xml_get_widget (gui, "db_entry")));
	user_entry = gnome_entry_gtk_entry (
		GNOME_ENTRY (glade_xml_get_widget (gui, "user_entry")));
	password_entry = glade_xml_get_widget (gui, "password_entry");

	str = planner_conf_get_string (CONF_SERVER, NULL);
	if (str) {
		gtk_entry_set_text (GTK_ENTRY (server_entry), str);
		g_free (str);
	}

	str = planner_conf_get_string (CONF_DATABASE, NULL);
	if (str) {
		gtk_entry_set_text (GTK_ENTRY (db_entry), str);
		g_free (str);
	}

	str = planner_conf_get_string (CONF_USERNAME, NULL);
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

		planner_conf_set_string (CONF_SERVER,
					 *server ? *server : "",
					 NULL);

		planner_conf_set_string (CONF_DATABASE,
					 *database ? *database : "",
					 NULL);

		planner_conf_set_string (CONF_USERNAME,
					 *login ? *login : "",
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
sql_plugin_open (GtkAction *action,
		 gpointer   user_data)
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
sql_plugin_save (GtkAction *action,
		 gpointer   user_data)
{
	GdaClient     *client;
	GdaConnection *conn;
	PlannerPlugin *plugin   = user_data;
	MrpProject    *project;
	GObject       *object;
	gchar         *server   = NULL;
	gchar         *port     = NULL;
	gchar         *database = NULL;
	gchar         *login    = NULL;
	gchar         *password = NULL;
	gchar         *uri      = NULL;
	const gchar   *uri_plan = NULL;
	GError        *error    = NULL;
	gchar         *db_txt;
	const gchar   *dsn_name = "planner-auto";
	const gchar   *provider = "PostgreSQL";

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

	db_txt = g_strdup_printf (CONNECTION_FORMAT_STRING,server,database);
	gda_config_save_data_source (dsn_name,
                                     provider,
                                     db_txt,
                                     "planner project", login, password, FALSE);
	g_free (db_txt);
	client = gda_client_new ();
	conn = sql_get_tested_connection (dsn_name, server, database, client, plugin);
	if (conn == NULL) {
		g_object_unref (client);
		return;
	}
	gda_connection_close (conn);
	g_object_unref (client);

	/* This code is prepared for getting support for selecting a project to
	 * save over. Needs finishing though. Pass project id -1 for now (always
	 * create a new project).
	 */
	uri_plan = mrp_project_get_uri (project);

	/* First time project */
	if (uri_plan == NULL) {
		uri = create_sql_uri (server, port, database, login, password, -1);
		if (!mrp_project_save_as (project, uri, FALSE, &error)) {
			show_error_dialog (plugin, error->message);
			g_clear_error (&error);
			goto fail;
		}
		g_free (uri);

	}
	/* Project was in database */
	else if (strncmp (uri_plan, "sql://", 6) == 0) {
		if (!mrp_project_save (project, FALSE, &error)) {
			show_error_dialog (plugin, error->message);
			g_clear_error (&error);
			goto fail;
		}
	}
	/* Project wasn't in database */
	else {
		uri = create_sql_uri (server, port, database, login, password, -1);
		if (!mrp_project_save_as (project, uri, FALSE, &error)) {
			show_error_dialog (plugin, error->message);
			g_clear_error (&error);
			goto fail;
		}
		g_free (uri);
	}

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

G_MODULE_EXPORT void
plugin_init (PlannerPlugin *plugin)
{
	PlannerPluginPriv *priv;
	GtkUIManager      *ui;
	gint               i = -1;
	gchar             *filename;

	priv = g_new0 (PlannerPluginPriv, 1);
	plugin->priv = priv;

	gda_init (PACKAGE, VERSION, 0, NULL);

	g_object_set_data (G_OBJECT (plugin->main_window),
			   PROJECT_ID,
			   GINT_TO_POINTER (i));
	g_object_set_data (G_OBJECT (plugin->main_window),
			   "sql-plugin-revision",
			   GINT_TO_POINTER (i));

	g_object_set_data (G_OBJECT (plugin->main_window),
			   "sql-plugin",
			   plugin);

	/* Create the actions, get the ui manager and merge the whole */
	priv->actions = gtk_action_group_new ("SQL plugin actions");
	gtk_action_group_set_translation_domain (priv->actions, GETTEXT_PACKAGE);

	gtk_action_group_add_actions (priv->actions,
				      entries,
				      G_N_ELEMENTS (entries),
				      plugin);

	ui = planner_window_get_ui_manager (plugin->main_window);
	gtk_ui_manager_insert_action_group (ui, priv->actions, 0);

	filename = mrp_paths_get_ui_dir ("sql-plugin.ui");
	gtk_ui_manager_add_ui_from_file (ui, filename, NULL);
	g_free (filename);

	gtk_ui_manager_ensure_update (ui);
}

