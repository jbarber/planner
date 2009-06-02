/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003 CodeFactory AB
 * Copyright (C) 2003 Richard Hult <richard@imendio.com>
 * Copyright (C) 2003 Mikael Hallendal <micke@imendio.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <string.h>
#include <stdlib.h>
#include <gmodule.h>
#include "mrp-error.h"
#include "mrp-storage-module.h"
#include "mrp-private.h"
#include <glib/gi18n.h>
#include "mrp-storage-sql.h"
#include "mrp-sql.h"

static void       storage_sql_init        (MrpStorageSQL       *storage);
static void       storage_sql_class_init  (MrpStorageSQLClass  *class);
static gboolean   storage_sql_load        (MrpStorageModule          *module,
					   const gchar               *uri,
					   GError                   **error);
static gboolean   storage_sql_save        (MrpStorageModule          *module,
					   const gchar               *uri,
					   gboolean                   force,
					   GError                   **error);
static void       storage_sql_set_project (MrpStorageModule          *module,
					   MrpProject                *project);
void              module_init             (GTypeModule               *module);
MrpStorageModule *module_new              (void                      *project);
void              module_exit             (void);



static MrpStorageModuleClass *parent_class;
GType mrp_storage_sql_type = 0;


void
mrp_storage_sql_register_type (GTypeModule *module)
{
	static const GTypeInfo object_info = {
		sizeof (MrpStorageSQLClass),
		(GBaseInitFunc) NULL,
		(GBaseFinalizeFunc) NULL,
		(GClassInitFunc) storage_sql_class_init,
		NULL,           /* class_finalize */
		NULL,           /* class_data */
		sizeof (MrpStorageSQL),
		0,              /* n_preallocs */
		(GInstanceInitFunc) storage_sql_init,
	};

	mrp_storage_sql_type = g_type_module_register_type (
		module,
		MRP_TYPE_STORAGE_MODULE,
		"MrpStorageSQL",
		&object_info, 0);
}

static void
storage_sql_init (MrpStorageSQL *storage)
{
	mrp_sql_init();
}

static void
storage_sql_class_init (MrpStorageSQLClass *klass)
{
	MrpStorageModuleClass *mrp_storage_module_class = MRP_STORAGE_MODULE_CLASS (klass);

	parent_class = MRP_STORAGE_MODULE_CLASS (g_type_class_peek_parent (klass));

	mrp_storage_module_class->set_project = storage_sql_set_project;
	mrp_storage_module_class->load        = storage_sql_load;
	mrp_storage_module_class->save        = storage_sql_save;
	mrp_storage_module_class->to_xml      = NULL;
	mrp_storage_module_class->from_xml    = NULL;
}

G_MODULE_EXPORT void
module_init (GTypeModule *module)
{
	mrp_storage_sql_register_type (module);
}

G_MODULE_EXPORT MrpStorageModule *
module_new (void *project)
{
	return MRP_STORAGE_MODULE (g_object_new (MRP_TYPE_STORAGE_SQL, NULL));
}

G_MODULE_EXPORT void
module_exit (void)
{
}

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

static gboolean
storage_sql_parse_uri (const gchar  *uri,
		       gchar       **server,
		       gchar       **port,
		       gchar       **database,
		       gchar       **login,
		       gchar       **password,
		       gint         *project_id,
		       GError      **error)
{
	const gchar  *p;
	gchar        *location, *args;
	gchar        *who, *host;
	gchar       **strs;
	gint          i;

	*server = NULL;
	*port = NULL;
	*database = NULL;
	*login = NULL;
	*password = NULL;
	*project_id = -1;

	if (strncmp (uri, "sql://", 6) != 0 || !strchr (uri, '#')) {
		g_set_error (error,
			     MRP_ERROR,
			     MRP_ERROR_INVALID_URI,
			     _("Invalid SQL URI (must start with 'sql://' and contain '#')."));
		return FALSE;
	}

	p = uri + 6;

	strs = g_strsplit (p, "#", 2);
	location = g_strdup (strs[0]);
	args = g_strdup (strs[1]);
	g_strfreev (strs);

	if (strchr (location, '@')) {
		strs = g_strsplit (location, "@", 2);
		who = strdup_null_if_empty (strs[0]);
		host = strdup_null_if_empty (strs[1]);
		g_strfreev (strs);
	} else {
		who = NULL;
		host = strdup_null_if_empty (location);
	}
	g_free (location);

	if (who && strchr (who, ':')) {
		strs = g_strsplit (who, ":", 2);
		*login = strdup_null_if_empty (strs[0]);
		*password = strdup_null_if_empty (strs[1]);
		g_strfreev (strs);
	}
	g_free (who);

	if (host && strchr (host, ':')) {
		strs = g_strsplit (host, ":", 2);
		*server = strdup_null_if_empty (strs[0]);
		*port = strdup_null_if_empty (strs[1]);

		g_strfreev (strs);
		g_free (host);
	} else {
		*server = host;
	}

	strs = g_strsplit (args, "&", 0);
	i = 0;
	while (strs[i]) {
		gchar **arg;

		arg = g_strsplit (strs[i], "=", 2);

		if (strcmp (arg[0], "id") == 0) {
			if (strlen (g_strstrip (arg[1])) > 0) {
				*project_id = atoi (arg[1]);
			} else {
				*project_id = -1;
			}
		}
		else if (strcmp (arg[0], "db") == 0) {
			*database = strdup_null_if_empty (arg[1]);
		}

		g_strfreev (arg);
		i++;
	}
	g_strfreev (strs);

	/* FIXME: add this back when done with the plugin move. */
	if (0 && *project_id == -1) {
		g_set_error (error,
			     MRP_ERROR,
			     MRP_ERROR_INVALID_URI,
			     _("Invalid SQL URI (invalid project id)."));
		goto fail;
	}

	if (*database == NULL) {
		g_set_error (error,
			     MRP_ERROR,
			     MRP_ERROR_INVALID_URI,
			     _("Invalid SQL URI (no database name specified)."));
		goto fail;
	}

	return TRUE;

 fail:
	g_free (*server);
	g_free (*port);
	g_free (*database);
	g_free (*login);
	g_free (*password);

	*server = NULL;
	*port = NULL;
	*database = NULL;
	*login = NULL;
	*password = NULL;
	*project_id = -1;

	return FALSE;
}

static void
test_uri_parser (const gchar *uri)
{
	GError *error = NULL;
	gchar  *server, *port, *database, *login, *password;
	gint    project_id;

	g_print ("Test: '%s'\n", uri);

	if (storage_sql_parse_uri (uri,
				   &server,
				   &port,
				   &database,
				   &login,
				   &password,
				   &project_id,
				   &error)) {
		g_print ("Server  : %s\n", server);
		g_print ("Port    : %s\n", port);
		g_print ("Database: %s\n", database);
		g_print ("Login   : %s\n", login);
		g_print ("Password: %s\n", password);
		g_print ("Id      : %d\n", project_id);
	} else {
		g_print ("Error: %s\n", error->message);
		g_clear_error (&error);
	}
}


static gboolean
storage_sql_load (MrpStorageModule *module, const gchar *uri, GError **error)
{
	MrpStorageSQL *sql;
	gchar         *server, *port, *database, *login, *password;
	gint           project_id;

	g_return_val_if_fail (MRP_IS_STORAGE_SQL (module), FALSE);

	sql = MRP_STORAGE_SQL (module);

	if (0) {
		g_print ("==================\n");
		test_uri_parser ("sql://foo:bar@baz:80#db=test&id=12&foo=42");
		g_print ("==================\n");
		test_uri_parser ("sql://#db=sloff&id=12");
		g_print ("==================\n");
		test_uri_parser ("sql://localhost#db=sliff&id=12");
		g_print ("==================\n");
		test_uri_parser ("sql://rhult:sliff.sloff#id=12&db=qwerty");
	}

	if (!storage_sql_parse_uri (uri, &server, &port, &database, &login, &password, &project_id, error)) {
		return FALSE;
	}

	mrp_sql_load_project (sql, server, port, database, login, password, project_id, error);

	return TRUE;
}

static gboolean
storage_sql_save (MrpStorageModule  *module,
		  const gchar       *uri,
		  gboolean           force,
		  GError           **error)
{
	MrpStorageSQL *sql;
	gchar         *server, *port, *database, *login, *password;
	gint           project_id;
	gchar         *new_uri;

	g_return_val_if_fail (MRP_IS_STORAGE_SQL (module), FALSE);

	sql = MRP_STORAGE_SQL (module);

	if (!storage_sql_parse_uri (uri, &server, &port, &database, &login, &password, &project_id, error)) {
		return FALSE;
	}

	if (!mrp_sql_save_project (sql, force, server, port, database, login, password, &project_id, error)) {
		return FALSE;
	}

	new_uri = create_sql_uri (server,
				  port,
				  database,
				  login,
				  password,
				  project_id);

	g_object_set_data_full (G_OBJECT (sql), "uri", new_uri, g_free);

	return TRUE;
}

static void
storage_sql_set_project (MrpStorageModule *module,
			 MrpProject       *project)
{
	MRP_STORAGE_SQL (module)->project = project;
}
