/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003 CodeFactory AB
 * Copyright (C) 2003 Richard Hult <richard@imendio.com>
 * Copyright (C) 2003 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004 Alvaro del Castillo <acs@barrapunto.com> 
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
#include <libgda/libgda.h>
#include <libplanner/planner.h>
#include <libplanner/mrp-intl.h>
#include <libplanner/mrp-private.h>
#include "mrp-storage-sql.h"
#include "mrp-sql.h"

#define d(x)

#define REVISION "sql-storage-revision"

/* Struct to keep calendar data before we can build the tree, create the
 * calendars and insert the in the project.
 */
typedef struct {
	gint    id;
	gint    parent_id;
       	gchar  *name;
	MrpDay *day_mon;
	MrpDay *day_tue;
	MrpDay *day_wed;
	MrpDay *day_thu;
	MrpDay *day_fri;
	MrpDay *day_sat;
	MrpDay *day_sun;
} CalendarData;

/* Struct to keep task data before we can build the tree, and insert the tasks
 * in the project.
 */
typedef struct {
	gint     id;
	gint     parent_id;
       	MrpTask *task;
} TaskData;

/* Struct to add the overriden day type intervals to, as we read them, since we
 * need to add the whole list at once.
 */
typedef struct {
	GList  *intervals;
	MrpDay *day;
} OverriddenDayTypeData;

typedef struct {
	GdaConnection *con;

	MrpProject    *project;
	gint           project_id;

	gint           calendar_id;
	gint           default_group_id;
	gint           phase_id;
	
	GList         *calendars;
	GList         *tasks;

	gint           revision;
	gchar         *last_user;

	MrpTask       *root_task;
	
	/* Maps from database id to project objects. */
	GHashTable *calendar_id_hash;
	GHashTable *group_id_hash;
	GHashTable *resource_id_hash;
	GHashTable *task_id_hash;
	GHashTable *day_id_hash;
	GHashTable *property_type_id_hash;

	/* Backwards mapping. */
	GHashTable *calendar_hash;
	GHashTable *group_hash;
	GHashTable *resource_hash;
	GHashTable *task_hash;
	GHashTable *day_hash;
	GHashTable *property_type_hash;
} SQLData; 

static gint     get_int                       (GdaDataModel         *res,
					       gint                  i,
					       gint                  j);
static gint     get_id                        (GdaDataModel         *res,
					       gint                  i,
					       gint                  j);
static gchar *  get_string                    (GdaDataModel         *res,
					       gint                  i,
					       gint                  j);
static gboolean get_boolean                   (GdaDataModel         *res,
					       gint                  i,
					       gint                  j);
static gfloat   get_float                     (GdaDataModel         *res,
					       gint                  i,
					       gint                  j);
static gboolean is_field                      (GdaDataModel         *res,
					       gint                  j,
					       const gchar          *name);
static gint     get_inserted_id               (SQLData              *data,
					       const gchar          *id_name);
static gint     get_hash_data_as_id           (GHashTable           *hash,
					       gpointer              key);
static gboolean sql_read_project              (SQLData              *data,
					       gint                  proj_id);
static gboolean sql_read_phases               (SQLData              *data);
static gboolean sql_read_property_specs       (SQLData              *data);
static gboolean sql_read_property_values      (SQLData              *data,
					       MrpObject            *object);
static gboolean sql_read_overriden_day_types  (SQLData              *data,
					       gint                  calendar_id);
static gboolean sql_read_overriden_days       (SQLData              *data,
					       gint                  calendar_id);
static gboolean sql_read_day_types            (SQLData              *data);
static gboolean sql_calendar_create           (GNode                *node,
					       SQLData              *data);
static void     dump_calendar_tree            (GNode                *node);
static void     sql_calendar_insert_node      (GHashTable           *hash,
					       GNode                *root,
					       GNode                *node);
static gboolean sql_read_calendars            (SQLData              *data);
static gboolean sql_read_groups               (SQLData              *data);
static gboolean sql_read_resources            (SQLData              *data);
static gboolean sql_read_assignments          (SQLData              *data,
					       gint                  task_id);
static gboolean sql_read_relations            (SQLData              *data,
					       gint                  task_id);
static void     sql_task_insert_node          (GHashTable           *hash,
					       GNode                *root,
					       GNode                *node);
static void     dump_task_tree                (GNode                *node);
static gboolean sql_read_tasks                (SQLData              *data);
static gboolean sql_write_project             (MrpStorageSQL        *storage,
					       SQLData              *data,
					       gboolean              force,
					       GError              **error);
static gboolean sql_write_phases              (SQLData              *data);
static gboolean sql_write_phase               (SQLData              *data);
static gboolean sql_write_property_specs      (SQLData              *data);
static gboolean sql_write_property_values     (SQLData              *data,
					       MrpObject            *object);
static gboolean sql_write_overridden_day_type (SQLData              *data,
					       MrpCalendar          *calendar,
					       MrpDayWithIntervals  *day_ivals);
static gboolean sql_write_overridden_dates    (SQLData              *data,
					       MrpCalendar          *calendar,
					       MrpDateWithDay       *date_day);
static gboolean sql_write_calendars           (SQLData              *data);
static gboolean sql_write_calendar_id         (SQLData              *data);
static gboolean sql_write_groups              (SQLData              *data);
static gboolean sql_write_default_group_id    (SQLData              *data);
static gboolean sql_write_resources           (SQLData              *data);
static gboolean sql_write_tasks               (SQLData              *data);

static GdaDataModel * 
                sql_execute_query             (GdaConnection        *con, 
					       gchar                *query);

#define STOP_ON_ERR     GDA_COMMAND_OPTION_STOP_ON_ERRORS

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

static const gchar *
sql_get_last_error (GdaConnection *connection)
{
	GList       *list;
	GdaError    *error;
	const gchar *error_txt;

	list = (GList *) gda_connection_get_errors (connection);

	error = (GdaError *) g_list_last (list)->data;
      
	/* Poor user, she won't get localized messages */
	error_txt = gda_error_get_description (error);

	return error_txt;
}

static gint
get_int (GdaDataModel *res, gint row, gint column)
{
	GdaValue *value;
	gchar    *str;
	gint      i;
	
	value = (GdaValue *) gda_data_model_get_value_at (res, column, row);
	if (value == NULL) {
		g_warning ("Failed to get a value: (%d,%d)", column, row);
		d(sql_show_result (res));
		return INT_MAX;
	}

	str = gda_value_stringify (value);
	i = strtol (str, NULL, 10);
	g_free (str);

	return i;
}

static gint
get_id (GdaDataModel *res, gint row, gint column)
{
	GdaValue *value;
	gchar    *str;
	gint      i;
	
	value = (GdaValue *) gda_data_model_get_value_at (res, column, row);
	if (value == NULL) {
		g_warning ("Failed to get a value: (%d,%d)", column, row);
		d(sql_show_result (res));
		return INT_MAX;
	}

	str = gda_value_stringify (value);
	if (!str || !str[0]) {
		g_free (str);
		return -1;
	}

	i = strtol (str, NULL, 10);
	g_free (str);

	return i;
}

static gchar *
get_string (GdaDataModel *res, gint row, gint column)
{
	GdaValue *value;
	gsize     len;
	gchar    *ret;
	gchar    *str;
	
	value = (GdaValue *) gda_data_model_get_value_at (res, column, row);
	if (value == NULL) {
		g_warning ("Failed to get a value: (%d,%d)", column, row);
		d(sql_show_result (res));
		return g_strdup ("");
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

static gboolean
get_boolean (GdaDataModel *res, gint row, gint column)
{
	GdaValue *value;
	
	value = (GdaValue *) gda_data_model_get_value_at (res, column, row);
	if (value == NULL) {
		g_warning ("Failed to get a value: (%d,%d)", column, row);
		d(sql_show_result (res));
		return FALSE;
	}
	
	return gda_value_get_boolean (value);
}

static gfloat
get_float (GdaDataModel *res, gint row, gint column)
{
	GdaValue *value;
	gchar    *str;
	gdouble   d;
	
	value = (GdaValue *) gda_data_model_get_value_at (res, column, row);

	if (value == NULL) {
		g_warning ("Failed to get a value: (%d,%d)", column, row);
		d(sql_show_result (res));
		return -1;
	}
	
	str = gda_value_stringify (value);
	d = g_ascii_strtod (str, NULL);
	g_free (str);

	return d;
}

static gboolean
is_field (GdaDataModel *res, gint j, const gchar *name)
{
	const gchar *str;

	str = gda_data_model_get_column_title (res, j);
	
	return str && (strcmp (str, name) == 0);
}

static gint
get_inserted_id (SQLData     *data,
		 const gchar *id_name)
{
	GdaDataModel *res;
	gchar        *query;
	gint          id = -1;

	/* Check which id the field id_name got assigned. */
	query = g_strdup_printf ("DECLARE idcursor CURSOR FOR SELECT "
				 "currval('%s')", id_name);
	res = sql_execute_query (data->con, query);
	g_free (query);

	if (res == NULL) {
		g_warning ("Couldn't get cursor (get_inserted_id) %s.", 
				sql_get_last_error (data->con));
		goto out;
	}
	g_object_unref (res);

	res = sql_execute_query (data->con, "FETCH ALL in idcursor");

	if (res == NULL) {
		g_warning ("FETCH ALL failed (%s) %s.", id_name, 
				sql_get_last_error (data->con));
		goto out;
	}
	
	if (gda_data_model_get_n_rows (res) > 0) {
		id = get_int (res, 0, 0);
	}

	g_object_unref (res);

	res = sql_execute_query (data->con,"CLOSE idcursor");
	
	g_object_unref (res);
	
	
	return id;

 out:
	if (res) {
		g_object_unref (res);
	}

	return -1;
}

static gint
get_hash_data_as_id (GHashTable *hash, gpointer key)
{
	gpointer orig_key, value;

	if (!g_hash_table_lookup_extended (hash, key, &orig_key, &value)) {
		return -1;
	}

	return GPOINTER_TO_INT (value);
}

static gboolean
sql_read_project (SQLData *data, gint proj_id)
{
	gint          n;
	gint          j;
	GdaDataModel *res;
	gchar        *query;
	
	gchar        *name = NULL;
	gchar        *manager = NULL;
	gchar        *company = NULL;
	gchar        *phase = NULL;
	mrptime       project_start = -1;

	/* Find the project to open. */
	query = g_strdup_printf ("DECLARE mycursor CURSOR FOR SELECT "
				 "extract (epoch from proj_start) as proj_start_seconds, "
				 " * FROM project WHERE proj_id=%d", proj_id);
	res = sql_execute_query (data->con, query);
	g_free (query);
	
	if (res == NULL) {
		g_warning ("Couldn't get cursor for project %s.", 
				sql_get_last_error (data->con));
		goto out;
	}
	g_object_unref (res);

	res = sql_execute_query (data->con, "FETCH ALL in mycursor");

	if (res == NULL) {
		g_warning ("FETCH ALL failed %s.", sql_get_last_error (data->con));
		goto out;
	}
	

	if (gda_data_model_get_n_rows (res) == 0) {
		g_warning ("There is no project with the id '%d'.", proj_id);
		goto out;
	}
	
	n = gda_data_model_get_n_columns (res);
	for (j = 0; j < n; j++) {
		if (is_field (res, j, "proj_id")) {
			data->project_id = get_int (res, 0, j);
		}
		else if (is_field (res, j, "name")) {
			name = get_string (res, 0, j);
		}
		else if (is_field (res, j, "manager")) {
			manager = get_string (res, 0, j);
		}
		else if (is_field (res, j, "company")) {
			company = get_string (res, 0, j);
		}
		else if (is_field (res, j, "proj_start_seconds")) {
			project_start = get_int (res, 0, j);
		}
		else if (is_field (res, j, "cal_id")) {
			data->calendar_id = get_int (res, 0, j);
		}
		else if (is_field (res, j, "phase")) {
			phase = get_string (res, 0, j);
		}
		else if (is_field (res, j, "default_group_id")) {
			data->default_group_id = get_id (res, 0, j);
		}
		else if (is_field (res, j, "revision")) {
			data->revision = get_int (res, 0, j);
		}
		else if (is_field (res, j, "last_user")) {
			data->last_user = get_string (res, 0, j);
		}
	}
	g_object_unref (res);

	g_object_set (data->project,
		      "name", name,
		      "manager", manager,
		      "organization", company,
		      "project_start", project_start,
		      "phase", phase,
		      NULL);

	g_free (name);
	g_free (manager);
	g_free (company);
	g_free (phase);
	
	res = sql_execute_query (data->con, "CLOSE mycursor");
	g_object_unref (res);
	

	return TRUE;

 out:
	if (res) {
		g_object_unref (res);
	}
	return FALSE;
}

static gboolean
sql_read_phases (SQLData *data)
{
	gint          n, i, j;
	GdaDataModel *res;
	gchar        *query;
	
	gchar        *name;
	GList        *phases = NULL;
		
	/* Get phases. */
	query = g_strdup_printf ("DECLARE mycursor CURSOR FOR SELECT * FROM phase WHERE proj_id=%d",
				 data->project_id);
	
	res = sql_execute_query (data->con, query);
	g_free (query);
	

	if (res == NULL) {
		g_warning ("DECLARE CURSOR command failed (phase) %s.", 
				sql_get_last_error (data->con));
		goto out;
	}
	g_object_unref (res);

	res = sql_execute_query (data->con, "FETCH ALL in mycursor");
	if (res == NULL) {
		g_warning ("FETCH ALL failed for phase %s.", 
				sql_get_last_error (data->con));
		goto out;
	}
	
	
	n = gda_data_model_get_n_columns (res);
	for (i = 0; i < gda_data_model_get_n_rows (res); i++) {
		name = NULL;
		
		for (j = 0; j < n; j++) {
			if (is_field (res, j, "name")) {
				name = get_string (res, i, j);
			}
		}

		if (name) {
			phases = g_list_prepend (phases, name);
		}
	}

	g_object_unref (res);

	
	res = sql_execute_query (data->con, "CLOSE mycursor");
	g_object_unref (res);
	

	phases = g_list_reverse (phases);
	g_object_set (data->project, "phases", phases, NULL);
	mrp_string_list_free (phases);

	return TRUE;

 out:
	if (res) {
		g_object_unref (res);
	}

	return FALSE;
}

static gboolean
sql_read_property_specs (SQLData *data)
{
	gint             n, i, j;
	GdaDataModel        *res;
	gchar           *query;
	
	gint             property_type_id;
	gchar           *name;
	gchar           *label;
	gchar           *description;
	MrpPropertyType  type;
	MrpProperty     *property;
	GType            owner;
	const gchar     *tmp;
	
	/* Get property types/specs. */
	query = g_strdup_printf ("DECLARE mycursor CURSOR FOR SELECT * FROM property_type WHERE proj_id=%d",
				 data->project_id);
	
	res = sql_execute_query (data->con, query);
	g_free (query);
	

	if (res == NULL) {
		g_warning ("DECLARE CURSOR command failed (propecty_specs) %s.",
				sql_get_last_error (data->con));
		goto out;
	}
	g_object_unref (res);

	res = sql_execute_query (data->con, "FETCH ALL in mycursor");
	if (res == NULL) {
		g_warning ("FETCH ALL failed for property_specs %s.", 
				sql_get_last_error (data->con));
		goto out;
	}
	
	
	n = gda_data_model_get_n_columns (res);
	for (i = 0; i < gda_data_model_get_n_rows (res); i++) {
		name = NULL;
		label = NULL;
		description = NULL;
		property_type_id = -1;
		owner = G_TYPE_INVALID;
		type = MRP_PROPERTY_TYPE_NONE;

		for (j = 0; j < n; j++) {
			if (is_field (res, j, "name")) {
				name = get_string (res, i, j);
			}
			else if (is_field (res, j, "label")) {
				label = get_string (res, i, j);
			}
			else if (is_field (res, j, "descr")) {
				description = get_string (res, i, j);
			}
			else if (is_field (res, j, "owner")) {
				tmp = get_string (res, i, j);

				if (!strcmp (tmp, "task")) {
					owner = MRP_TYPE_TASK;
				}
				else if (!strcmp (tmp, "resource")) {
					owner = MRP_TYPE_RESOURCE;
				}
				else if (!strcmp (tmp, "project")) {
					owner = MRP_TYPE_PROJECT;
				}
			}
			else if (is_field (res, j, "type")) {
				tmp = get_string (res, i, j); 

				if (!strcmp (tmp, "date")) {
					type = MRP_PROPERTY_TYPE_DATE;
				}
				else if (!strcmp (tmp, "duration")) {
					type = MRP_PROPERTY_TYPE_DURATION;
				}
				else if (!strcmp (tmp, "float")) {
					type = MRP_PROPERTY_TYPE_FLOAT;
				} 
				else if (!strcmp (tmp, "int")) {
					type = MRP_PROPERTY_TYPE_INT;
				} 
				else if (!strcmp (tmp, "text")) {
					type = MRP_PROPERTY_TYPE_STRING;
				}
				else if (!strcmp (tmp, "text-list")) {
					type = MRP_PROPERTY_TYPE_STRING_LIST;
				}
				else if (!strcmp (tmp, "cost")) {
					type = MRP_PROPERTY_TYPE_COST;
				}
			}
			else if (is_field (res, j, "proptype_id")) {
				property_type_id = get_int (res, i, j);
			}
		}

		if (type != MRP_PROPERTY_TYPE_NONE &&
		    owner != G_TYPE_INVALID &&
		    !mrp_project_has_property (data->project, owner, name)) {
			property = mrp_property_new (name,
						     type,
						     label,
						     description,
						     TRUE);
			
			mrp_project_add_property (data->project,
						  owner,
						  property,
						  TRUE /* FIXME: user_defined, should 
							  be read from the file */);
					
			g_hash_table_insert (data->property_type_id_hash, GINT_TO_POINTER (property_type_id), property);
		} else {
			/* Properties that are already added (e.g. cost). */
			property = mrp_project_get_property (data->project, name, owner);
			g_hash_table_insert (data->property_type_id_hash, GINT_TO_POINTER (property_type_id), property);
		}
		
		g_free (name);
		g_free (label);
		g_free (description);
	}
	g_object_unref (res);

	res = sql_execute_query (data->con, "CLOSE mycursor");
	g_object_unref (res);
	

	return TRUE;

 out:
	if (res) {
		g_object_unref (res);
	}

	return FALSE;
}	

static gboolean
sql_set_property_value (SQLData     *data,
			       MrpObject   *object,
			       MrpProperty *property,
			       const gchar *value)
{
	const gchar     *name;
	MrpPropertyType  type;
	gint             i;
	gfloat           f;
	mrptime          date;

	name = mrp_property_get_name (property);
	type = mrp_property_get_property_type (property);

	switch (type) {
	case MRP_PROPERTY_TYPE_STRING:
		mrp_object_set (object, name, value, NULL);
		break;
	case MRP_PROPERTY_TYPE_STRING_LIST:
		g_warning ("String list not supported.");
		break;
	case MRP_PROPERTY_TYPE_INT:
		i = atoi (value);
		mrp_object_set (object, name, i, NULL);
		break;
	case MRP_PROPERTY_TYPE_FLOAT:
		f = g_ascii_strtod (value, NULL);
		mrp_object_set (object, name, f, NULL);
		break;
	case MRP_PROPERTY_TYPE_DURATION:
		i = atoi (value);
		mrp_object_set (object, name, i, NULL);
		break;
	case MRP_PROPERTY_TYPE_DATE:
		date = mrp_time_from_string (value, NULL);
		mrp_object_set (object, name, &date, NULL);
		break;
	case MRP_PROPERTY_TYPE_COST:
		f = g_ascii_strtod (value, NULL);
		mrp_object_set (object, name, f, NULL);
		break;
	default:
		g_warning ("Not implemented support for type.");
		return FALSE;
	}

	return TRUE;
}

static gboolean
sql_read_property_values (SQLData   *data,
				 MrpObject *object)
{
	gint         n, i, j;
	GdaDataModel    *res;
	gchar       *query;
	
	const gchar *table;
	const gchar *object_id_name;
	gint         object_id;
	gint         prop_id;
	GList       *prop_ids = NULL, *l;
	gint         prop_type_id;
	MrpProperty *property;
	gchar       *value;

	if (G_OBJECT_TYPE (object) == MRP_TYPE_PROJECT) {
		table = "project_to_property";
		object_id_name = "proj_id";
		object_id = data->project_id;
	}
	else if (G_OBJECT_TYPE (object) == MRP_TYPE_TASK) {
		table = "task_to_property";
		object_id_name = "task_id";
		object_id = get_hash_data_as_id (data->task_hash, object);
	}
	else if (G_OBJECT_TYPE (object) == MRP_TYPE_RESOURCE) {
		table = "resource_to_property";
		object_id_name = "res_id";
		object_id = get_hash_data_as_id (data->resource_hash, object);
	} else {
		g_assert_not_reached ();
		
		table = NULL;
		object_id_name = NULL;
		object_id = -1;
	}
	
	/* Get properties. */
	query = g_strdup_printf ("DECLARE propcursor CURSOR FOR SELECT * "
				 "FROM %s WHERE %s=%d",
				 table, object_id_name, object_id);
	
	res = sql_execute_query (data->con, query);
	g_free (query);
	

	if (res == NULL) {
		g_warning ("DECLARE CURSOR command failed (*_to_property) %s.",
				sql_get_last_error (data->con));
		goto out;
	}
	g_object_unref (res);

	res = sql_execute_query (data->con,"FETCH ALL in propcursor");
	if (res == NULL) {
		g_warning ("FETCH ALL failed for *_to_property %s.", 
				sql_get_last_error (data->con));
		goto out;
	}
	
	
	n = gda_data_model_get_n_columns (res);
	for (i = 0; i < gda_data_model_get_n_rows (res); i++) {
		prop_id = -1;

		for (j = 0; j < n; j++) {
			if (is_field (res, j, "prop_id")) {
				prop_id = get_id (res, i, j);
			}
		}

		prop_ids = g_list_prepend (prop_ids, GINT_TO_POINTER (prop_id));
	}
	g_object_unref (res);

	res = sql_execute_query (data->con, "CLOSE propcursor");
	g_object_unref (res);
	

	/* Get the actual values. */
	for (l = prop_ids; l; l = l->next) {
		prop_id = GPOINTER_TO_INT (l->data);
		
		query = g_strdup_printf ("DECLARE propcursor CURSOR FOR SELECT * "
					 "FROM property WHERE prop_id=%d",
					 prop_id);
		
		res = sql_execute_query (data->con, query);
		g_free (query);
		
		
		if (res == NULL) {
			g_warning ("DECLARE CURSOR command failed (property) %s.",
					sql_get_last_error (data->con));
			goto out;
		}
		g_object_unref (res);
		
		res = sql_execute_query (data->con,"FETCH ALL in propcursor");
		if (res == NULL) {
			g_warning ("FETCH ALL failed for property %s.",
					sql_get_last_error (data->con));
			goto out;
		}
		
		
		n = gda_data_model_get_n_columns (res);
		for (i = 0; i < gda_data_model_get_n_rows (res); i++) {
			prop_type_id = -1;
			value = NULL;
			
			for (j = 0; j < n; j++) {
				if (is_field (res, j, "proptype_id")) {
					prop_type_id = get_id (res, i, j);
				}
				if (is_field (res, j, "value")) {
					value = get_string (res, i, j);
				}
			}

			property = g_hash_table_lookup (data->property_type_id_hash, GINT_TO_POINTER (prop_type_id));

			sql_set_property_value (data, object, property, value);
			g_free (value);
		}
		g_object_unref (res);
		
		res = sql_execute_query (data->con, "CLOSE propcursor");
		g_object_unref (res);
		
		
	}

	g_list_free (prop_ids);
	
	return TRUE;

 out:
	if (res) {
		g_object_unref (res);
	}

	return FALSE;
}

static void
foreach_insert_overridden_day_type (gpointer key,
				    OverriddenDayTypeData *data,
				    MrpCalendar *calendar)
{
	mrp_calendar_day_set_intervals (calendar, data->day, data->intervals);

	g_list_foreach (data->intervals, (GFunc) mrp_interval_unref, NULL);
	g_list_free (data->intervals);
	g_free (data);
}

/* Reads overridden day types, i.e. redefinitions of the time intervals to use
 * for the day types in the given calendar.
 */
static gboolean
sql_read_overriden_day_types (SQLData *data, gint calendar_id)
{
	gint                   n, i, j;
	GdaDataModel              *res;
	gchar                 *query;
	
	gint                   day_type_id;
	mrptime                start, end;
	MrpInterval           *interval;
	MrpDay                *day;
	OverriddenDayTypeData *day_type_data;
	GHashTable            *hash;
	MrpCalendar           *calendar;
	
	/* Get overridden days for the given calendar. */
	query = g_strdup_printf ("DECLARE daycursor CURSOR FOR SELECT "
				 "extract (epoch from start_time) as start_seconds, "
				 "extract (epoch from end_time) as end_seconds, "
				 "* FROM day_interval WHERE cal_id=%d",
				 calendar_id);
	
	res = sql_execute_query (data->con, query);
	g_free (query);
	

	if (res == NULL) {
		g_warning ("DECLARE CURSOR command failed (day_interval) %s.",
				sql_get_last_error (data->con));
		goto out;
	}
	g_object_unref (res);

	res = sql_execute_query (data->con, "FETCH ALL in daycursor");
	if (res == NULL) {
		g_warning ("FETCH ALL failed for day_interval %s.",
				sql_get_last_error (data->con));
		goto out;
	}
	
	hash = g_hash_table_new (NULL, NULL);
	
	n = gda_data_model_get_n_columns (res);
	for (i = 0; i < gda_data_model_get_n_rows (res); i++) {
		day_type_id = -1;
		start = -1;
		end = -1;

		for (j = 0; j < n; j++) {
			if (is_field (res, j, "dtype_id")) {
				day_type_id = get_int (res, i, j);
			}
			else if (is_field (res, j, "start_seconds")) {
				start = get_int (res, i, j);
			}
			else if (is_field (res, j, "end_seconds")) {
				end = get_int (res, i, j);
			}
		}

		day_type_data = g_hash_table_lookup (hash, GINT_TO_POINTER (day_type_id));
		if (!day_type_data) {
			day_type_data = g_new0 (OverriddenDayTypeData, 1);

			day = g_hash_table_lookup (data->day_id_hash, GINT_TO_POINTER (day_type_id));
			day_type_data->day = day;

			g_hash_table_insert (hash, GINT_TO_POINTER (day_type_id), day_type_data);
		}

		interval = mrp_interval_new (start, end);
		day_type_data->intervals = g_list_append (day_type_data->intervals, interval);		
		
		d(g_print ("Overridden intervals for day %d, on cal %d, %d - %d\n",
			 day_type_id, calendar_id,
			 (int)start, (int)end));

		interval = mrp_interval_new (start, end);
	}
	g_object_unref (res);

	res = sql_execute_query (data->con,"CLOSE daycursor");
	g_object_unref (res);
	
	
	/* Set the intervals for the day types. */
	calendar = g_hash_table_lookup (data->calendar_id_hash, GINT_TO_POINTER (calendar_id));
	g_hash_table_foreach (hash, (GHFunc) foreach_insert_overridden_day_type, calendar);
	g_hash_table_destroy (hash);

	return TRUE;

 out:
	if (res) {
		g_object_unref (res);
	}

	return FALSE;
}	

/* Reads overridden days, i.e. specific dates that doesn't use the default day.
 */
static gboolean
sql_read_overriden_days (SQLData *data, gint calendar_id)
{
	gint          n, i, j;
	GdaDataModel *res;
	gchar        *query;
	
	gint          day_type_id;
	mrptime       date;

	/* Get overridden days for the given calendar. */
	query = g_strdup_printf ("DECLARE daycursor CURSOR FOR SELECT "
				 "extract (epoch from date) as date_seconds, "
				 "* FROM day WHERE cal_id=%d",
				 calendar_id);
	
	res = sql_execute_query (data->con, query);
	g_free (query);
	

	if (res == NULL) {
		g_warning ("DECLARE CURSOR command failed (day) %s.",
				sql_get_last_error (data->con));
		goto out;
	}
	g_object_unref (res);
	
	res = sql_execute_query (data->con, "FETCH ALL in daycursor");
	if (res == NULL) {
		g_warning ("FETCH ALL failed for day %s.",
				sql_get_last_error (data->con));
		goto out;
	}
	
	
	n = gda_data_model_get_n_columns (res);
	for (i = 0; i < gda_data_model_get_n_rows (res); i++) {
		day_type_id = -1;
		date = -1;
		
		for (j = 0; j < n; j++) {
			if (is_field (res, j, "date_seconds")) {
				date = get_int (res, i, j);				
			}
			else if (is_field (res, j, "dtype_id")) {
				day_type_id = get_int (res, i, j);
			}
		}
		
		d(g_print ("Overridden for cal %d, on %s\n", calendar_id, mrp_time_format ("%a %e %b", date)));

		/*data->days = g_list_prepend (data->days, day);*/
	}
	g_object_unref (res);

	res = sql_execute_query (data->con,"CLOSE daycursor");
	g_object_unref (res);
	

	return TRUE;

 out:
	if (res) {
		g_object_unref (res);
	}

	return FALSE;
}	

static gboolean
sql_read_day_types (SQLData *data)
{
	gint          n, i, j;
	GdaDataModel *res;
	gchar        *query;
	
	gint          day_type_id;
	gchar        *name;
	gchar        *description;
	MrpDay       *day;
	gboolean      is_work, is_nonwork;
	
	/* Get day types. */
	query = g_strdup_printf ("DECLARE mycursor CURSOR FOR SELECT * FROM daytype WHERE proj_id=%d",
				 data->project_id);
	
	res = sql_execute_query (data->con, query);
	g_free (query);

	if (res == NULL) {
		g_warning ("DECLARE CURSOR command failed (daytype) %s.",
				sql_get_last_error (data->con));
		goto out;
	}
	g_object_unref (res);

	res = sql_execute_query (data->con, "FETCH ALL in mycursor");
	if (res == NULL) {
		g_warning ("FETCH ALL failed for daytype %s.",
				sql_get_last_error (data->con));
		goto out;
	}
	
	n = gda_data_model_get_n_columns (res);
	for (i = 0; i < gda_data_model_get_n_rows (res); i++) {
		name = NULL;
		description = NULL;
		day_type_id = -1;
		is_work = FALSE;
		is_nonwork = FALSE;

		for (j = 0; j < n; j++) {
			if (is_field (res, j, "name")) {
				name = get_string (res, i, j);
			}
			else if (is_field (res, j, "descr")) {
				description = get_string (res, i, j);
			}
			else if (is_field (res, j, "dtype_id")) {
				day_type_id = get_int (res, i, j);
			}
			else if (is_field (res, j, "is_work")) {
				is_work = get_boolean (res, i, j);
			}
			else if (is_field (res, j, "is_nonwork")) {
				is_nonwork = get_boolean (res, i, j);
			}
		}

		d(g_print ("Day type: %s, id: %d, work: %d, nonwork: %d\n", name, day_type_id, is_work, is_nonwork));

		if (is_work) {
			day = mrp_day_get_work ();
		}
		else if (is_nonwork) {
			day = mrp_day_get_nonwork ();
		} else {
			day = mrp_day_add (data->project, name, description);
		}
		
		g_free (name);
		g_free (description);
		
		g_hash_table_insert (data->day_id_hash, GINT_TO_POINTER (day_type_id), day);
	}
	g_object_unref (res);

	res = sql_execute_query (data->con, "CLOSE mycursor");
	g_object_unref (res);

	return TRUE;

 out:
	if (res) {
		g_object_unref (res);
	}

	return FALSE;
}	

static gboolean
sql_calendar_create (GNode   *node,
			    SQLData *data)
{
	CalendarData *cal_data = node->data;
	MrpCalendar  *calendar, *parent;

	if (!node->parent) {
		/* Skip the root .*/
		return FALSE;
	}
	
	if (!node->parent->parent) {
		/* Calendar directly under the root don't inherit from another
		 * calendar.
		 */
		d(g_print ("Create new calendar: %s\n", cal_data->name));
	
		calendar = mrp_calendar_new (cal_data->name, data->project);
	} else {
		d(g_print ("Derive new calendar: %s\n", cal_data->name));
		
		parent = g_hash_table_lookup (data->calendar_id_hash, GINT_TO_POINTER (cal_data->parent_id));
		calendar = mrp_calendar_derive (cal_data->name, parent);
	}

	g_hash_table_insert (data->calendar_id_hash, GINT_TO_POINTER (cal_data->id), calendar);

	mrp_calendar_set_default_days (calendar,
				       MRP_CALENDAR_DAY_MON, cal_data->day_mon,
				       MRP_CALENDAR_DAY_TUE, cal_data->day_tue,
				       MRP_CALENDAR_DAY_WED, cal_data->day_wed,
				       MRP_CALENDAR_DAY_THU, cal_data->day_thu,
				       MRP_CALENDAR_DAY_FRI, cal_data->day_fri,
				       MRP_CALENDAR_DAY_SAT, cal_data->day_sat,
				       MRP_CALENDAR_DAY_SUN, cal_data->day_sun,
				       -1);
	
	sql_read_overriden_days (data, cal_data->id);
	sql_read_overriden_day_types (data, cal_data->id);
	
	return FALSE;
}

static void
sql_calendar_insert_node (GHashTable *hash,
				 GNode      *root,
				 GNode      *node)
{
	CalendarData *data;
	GNode        *parent;
	
	/* If the node is already inserted, do nothing. */
	if (node->parent) {
		return;
	}

	/* If the parent id of the node is -1, insert below the root. */
	data = node->data;
	if (data->parent_id == -1) {
		g_node_prepend (root, node);
		return;
	}
	
	/* Otherwise insert below the parent corresponding to the parent id. */
	parent = g_hash_table_lookup (hash, GINT_TO_POINTER (data->parent_id));

	if (!parent) {
		/* If we for some reason don't find the parent, use the root. */
		parent = root;
	}
	
	g_node_prepend (parent, node);
}

static void
dump_calendar_tree (GNode *node)
{
	CalendarData *data;
	GNode        *child;
	gchar        *str;

	return;
	
	str = g_malloc0 (g_node_depth (node));
	memset (str, ' ', g_node_depth (node)-1);
	
	data = node->data;

	d(g_print ("%s%s\n", str, data ? data->name : "[Root]"));
	g_free (str);

	for (child = node->children; child; child = child->next) {
		dump_calendar_tree (child);
	}
}

static gboolean
sql_read_calendars (SQLData *data)
{
	gint          n, i, j;
	GdaDataModel *res;
	gchar        *query;
	
	CalendarData *calendar_data;
	GNode        *tree;
	GNode        *node;
	GHashTable   *hash;
	GList        *calendars = NULL, *l;
	gint          day_id;

	/* Get calendars. */
	query = g_strdup_printf ("DECLARE mycursor CURSOR FOR SELECT * FROM calendar WHERE proj_id=%d",
				 data->project_id);
	res = sql_execute_query (data->con, query);
	g_free (query);

	if (res == NULL) {
		g_warning ("DECLARE CURSOR command failed (calendar) %s.",
				sql_get_last_error (data->con));
		goto out;
	}
	g_object_unref (res);

	res = sql_execute_query (data->con, "FETCH ALL in mycursor");
	if (res == NULL) {
		g_warning ("FETCH ALL failed for calendar %s.",
				sql_get_last_error (data->con));
		goto out;
	}

	tree = g_node_new (NULL);
	hash = g_hash_table_new (NULL, NULL);
	
	n = gda_data_model_get_n_columns (res);
	for (i = 0; i < gda_data_model_get_n_rows (res); i++) {
		calendar_data = g_new0 (CalendarData, 1);
		node = g_node_new (calendar_data);
		
		for (j = 0; j < n; j++) {
			if (is_field (res, j, "name")) {
				calendar_data->name = get_string (res, i, j);
			}
			else if (is_field (res, j, "cal_id")) {
				calendar_data->id = get_int (res, i, j);
			}
			else if (is_field (res, j, "parent_cid")) {
				calendar_data->parent_id = get_id (res, i, j);
			}
			else if (is_field (res, j, "day_mon")) {
				day_id = get_int (res, i, j);
				calendar_data->day_mon = g_hash_table_lookup (data->day_id_hash, GINT_TO_POINTER (day_id));
			}
			else if (is_field (res, j, "day_tue")) {
				day_id = get_int (res, i, j);
				calendar_data->day_tue = g_hash_table_lookup (data->day_id_hash, GINT_TO_POINTER (day_id));
			}
			else if (is_field (res, j, "day_wed")) {
				day_id = get_int (res, i, j);
				calendar_data->day_wed = g_hash_table_lookup (data->day_id_hash, GINT_TO_POINTER (day_id));
			}
			else if (is_field (res, j, "day_thu")) {
				day_id = get_int (res, i, j);
				calendar_data->day_thu = g_hash_table_lookup (data->day_id_hash, GINT_TO_POINTER (day_id));
			}
			else if (is_field (res, j, "day_fri")) {
				day_id = get_int (res, i, j);
				calendar_data->day_fri = g_hash_table_lookup (data->day_id_hash, GINT_TO_POINTER (day_id));
			}
			else if (is_field (res, j, "day_sat")) {
				day_id = get_int (res, i, j);
				calendar_data->day_sat = g_hash_table_lookup (data->day_id_hash, GINT_TO_POINTER (day_id));
			}
			else if (is_field (res, j, "day_sun")) {
				day_id = get_int (res, i, j);
				calendar_data->day_sun = g_hash_table_lookup (data->day_id_hash, GINT_TO_POINTER (day_id));
			}
		}
		
		d(g_print ("Calendar: %s, id: %d, parent: %d\n",
			 calendar_data->name,
			 calendar_data->id,
			 calendar_data->parent_id));
		
		calendars = g_list_prepend (calendars, node);
		g_hash_table_insert (hash, GINT_TO_POINTER (calendar_data->id), node);
	}
	g_object_unref (res);

	res = sql_execute_query (data->con, "CLOSE mycursor");
	g_object_unref (res);

	/* Build a GNode tree with all the calendars. */
	for (l = calendars; l; l = l->next) {
		sql_calendar_insert_node (hash, tree, l->data);
	}

	/* Debug output. */
	dump_calendar_tree (tree);

	/* Create calendars. */
	g_node_traverse (tree,
			 G_PRE_ORDER,
			 G_TRAVERSE_ALL,
			 -1,
			 (GNodeTraverseFunc) sql_calendar_create,
			 data);

	for (l = calendars; l; l = l->next) {
		GNode *node = l->data;

		calendar_data = node->data;

		g_free (calendar_data->name);
		g_free (calendar_data);
	}
	g_list_free (calendars);
	g_hash_table_destroy (hash);
	g_node_destroy (tree);
	
	return TRUE;

 out:
	if (res) {
		g_object_unref (res);
	}

	return FALSE;
}	

static gboolean
sql_read_groups (SQLData *data)
{
	gint          n, i, j;
	GdaDataModel *res;
	gchar        *query;
	
	MrpGroup     *group;
	gint          group_id;
	gchar        *name;
	gchar        *admin_name;
	gchar        *admin_phone;
	gchar        *admin_email;
		
	/* Get resource groups. */
	query = g_strdup_printf ("DECLARE mycursor CURSOR FOR SELECT * FROM resource_group WHERE proj_id=%d",
				 data->project_id);
	res = sql_execute_query (data->con, query);
	g_free (query);

	if (res == NULL) {
		g_warning ("DECLARE CURSOR command failed (resource_group) %s.",
				sql_get_last_error (data->con));
		goto out;
	}
	g_object_unref (res);

	res = sql_execute_query (data->con, "FETCH ALL in mycursor");
	if (res == NULL) {
		g_warning ("FETCH ALL failed for resource_group %s.",
				sql_get_last_error (data->con));
		goto out;
	}
	
	n = gda_data_model_get_n_columns (res);
	for (i = 0; i < gda_data_model_get_n_rows (res); i++) {
		group_id = -1;
		name = NULL;
		admin_name = NULL;
		admin_phone = NULL;
		admin_email = NULL;
		
		for (j = 0; j < n; j++) {
			if (is_field (res, j, "name")) {
				name = get_string (res, i, j);
			}
			else if (is_field (res, j, "group_id")) {
				group_id = get_int (res, i, j);
			}
			else if (is_field (res, j, "admin_name")) {
				admin_name = get_string (res, i, j);
			}
			else if (is_field (res, j, "admin_phone")) {
				admin_phone = get_string (res, i, j);
			}
			else if (is_field (res, j, "admin_email")) {
				admin_email = get_string (res, i, j);
			}
		}
		
		group = g_object_new (MRP_TYPE_GROUP,
				      "name", name,
				      "manager_name", admin_name,
				      "manager_phone", admin_phone,
				      "manager_email", admin_email,
				      NULL);

		g_free (name);
		g_free (admin_name);
		g_free (admin_phone);
		g_free (admin_email);

		/* Add group to project. */
		mrp_project_add_group (data->project, group);

		g_hash_table_insert (data->group_id_hash, GINT_TO_POINTER (group_id), group);
	}
	g_object_unref (res);

	res = sql_execute_query (data->con, "CLOSE mycursor");
	g_object_unref (res);

	return TRUE;

 out:
	if (res) {
		g_object_unref (res);
	}

	return FALSE;
}	

static gboolean
sql_read_resources (SQLData *data)
{
	gint          n, i, j;
	GdaDataModel *res;
	gchar        *query;
	
	gint          resource_id;
	gint          group_id;
	gint          calendar_id;
	gchar        *name;
	gchar        *email;
	gchar	     *short_name;
	gchar        *note;
	MrpGroup     *group;
	MrpCalendar  *calendar;
	MrpResource  *resource;

	/* Get resources. */
	query = g_strdup_printf ("DECLARE mycursor CURSOR FOR SELECT * FROM resource WHERE proj_id=%d",
				 data->project_id);
	res = sql_execute_query (data->con, query);
	g_free (query);

	if (res == NULL) {
		g_warning ("DECLARE CURSOR command failed (resource) %s.",
				sql_get_last_error (data->con));
		goto out;
	}
	g_object_unref (res);

	res = sql_execute_query (data->con, "FETCH ALL in mycursor");
	if (res == NULL) {
		g_warning ("FETCH ALL failed for resource %s.",
				sql_get_last_error (data->con));
		goto out;
	}
	
	n = gda_data_model_get_n_columns (res);
	
	for (i = 0; i < gda_data_model_get_n_rows (res); i++) {
		resource_id = -1;
		group_id = -1;
		calendar_id = -1;
		name = NULL;
		short_name = NULL;  /* Added in schema 0.7 */
		email = NULL;
		note = NULL;
		
		for (j = 0; j < n; j++) {
			if (is_field (res, j, "name")) {
				name = get_string (res, i, j);
			}
			else if (is_field (res, j, "short_name")) {
				short_name = get_string (res, i, j);
				
				/* FIXME: The next section detects the case if
				 * short_name is NULL. If a string field is null
				 * then get_string() seems to actually return
				 * the word "NULL". The fix is to correct the
				 * contents of the database at upgrade times so
				 * the following is a workaround until we have a
				 * database upgrade process that fixes NULLs.
				 */
				if (strcmp (short_name, "NULL") == 0) {  
					short_name = g_strdup ("");
				}
			}
			else if (is_field (res, j, "group_id")) {
				group_id = get_id (res, i, j);
			}
			else if (is_field (res, j, "res_id")) {
				resource_id = get_id (res, i, j);
			}
			else if (is_field (res, j, "email")) {
				email = get_string (res, i, j);
			}
			else if (is_field (res, j, "note")) {
				note = get_string (res, i, j);
			}
			else if (strcmp (gda_data_model_get_column_title (res, j), "cal_id") == 0) {
				calendar_id = get_id (res, i, j);
			}
		}

		group = g_hash_table_lookup (data->group_id_hash, GINT_TO_POINTER (group_id));
		calendar = g_hash_table_lookup (data->calendar_id_hash, GINT_TO_POINTER (calendar_id));

		/* Warning: Always check for NULL data on strings here for any new schema variables 
		*  because if you don't and someone opens an OLD database then it will segfault 
		*  in resource-set-property. Use short_name as an example.
		*/
		
		resource = g_object_new (MRP_TYPE_RESOURCE,
					 "name", name,
					 "short_name", (short_name ? short_name : ""),
					 "email", email,
					 "note", note,
					 "group", group,
					 "calendar", calendar,
					 NULL);

		g_free (name);
		g_free (short_name);
		g_free (email);
		g_free (note);
		
		/* Add resource to project. */
		mrp_project_add_resource (data->project, resource);
		g_hash_table_insert (data->resource_id_hash, GINT_TO_POINTER (resource_id), resource);
		g_hash_table_insert (data->resource_hash, resource, GINT_TO_POINTER (resource_id));

		/* Get property values. */
		if (!sql_read_property_values (data, MRP_OBJECT (resource))) {
			g_warning ("Couldn't read resource properties.");
		}
	}
	
	g_object_unref (res);
	
	res = sql_execute_query (data->con, "CLOSE mycursor");
	g_object_unref (res);

	return TRUE;
	
 out:
	if (res) {
		g_object_unref (res);
	}
	
	return FALSE;
}

static gboolean
sql_read_assignments (SQLData *data,
			    gint     task_id)
{
	gint          n, i, j;
	GdaDataModel *res;
	gchar        *query;
	
	gint          units;
	gint          resource_id;
	MrpTask      *task;
	MrpResource  *resource;

	/* Get assignments. */
	query = g_strdup_printf ("DECLARE alloccursor CURSOR FOR SELECT "
				 "* FROM allocation WHERE task_id=%d",
				 task_id);
	res = sql_execute_query (data->con, query);
	g_free (query);

	if (res == NULL) {
		g_warning ("DECLARE CURSOR command failed (allocation) %s.",
				sql_get_last_error (data->con));
		goto out;
	}
	g_object_unref (res);

	res = sql_execute_query (data->con, "FETCH ALL in alloccursor");
	if (res == NULL) {
		g_warning ("FETCH ALL failed for allocation %s.",
				sql_get_last_error (data->con));
		goto out;
	}

	n = gda_data_model_get_n_columns (res);
	for (i = 0; i < gda_data_model_get_n_rows (res); i++) {
		resource_id = -1;
		units = -1;
		
		for (j = 0; j < n; j++) {
			if (is_field (res, j, "units")) {
				units = floor (0.5 + 100.0 * get_float (res, i, j));
			}
			else if (is_field (res, j, "res_id")) {
				resource_id = get_id (res, i, j);
			}
		}

		task = g_hash_table_lookup (data->task_id_hash, GINT_TO_POINTER (task_id));
		resource = g_hash_table_lookup (data->resource_id_hash, GINT_TO_POINTER (resource_id));

		mrp_resource_assign (resource, task, units);
	}
	g_object_unref (res);

	res = sql_execute_query (data->con, "CLOSE alloccursor");
	g_object_unref (res);

	return TRUE;
	
 out:
	if (res) {
		g_object_unref (res);
	}
	
	return FALSE;
}

static gboolean
sql_read_relations (SQLData *data, gint task_id)
{
	gint          n, i, j;
	GdaDataModel *res;
	gchar        *query;
	
	gint          predecessor_id;
	gint          lag;
	MrpTask      *task;
	MrpTask      *predecessor;

	/* Get relations. */
	query = g_strdup_printf ("DECLARE predcursor CURSOR FOR SELECT "
				 "* FROM predecessor WHERE task_id=%d",
				 task_id);
	res = sql_execute_query (data->con, query);
	g_free (query);

	if (res == NULL) {
		g_warning ("DECLARE CURSOR command failed (predecessor) %s.",
				sql_get_last_error (data->con));
		goto out;
	}
	g_object_unref (res);

	res = sql_execute_query (data->con, "FETCH ALL in predcursor");
	if (res == NULL) {
		g_warning ("FETCH ALL failed for predecessor %s.",
				sql_get_last_error (data->con));
		goto out;
	}

	n = gda_data_model_get_n_columns (res);
	for (i = 0; i < gda_data_model_get_n_rows (res); i++) {
		predecessor_id = -1;
		lag = 0;
		
		for (j = 0; j < n; j++) {
			if (is_field (res, j, "pred_task_id")) {
				predecessor_id = get_id (res, i, j);
			}
			else if (is_field (res, j, "lag")) {
				lag = get_int (res, i, j);
			}
		}

		task = g_hash_table_lookup (data->task_id_hash, GINT_TO_POINTER (task_id));
		predecessor = g_hash_table_lookup (data->task_id_hash, GINT_TO_POINTER (predecessor_id));

		mrp_task_add_predecessor (task,
					  predecessor,
					  MRP_RELATION_FS,
					  lag,
					  NULL);
	}
	g_object_unref (res);

	res = sql_execute_query (data->con, "CLOSE predcursor");
	g_object_unref (res);

	return TRUE;
	
 out:
	if (res) {
		g_object_unref (res);
	}
	
	return FALSE;
}

/**
 * Insert a task from the GNode tree into the project.
 */
static gboolean
sql_task_insert (GNode   *node,
		 SQLData *data)
{
	TaskData *task_data = node->data;
	MrpTask  *parent;

	if (!node->parent) {
		/* Skip the root .*/
		return FALSE;
	}
	
	if (!node->parent->parent) {
		/* Tasks directly under the root is inserted below the root. */
		parent = data->root_task;
	} else {
		parent = g_hash_table_lookup (data->task_id_hash, GINT_TO_POINTER (task_data->parent_id));
	}

	imrp_task_insert_child (parent, -1, task_data->task);
	
	return FALSE;
}

/* Used to build a tree of tasks while reading them. This is needed because we
 * can't insert a task below its parent until the parent is inserted and we can
 * get tasks in any order (e.g. children before parents).
 */
static void
sql_task_insert_node (GHashTable *hash,
		      GNode      *root,
		      GNode      *node)
{
	TaskData *data;
	GNode    *parent;
	
	/* If the node is already inserted, do nothing. */
	if (node->parent) {
		return;
	}

	/* If the parent id of the node is -1, insert below the root. */
	data = node->data;
	if (data->parent_id == -1) {
		g_node_prepend (root, node);
		return;
	}
	
	/* Otherwise insert below the parent corresponding to the parent id. */
	parent = g_hash_table_lookup (hash, GINT_TO_POINTER (data->parent_id));
	if (!parent) {
		/* If we for some reason don't find the parent, use the root. */
		parent = root;
	}
	
	g_node_prepend (parent, node);
}

static void
dump_task_tree (GNode *node)
{
	TaskData *data;
	GNode    *child;
	gchar    *str;
	gchar    *name = NULL;

	return;
	
	str = g_malloc0 (g_node_depth (node));
	memset (str, ' ', g_node_depth (node)-1);
	
	data = node->data;

	if (data) {
		g_object_get (data->task, "name", &name, NULL);
	}
	d(g_print ("%s%s\n", str, data ? name : "[Root]"));
	g_free (name);
	g_free (str);

	for (child = node->children; child; child = child->next) {
		dump_task_tree (child);
	}
}

static MrpConstraintType
constraint_string_to_type (const gchar *type)
{
	if (!strcmp (type, "ASAP")) {
		return MRP_CONSTRAINT_ASAP;
	}
	else if (!strcmp (type, "MSO")) {
		return MRP_CONSTRAINT_MSO;
	}
	else if (!strcmp (type, "FNLT")) {
		return MRP_CONSTRAINT_FNLT;
	}
	else if (!strcmp (type, "SNET")) {
		return MRP_CONSTRAINT_SNET;
	}

	return MRP_CONSTRAINT_ASAP;
}

static gboolean
sql_read_tasks (SQLData *data)
{
	gint               n, i, j;
	GdaDataModel      *res;
	gchar             *query;
	
	gint               task_id;
	gint               parent_id;
	gchar             *name;
	gchar             *note;
	gint               work;
	gint               duration;
	gint               percent_complete;
	gint               priority;
	gboolean           is_fixed_work;
	gboolean           is_milestone;
	MrpTaskType        type;
	MrpTaskSched       sched;
	MrpConstraintType  constraint_type;
	mrptime            constraint_time;
	MrpConstraint      constraint;
	MrpTask           *task;
	GNode             *tree, *node;
	GList             *tasks = NULL, *l;
	GHashTable        *hash;
	TaskData          *task_data;

	/* Get tasks. */
	query = g_strdup_printf ("DECLARE mycursor CURSOR FOR SELECT "
				 "extract (epoch from constraint_time) as constraint_time_seconds, "
				 "* FROM task WHERE proj_id=%d",
				 data->project_id);
	res = sql_execute_query (data->con, query);
	g_free (query);

	if (res == NULL) {
		g_warning ("DECLARE CURSOR command failed (task) %s.",
				sql_get_last_error (data->con));
		goto out;
	}
	g_object_unref (res);

	res = sql_execute_query (data->con, "FETCH ALL in mycursor");
	if (res == NULL) {
		g_warning ("FETCH ALL failed for task %s.",
				sql_get_last_error (data->con));
		goto out;
	}

	hash = g_hash_table_new (NULL, NULL);

	n = gda_data_model_get_n_columns (res);
	for (i = 0; i < gda_data_model_get_n_rows (res); i++) {
		task_id = -1;
		parent_id = -1;
		name = NULL;
		note = NULL;
		work = 0;
		duration = 0;
		percent_complete = 0;
		priority = 0;
		is_fixed_work = FALSE;
		is_milestone = FALSE;
		constraint_time = 0;
		constraint_type = MRP_CONSTRAINT_ASAP;
		
		for (j = 0; j < n; j++) {
			if (is_field (res, j, "name")) {
				name = get_string (res, i, j);
			}
			else if (is_field (res, j, "task_id")) {
				task_id = get_int (res, i, j);
			}
			else if (is_field (res, j, "parent_id")) {
				parent_id = get_id (res, i, j);
			}
			else if (is_field (res, j, "work")) {
				work = get_int (res, i, j);
			}
			else if (is_field (res, j, "duration")) {
				duration = get_int (res, i, j);
			}
			else if (is_field (res, j, "percent_complete")) {
				percent_complete = get_int (res, i, j);
			}
			else if (is_field (res, j, "priority")) {
				priority = get_int (res, i, j);
			}
			else if (is_field (res, j, "is_milestone")) {
				is_milestone = get_boolean (res, i, j);
			}
			else if (is_field (res, j, "is_fixed_work")) {
				is_fixed_work = get_boolean (res, i, j);
			}
			else if (is_field (res, j, "note")) {
				note = get_string (res, i, j);
			}
			else if (is_field (res, j, "constraint_type")) {
				constraint_type = constraint_string_to_type 
					(get_string (res, i, j));
			}
			else if (is_field (res, j, "constraint_time_seconds")) {
				constraint_time = get_int (res, i, j);
			}
		}

		if (is_milestone) {
			type = MRP_TASK_TYPE_MILESTONE;
		} else {
			type = MRP_TASK_TYPE_NORMAL;
		}			

		if (is_fixed_work) {
			sched = MRP_TASK_SCHED_FIXED_WORK;
			duration = work;
		} else {
			sched = MRP_TASK_SCHED_FIXED_DURATION;
		}			

		constraint.type = constraint_type;
		constraint.time = constraint_time;
				
		task = g_object_new (MRP_TYPE_TASK,
				     "name", name,
				     "note", note,
				     "type", type,
				     "percent_complete", percent_complete,
				     "priority", priority,
				     "sched", sched,
				     "work", work,
				     "duration", duration,
				     "constraint", &constraint,
				     "project", data->project,
				     NULL);
		
		g_free (name);
		g_free (note);

		task_data = g_new0 (TaskData, 1);
		task_data->id = task_id;
		task_data->parent_id = parent_id;
		task_data->task = task;

		node = g_node_new (task_data);
		g_hash_table_insert (hash, GINT_TO_POINTER (task_id), node);
		tasks = g_list_prepend (tasks, node);

		data->tasks = g_list_prepend (data->tasks, task);
		g_hash_table_insert (data->task_id_hash, GINT_TO_POINTER (task_id), task);
		g_hash_table_insert (data->task_hash, task, GINT_TO_POINTER (task_id));
	}
	g_object_unref (res);

	res = sql_execute_query (data->con, "CLOSE mycursor");
	g_object_unref (res);

	/* Build a GNode tree with all the tasks. */
	tree = g_node_new (NULL);
	for (l = tasks; l; l = l->next) {
		sql_task_insert_node (hash, tree, l->data);
	}
	
	/* Debug output. */
	dump_task_tree (tree);

	/* Insert tasks. */
	g_node_traverse (tree,
			 G_PRE_ORDER,
			 G_TRAVERSE_ALL,
			 -1,
			 (GNodeTraverseFunc) sql_task_insert,
			 data);

	/* Get predecessor relations. */
	for (l = tasks; l; l = l->next) {
		GNode *node = l->data;
		
		task_data = node->data;
		if (!sql_read_relations (data, task_data->id)) {
			g_warning ("Couldn't read predecessor relations.");
		}
	}

	/* Get resource assignments. */
	for (l = tasks; l; l = l->next) {
		GNode *node = l->data;
		
		task_data = node->data;
		if (!sql_read_assignments (data, task_data->id)) {
			g_warning ("Couldn't read resource assignments.");
		}
	}

	/* Get property values. */
	for (l = tasks; l; l = l->next) {
		GNode *node = l->data;
		
		task_data = node->data;
		if (!sql_read_property_values (data, MRP_OBJECT (task_data->task))) {
			g_warning ("Couldn't read task properties.");
		}
	}

	/* Clean up. */
	for (l = tasks; l; l = l->next) {
		GNode *node = l->data;

		task_data = node->data;
		g_free (task_data);
	}

	g_list_free (tasks);
	g_hash_table_destroy (hash);
	g_node_destroy (tree);

	return TRUE;
	
 out:
	if (res) {
		g_object_unref (res);
	}
	
	return FALSE;
}

gboolean
mrp_sql_load_project (MrpStorageSQL *storage,
		      const gchar   *server,
		      const gchar   *port,
		      const gchar   *database,
		      const gchar   *login,
		      const gchar   *password,
		      gint           project_id,
		      GError       **error)
{
	SQLData        *data;
	GdaDataModel   *res = NULL;
	GdaClient      *client;
	const gchar    *dsn_name = "planner-auto";
	gchar          *db_txt;
	MrpCalendar    *calendar;
	MrpGroup       *group;
	MrpTaskManager *task_manager;
	const gchar    *provider = "PostgreSQL";

	data = g_new0 (SQLData, 1);

	data->project_id = -1;
	data->day_id_hash = g_hash_table_new (NULL, NULL);
	data->calendar_id_hash = g_hash_table_new (NULL, NULL);
	data->group_id_hash = g_hash_table_new (NULL, NULL);
	data->task_id_hash = g_hash_table_new (NULL, NULL);
	data->resource_id_hash = g_hash_table_new (NULL, NULL);
	data->property_type_id_hash = g_hash_table_new (NULL, NULL);

	data->task_hash = g_hash_table_new (NULL, NULL);
	data->resource_hash = g_hash_table_new (NULL, NULL);

	data->project = storage->project;

	data->root_task = mrp_task_new ();

	db_txt = g_strdup_printf ("DATABASE=%s",database);
	gda_config_save_data_source (dsn_name, 
                                     provider, 
                                     db_txt,
                                     "planner project", login, password);
	g_free (db_txt);

	client = gda_client_new ();

	data->con = gda_client_open_connection (client, dsn_name, NULL, NULL, 0);

	if (!GDA_IS_CONNECTION (data->con)) {
		g_warning (_("Connection to database '%s' failed.\n"), database);
		/* This g_set_error isn't shown - don't know why exactly */
		g_set_error (error,
			MRP_ERROR,
			MRP_ERROR_SAVE_WRITE_FAILED,
			_("Connection to database '%s' failed.\n%s"),
			database,
			sql_get_last_error (data->con));
		goto out;
	}
	
	res = sql_execute_query (data->con, "BEGIN");
	if (res == NULL) {
		g_warning (_("BEGIN command failed %s."),
				sql_get_last_error (data->con));
		goto out;
	}
	g_object_unref (res);
	res = NULL;

	/* Get project. */
	if (!sql_read_project (data, project_id)) {
		g_warning ("Couldn't read project.");
		goto out;
	}

	/* Get phases. */
	if (!sql_read_phases (data)) {
		g_warning ("Couldn't read phases.");
	}

	/* Get custom property specs. */
	if (!sql_read_property_specs (data)) {
		g_warning ("Couldn't read property specs.");
	}

	/* Get custom property specs. */
	if (!sql_read_property_values (data, MRP_OBJECT (data->project))) {
		g_warning ("Couldn't read project properties.");
	}
	
	/* Get day types. */
	if (!sql_read_day_types (data)) {
		g_warning ("Couldn't read day types.");
	}

	/* Get calendars. */
	if (!sql_read_calendars (data)) {
		g_warning ("Couldn't read calendars.");
	}

	calendar = g_hash_table_lookup (data->calendar_id_hash, GINT_TO_POINTER (data->calendar_id));
	g_object_set (data->project, "calendar", calendar, NULL);

	/* Get resource groups. */
	if (!sql_read_groups (data)) {
		g_warning ("Couldn't read resource groups.");
	}

	group = g_hash_table_lookup (data->group_id_hash, GINT_TO_POINTER (data->default_group_id));
	g_object_set (data->project, "default_group", group, NULL);

	/* Get resources. */
	if (!sql_read_resources (data)) {
		g_warning ("Couldn't read resources.");
	}

	/* Get tasks. */
	if (!sql_read_tasks (data)) {
		g_warning ("Couldn't read tasks.");
	} else {
		task_manager = imrp_project_get_task_manager (storage->project);
		mrp_task_manager_set_root (task_manager, data->root_task);
	}

	res = sql_execute_query (data->con, "COMMIT");
	g_object_unref (res);

	g_object_unref (data->con);

	d(g_print ("Read project, set rev to %d\n", data->revision));
	
	g_object_set_data (G_OBJECT (storage->project),
			   REVISION,
			   GINT_TO_POINTER (data->revision));

	return TRUE;
	
 out:
	if (res) {
		g_object_unref (res);
	}
	
	g_object_unref (data->con);
	return FALSE;

	/* FIXME: free data */
}

#define WRITE_ERROR(e,c) \
G_STMT_START \
g_set_error(e,MRP_ERROR,MRP_ERROR_SAVE_WRITE_FAILED, sql_get_last_error (c)) \
G_STMT_END


/*************************
 * Save
 */
static gboolean
sql_write_project (MrpStorageSQL  *storage,
		   SQLData        *data,
		   gboolean        force,
		   GError        **error)
{
	GdaDataModel *res;
	gchar        *query;
	
	gint          project_id;
	gint          revision;
	gchar        *last_user;
	gchar        *name;
	mrptime       project_start;
	gchar        *manager, *company;
	gchar        *str;

	project_id = data->project_id;

	/* If the project id is -1, it means that we don't have a project saved
	 * in the database yet, so we don't need to remove a project before
	 * saving it.
	 */
	if (project_id != -1) {
		/* First check if a project with the given id already exists. */
		query = g_strdup_printf ("DECLARE mycursor CURSOR FOR SELECT "
					 "name, revision, last_user FROM project WHERE proj_id=%d",
					 project_id);
		res = sql_execute_query (data->con, query);
		
		g_free (query);
		if (res == NULL) {
			WRITE_ERROR (error, data->con);
			return FALSE;
		}
		g_object_unref (res);
		
		res = sql_execute_query (data->con, "FETCH ALL in mycursor");
		if (res == NULL) {
			WRITE_ERROR (error, data->con);
			return FALSE;
		}

		if (gda_data_model_get_n_rows (res) > 0) {
			name = get_string (res, 0, 0);
			revision = get_int (res, 0, 1);
			last_user = get_string (res, 0, 2);
			
			g_object_unref (res);
			
			res = sql_execute_query (data->con, "CLOSE mycursor");
			g_object_unref (res);
			
			/* Remove the old project. */
			d(g_print ("Got old project with id %d (rev %d), remove.\n", project_id, revision));
		
			query = g_strdup_printf ("DELETE FROM project WHERE proj_id=%d", project_id);
			res = sql_execute_query (data->con, query);
			g_free (query);
			
			if (res == NULL) {
				WRITE_ERROR (error, data->con);
				return FALSE;
			}
			g_object_unref (res);
		
			d(g_print ("*** revision: %d, old revision: %d\n", revision, data->revision));
			
			if (!force && data->revision > 0 && revision != data->revision) {
				g_set_error (error,
					     MRP_ERROR, MRP_ERROR_SAVE_FILE_CHANGED,
					     _("The project '%s' has been changed by the user '%s' "
					       "since you opened it. Do you want to save anyway?"),
					     name, last_user);

				g_free (last_user);
				g_free (name);
				
				return FALSE;
			}

			g_free (last_user);
			g_free (name);
			
			data->revision = revision + 1;
		} else {
			g_object_unref (res);
			
			data->revision = 1;
		}
	} else {
		/* There was no old project. */
		data->revision = 1;
	}

	g_object_get (data->project,
		      "name", &name,
		      "manager", &manager,
		      "organization", &company,
		      "project_start", &project_start,
		      NULL);

	/* Note: Could probably let the sql server to the conversion here. */
	str = mrp_time_format ("%Y-%m-%d", project_start);

	if (project_id != -1) {
		d(g_print ("Trying to insert project with id: %d\n", project_id));
		query = g_strdup_printf ("INSERT INTO project(proj_id, name, company, manager, proj_start, revision) "
					 "VALUES(%d, '%s', '%s', '%s', '%s', %d)",
					 project_id, name, company, manager, str, data->revision);
	} else {
		d(g_print ("Trying to insert new project.\n"));
		query = g_strdup_printf ("INSERT INTO project(name, company, manager, proj_start, revision) "
					 "VALUES('%s', '%s', '%s', '%s', %d)",
					 name, company, manager, str, data->revision);
	}

	res = sql_execute_query (data->con, query); 
	g_free (query);
	g_free (str);
	
	if (res == NULL) {
		WRITE_ERROR (error, data->con);
		return FALSE;
	}
	g_object_unref (res);


	if (project_id == -1) {
		/* Get the assigned id. */
		project_id = get_inserted_id (data, "project_proj_id_seq");
	}
	
	d(g_print ("Inserted project '%s', %d\n", name, project_id));

	data->project_id = project_id;
	
	return TRUE;
}

static gboolean
sql_write_phases (SQLData *data)
{
	GdaDataModel *res;
	gchar        *query;
	
	GList        *phases, *l;
	gchar        *name;

	g_object_get (data->project,
		      "phases", &phases,
		      NULL);
	for (l = phases; l; l = l->next) {
		name = l->data;

		query = g_strdup_printf ("INSERT INTO phase(proj_id, name) "
					 "VALUES(%d, '%s')",
					 data->project_id, name);

		res = sql_execute_query (data->con, query); 
		g_free (query);

		if (res == NULL) {
			g_warning ("INSERT command failed (phase) %s.",
					sql_get_last_error (data->con));
			goto out;
		}
	}

	mrp_string_list_free (phases);

	return TRUE;
	
 out:
	if (res) {
		g_object_unref (res);
	}
	
	return FALSE;
}

static gboolean
sql_write_phase (SQLData *data)
{
	GdaDataModel *res;
	gchar        *query;
	
	gchar        *phase;
	
	g_object_get (data->project,
		      "phase", &phase,
		      NULL);
	
	if (phase && phase[0]) {
		query = g_strdup_printf ("UPDATE project SET phase='%s' WHERE proj_id=%d", 
					 phase, data->project_id);
	} else {
		query = g_strdup_printf ("UPDATE project SET phase=NULL WHERE proj_id=%d", 
					 data->project_id);
	}
	
	res = sql_execute_query (data->con, query); 
	g_free (query);
	
	if (res == NULL) {
		g_warning ("UPDATE command failed (phase) %s.",
				sql_get_last_error (data->con));
		goto out;
	}
	
	return TRUE;
	
 out:
	if (res) {
		g_object_unref (res);
	}
	
	return FALSE;
}

static const gchar *
property_type_to_string (MrpPropertyType type)
{
	switch (type) {
	case MRP_PROPERTY_TYPE_INT:
		return "int";
	case MRP_PROPERTY_TYPE_STRING:
		return "text";
	case MRP_PROPERTY_TYPE_STRING_LIST:
		return "text-list";
	case MRP_PROPERTY_TYPE_FLOAT:
		return "float";
	case MRP_PROPERTY_TYPE_DURATION:
		return "duration";
	case MRP_PROPERTY_TYPE_DATE:
		return "date";	
	case MRP_PROPERTY_TYPE_COST:
		return "cost";	
	default:
		g_warning ("Not implemented support for type %d", type);
		break;
	}
	
	return "";
}

static gchar *
property_to_string (MrpObject   *object,
		    MrpProperty *property)
{
	const gchar *name;
	gchar       *str;
	gchar        buffer[G_ASCII_DTOSTR_BUF_SIZE];
	gint         i;
	gfloat       f;
	mrptime      date;
	
	name = mrp_property_get_name (property);
	
	switch (mrp_property_get_property_type (property)) {
	case MRP_PROPERTY_TYPE_STRING:
		mrp_object_get (object, name, &str, NULL);
		return str;
	case MRP_PROPERTY_TYPE_STRING_LIST:
		g_warning ("String list not supported.");
		return g_strdup ("");
	case MRP_PROPERTY_TYPE_INT:
		mrp_object_get (object, name, &i, NULL);
		return g_strdup_printf ("%d", i);
	case MRP_PROPERTY_TYPE_FLOAT:
		mrp_object_get (object, name, &f, NULL);
		g_ascii_dtostr (buffer, sizeof (buffer), (double) f);
		return g_strdup (buffer);
	case MRP_PROPERTY_TYPE_DURATION:
		mrp_object_get (object, name, &i, NULL);
		return g_strdup_printf ("%d", i);
	case MRP_PROPERTY_TYPE_DATE:
		mrp_object_get (object, name, &date, NULL);
		str = mrp_time_to_string (date);		
		return str;
	case MRP_PROPERTY_TYPE_COST:
		mrp_object_get (object, name, &f, NULL);
		g_ascii_dtostr (buffer, sizeof (buffer), (double) f);
		return g_strdup (buffer);
	default:
		g_warning ("Not implemented support for type %d",
			   mrp_property_get_property_type (property));
		break;
	}

	return NULL;
}

static gboolean
sql_write_property_specs (SQLData *data)
{
	GdaDataModel *res;
	gchar        *query;
	
	GList        *properties, *l;
	const gchar  *name, *label, *description, *type;
	MrpProperty  *property;
	gint          id;

	/* Project custom properties. */
	properties = mrp_project_get_properties_from_type (data->project, MRP_TYPE_PROJECT);
	for (l = properties; l; l = l->next) {
		property = l->data;

		name = mrp_property_get_name (property);
		label = mrp_property_get_label (property);
		description = mrp_property_get_description (property);
		type = property_type_to_string (mrp_property_get_property_type (property));

		query = g_strdup_printf ("INSERT INTO property_type(proj_id, name, label, type, owner, descr) "
					 "VALUES(%d, '%s', '%s', '%s', 'project', '%s')",
					 data->project_id,
					 name, label, type, description);
		res = sql_execute_query (data->con, query); 
		g_free (query);

		if (res == NULL) {
			g_warning ("INSERT command failed (property_type) %s.",
					sql_get_last_error (data->con));
			goto out;
		}

		id = get_inserted_id (data, "property_type_proptype_id_seq");
		d(g_print ("Inserted property type '%s', %d\n", name, id));

		g_hash_table_insert (data->property_type_hash, property, GINT_TO_POINTER (id));
	}

	/* Task custom properties. */
	properties = mrp_project_get_properties_from_type (
		data->project, MRP_TYPE_TASK);
	for (l = properties; l; l = l->next) {
		property = l->data;

		name = mrp_property_get_name (property);
		label = mrp_property_get_label (property);
		description = mrp_property_get_description (property);
		type = property_type_to_string (mrp_property_get_property_type (property));

		query = g_strdup_printf ("INSERT INTO property_type(proj_id, name, label, type, owner, descr) "
					 "VALUES(%d, '%s', '%s', '%s', 'task', '%s')",
					 data->project_id,
					 name, label, type, description);
		res = sql_execute_query (data->con, query); 
		g_free (query);

		if (res == NULL) {
			g_warning ("INSERT command failed (property_type) %s.",
					sql_get_last_error (data->con));
			goto out;
		}

		id = get_inserted_id (data, "property_type_proptype_id_seq");
		d(g_print ("Inserted property type '%s', %d\n", name, id));

		g_hash_table_insert (data->property_type_hash, property, GINT_TO_POINTER (id));
	}

	/* Resource custom properties. */
	properties = mrp_project_get_properties_from_type (
		data->project, MRP_TYPE_RESOURCE);
	for (l = properties; l; l = l->next) {
		property = l->data;

		name = mrp_property_get_name (property);
		label = mrp_property_get_label (property);
		description = mrp_property_get_description (property);
		type = property_type_to_string (mrp_property_get_property_type (property));

		query = g_strdup_printf ("INSERT INTO property_type(proj_id, name, label, type, owner, descr) "
					 "VALUES(%d, '%s', '%s', '%s', 'resource', '%s')",
					 data->project_id,
					 name, label, type, description);
		res = sql_execute_query (data->con, query); 
		g_free (query);

		if (res == NULL) {
			g_warning ("INSERT command failed (property_type) %s.",
					sql_get_last_error (data->con));
			goto out;
		}

		id = get_inserted_id (data, "property_type_proptype_id_seq");
		d(g_print ("Inserted property type '%s', %d\n", name, id));

		g_hash_table_insert (data->property_type_hash, property, GINT_TO_POINTER (id));
	}

	return TRUE;
	
 out:
	if (res) {
		g_object_unref (res);
	}
	
	return FALSE;
}

static gboolean
sql_write_property_values (SQLData   *data,
				  MrpObject *object)
{
	GdaDataModel *res;
	gchar        *query;
	
	GType         object_type;
	GList        *properties, *l;
	const gchar  *name, *label, *description, *type;
	const gchar  *str;
	gchar        *value;
	MrpProperty  *property;
	gint          property_type_id;
	gint          id;
	gint          object_id;

	object_type = G_OBJECT_TYPE (object);

	/* Write custom property values. */
	properties = mrp_project_get_properties_from_type (data->project, object_type);
	for (l = properties; l; l = l->next) {
		property = l->data;
		
		if (mrp_property_get_property_type (property) == MRP_PROPERTY_TYPE_STRING_LIST) {
			g_warning ("Don't support string list.");
			continue;
		}

		name = mrp_property_get_name (property);
		label = mrp_property_get_label (property);
		description = mrp_property_get_description (property);
		type = property_type_to_string (mrp_property_get_property_type (property));

		property_type_id = get_hash_data_as_id (data->property_type_hash, property);
		
		value = property_to_string (object, property);

		if (value) {
			query = g_strdup_printf ("INSERT INTO property(proptype_id, value) "
						 "VALUES(%d, '%s')", property_type_id, value);
		} else {
			query = g_strdup_printf ("INSERT INTO property(proptype_id, value) "
						 "VALUES(%d, NULL)", property_type_id);
		}
		
		res = sql_execute_query (data->con, query); 
		g_free (query);
		g_free (value);

		if (res == NULL) {
			g_warning ("INSERT command failed (property) %s.",
					sql_get_last_error (data->con));
			goto out;
		}

		id = get_inserted_id (data, "property_prop_id_seq");
		d(g_print ("Inserted property '%s', %d\n", name, id));

		if (object_type == MRP_TYPE_PROJECT) {
			str = "project_to_property(proj_id, prop_id)";
			object_id = data->project_id;
		}
		else if (object_type == MRP_TYPE_TASK) {
			str = "task_to_property(task_id, prop_id)";
			object_id = get_hash_data_as_id (data->task_hash, object);
		}
		else if (object_type == MRP_TYPE_RESOURCE) {
			str = "resource_to_property(res_id, prop_id)";
			object_id = get_hash_data_as_id (data->resource_hash, object);
		} else {
			str = NULL;
			object_id = -1;
			g_assert_not_reached ();
		}
		
		query = g_strdup_printf ("INSERT INTO %s "
					 "VALUES(%d, %d)",
					 str,
					 object_id, id);
		res = sql_execute_query (data->con, query); 
		g_free (query);

		if (res == NULL) {
			g_warning ("INSERT command failed (*_to_property) %s.",
					sql_get_last_error (data->con));
			goto out;
		}
	}

	return TRUE;
	
 out:
	if (res) {
		g_object_unref (res);
	}

	return FALSE;
}

static gboolean
sql_write_day_types (SQLData *data)
{
	GdaDataModel *res;	
	gchar        *query;
	
	GList        *days, *l;
	MrpDay       *day;
	gint          id;
	const gchar  *is_work;
	const gchar  *is_nonwork;

	days = g_list_copy (mrp_day_get_all (data->project));
	days = g_list_prepend (days, mrp_day_get_work ());
	days = g_list_prepend (days, mrp_day_get_nonwork ());

	for (l = days; l; l = l->next) {
		day = l->data;

		is_work = "false";
		
		if (day == mrp_day_get_work ()) {
			is_work = "true";
			is_nonwork = "false";
		}
		else if (day == mrp_day_get_nonwork ()) {
			is_nonwork = "true";
			is_work = "false";
		} else {
			is_nonwork = "false";
			is_work = "false";
		}			
		
		query = g_strdup_printf ("INSERT INTO daytype(proj_id, name, descr, is_work, is_nonwork) "
					 "VALUES(%d, '%s', '%s', %s, %s)",
					 data->project_id,
					 mrp_day_get_name (day),
					 mrp_day_get_description (day),
					 is_work, is_nonwork);
		res = sql_execute_query (data->con, query); 
		g_free (query);

		if (res == NULL) {
			g_warning ("INSERT command failed (resource_group) %s.",
					sql_get_last_error (data->con));
			goto out;
		}

		id = get_inserted_id (data, "daytype_dtype_id_seq");
		d(g_print ("Inserted day '%s', %d\n", mrp_day_get_name (day), id));

		g_hash_table_insert (data->day_hash, day, GINT_TO_POINTER (id));
	}

	g_list_free (days);
	
	return TRUE;
	
 out:
	if (res) {
		g_object_unref (res);
	}
	
	return FALSE;
}

static gchar *
get_day_id_string (SQLData *data, MrpCalendar *calendar, gint weekday)
{
	MrpDay *day;
	gint    day_id;
	
	day = mrp_calendar_get_default_day (calendar, weekday);
	day_id = get_hash_data_as_id (data->day_hash, day);

	if (day_id != -1) {
		return g_strdup_printf ("%d", day_id);
	} else {
		return g_strdup ("NULL");
	}
}

static gboolean
sql_write_overridden_day_type (SQLData             *data,
				      MrpCalendar         *calendar,
				      MrpDayWithIntervals *day_ivals)
{
	GdaDataModel *res;
	gchar        *query;
	
	GList        *l;
	gint          calendar_id;
	gint          day_type_id;
	MrpInterval  *ival;
	mrptime       start, end;
	gchar        *start_string, *end_string;

	calendar_id = get_hash_data_as_id (data->calendar_hash, calendar);
	day_type_id = get_hash_data_as_id (data->day_hash, day_ivals->day);
	
	for (l = day_ivals->intervals; l; l = l->next) {
		ival = l->data;
		
		mrp_interval_get_absolute (ival, 0, &start, &end);

		start_string = mrp_time_format ("%H:%M:%S+0", start);
		end_string = mrp_time_format ("%H:%M:%S+0", end);
		
		query = g_strdup_printf ("INSERT INTO day_interval(cal_id, dtype_id, start_time, end_time) "
					 "VALUES(%d, %d, '%s', '%s')",
					 calendar_id, day_type_id,
					 start_string, end_string);
		res = sql_execute_query (data->con, query); 
		g_free (query);
		g_free (start_string);
		g_free (end_string);

		if (res == NULL) {
			g_warning ("INSERT command failed (day_interval) %s.",
					sql_get_last_error (data->con));
			goto out;
		}
	}

	return TRUE;
	
 out:
	if (res) {
		g_object_unref (res);
	}
	
	return FALSE;
}

static gboolean
sql_write_overridden_dates (SQLData        *data,
				   MrpCalendar    *calendar,
				   MrpDateWithDay *date_day)
{
	GdaDataModel *res;
	gchar        *query;
	
	gint          calendar_id;
	gint          day_type_id;
	gchar        *date_string;

	calendar_id = get_hash_data_as_id (data->calendar_hash, calendar);
	day_type_id = get_hash_data_as_id (data->day_hash, date_day->day);

	date_string = mrp_time_format ("%Y-%m-%d %H:%M:%S+0", date_day->date);

	query = g_strdup_printf ("INSERT INTO day(cal_id, dtype_id, date) "
				 "VALUES(%d, %d, '%s')",
				 calendar_id, day_type_id, date_string);
	res = sql_execute_query (data->con, query); 
	g_free (query);
	g_free (date_string);

	if (res == NULL) {
		g_warning ("INSERT command failed (day) %s.",
				sql_get_last_error (data->con));
		goto out;
	}

	return TRUE;
	
 out:
	if (res) {
		g_object_unref (res);
	}
	
	return FALSE;
}

static gboolean
sql_write_calendars_recurse (SQLData     *data,
				    MrpCalendar *parent,
				    MrpCalendar *calendar)
{
	GdaDataModel *res;
	gchar        *query;
	
	GList        *list, *l;
	gint          id;
	gint          parent_id;
	gchar        *parent_id_string;
	gchar        *mon, *tue, *wed, *thu, *fri, *sat, *sun;

	/* Write the calendar. */

	if (!parent) {
		parent_id_string = g_strdup ("NULL");
	} else {
		parent_id = get_hash_data_as_id (data->calendar_hash, parent);
		parent_id_string = g_strdup_printf ("%d", parent_id);
	}

	/* Get the default week. */
	mon = get_day_id_string (data, calendar, MRP_CALENDAR_DAY_MON);
	tue = get_day_id_string (data, calendar, MRP_CALENDAR_DAY_TUE);
	wed = get_day_id_string (data, calendar, MRP_CALENDAR_DAY_WED);
	thu = get_day_id_string (data, calendar, MRP_CALENDAR_DAY_THU);
	fri = get_day_id_string (data, calendar, MRP_CALENDAR_DAY_FRI);
	sat = get_day_id_string (data, calendar, MRP_CALENDAR_DAY_SAT);
	sun = get_day_id_string (data, calendar, MRP_CALENDAR_DAY_SUN);
		
	query = g_strdup_printf ("INSERT INTO calendar(proj_id, parent_cid, name, "
				 "day_mon, day_tue, day_wed, day_thu, day_fri, day_sat, day_sun) "
				 "VALUES(%d, %s, '%s', "
				 "%s, %s, %s, %s, %s, %s, %s)",
				 data->project_id, parent_id_string, mrp_calendar_get_name (calendar),
				 mon, tue, wed, thu, fri, sat, sun);
	res = sql_execute_query (data->con, query); 
	g_free (query);
	g_free (mon);
	g_free (tue);
	g_free (wed);
	g_free (thu);
	g_free (fri);
	g_free (sat);
	g_free (sun);
	
	if (res == NULL) {
		g_warning ("INSERT command failed (calendar) %s.",
				sql_get_last_error (data->con));
		goto out;
	}
	
	id = get_inserted_id (data, "calendar_cal_id_seq");
	d(g_print ("Inserted calendar '%s', %d\n", mrp_calendar_get_name (calendar), id));

	g_hash_table_insert (data->calendar_hash, calendar, GINT_TO_POINTER (id));
	
	g_free (parent_id_string);

	g_object_unref (res);

	/* Write overridden day types. */
	list = mrp_calendar_get_overridden_days (calendar);
	for (l = list; l; l = l->next) {
		if (!sql_write_overridden_day_type (data, calendar, l->data)) {
			goto out;
		}
	}

	/* Write overridden dates. */
	list = mrp_calendar_get_all_overridden_dates (calendar);
	for (l = list; l; l = l->next) {
		if (!sql_write_overridden_dates (data, calendar, l->data)) {
			goto out;
		}
	}
	
	/* Write the calendar's children. */
	list = mrp_calendar_get_children (calendar);
	for (l = list; l; l = l->next) {
		if (!sql_write_calendars_recurse (data, calendar, l->data)) {
			goto out;
		}
	}
	
	return TRUE;
	
 out:
	if (res) {
		g_object_unref (res);
	}
	
	return FALSE;
}

static gboolean
sql_write_calendars (SQLData *data)
{
	MrpCalendar *root;
	GList       *calendars, *l;

	root = mrp_project_get_root_calendar (data->project);
	calendars = mrp_calendar_get_children (root);
	for (l = calendars; l; l = l->next) {
		if (!sql_write_calendars_recurse (data, NULL, l->data)) {
			return FALSE;
		}
	}

	return TRUE;
}

static gboolean
sql_write_calendar_id (SQLData *data)
{
	GdaDataModel *res;
	gchar        *query;
	
	MrpCalendar  *calendar;
	gint          id;

	g_object_get (data->project,
		      "calendar", &calendar,
		      NULL);
	
	id = get_hash_data_as_id (data->calendar_hash, calendar);

	if (id != -1) {
		query = g_strdup_printf ("UPDATE project SET cal_id=%d WHERE proj_id=%d", 
					 id, data->project_id);
	} else {
		query = g_strdup_printf ("UPDATE project SET cal_id=NULL WHERE proj_id=%d", 
					 data->project_id);
	}
	
	res = sql_execute_query (data->con, query); 
	g_free (query);
	
	if (res == NULL) {
		g_warning ("UPDATE command failed (cal_id) %s.",
				sql_get_last_error (data->con));
		goto out;
	}
	
	return TRUE;
	
 out:
	if (res) {
		g_object_unref (res);
	}
	
	return FALSE;
}

static gboolean
sql_write_groups (SQLData *data)
{
	GdaDataModel *res;
	gchar        *query;
	
	GList        *groups, *l;
	gchar        *name, *manager_name, *manager_phone, *manager_email;	       
	MrpGroup     *group;
	gint          id;

	groups = mrp_project_get_groups (data->project);
	for (l = groups; l; l = l->next) {
		group = l->data;

		g_object_get (group,
			      "name", &name,
			      "manager_name", &manager_name,
			      "manager_phone", &manager_phone,
			      "manager_email", &manager_email,
			      NULL);
		
		query = g_strdup_printf ("INSERT INTO resource_group(proj_id, name, admin_name, admin_phone, admin_email) "
					 "VALUES(%d, '%s', '%s', '%s', '%s')",
					 data->project_id,
					 name,
					 manager_name, manager_phone, manager_email);
		res = sql_execute_query (data->con, query); 
		g_free (query);

		if (res == NULL) {
			g_warning ("INSERT command failed (resource_group) %s.",
					sql_get_last_error (data->con));
			goto out;
		}

		id = get_inserted_id (data, "resource_group_group_id_seq");
		d(g_print ("Inserted group '%s', %d\n", name, id));

		g_hash_table_insert (data->group_hash, group, GINT_TO_POINTER (id));
		
		g_free (name);
		g_free (manager_name);
		g_free (manager_phone);
		g_free (manager_email);
	}

	return TRUE;
	
 out:
	if (res) {
		g_object_unref (res);
	}
	
	return FALSE;
}

static gboolean
sql_write_default_group_id (SQLData *data)
{
	GdaDataModel *res;
	gchar        *query;
	
	MrpGroup     *group;
	gint          id;

	g_object_get (data->project,
		      "default_group", &group,
		      NULL);
	
	id = get_hash_data_as_id (data->group_hash, group);

	if (id != -1) {
		query = g_strdup_printf ("UPDATE project SET default_group_id=%d WHERE proj_id=%d", 
					 id, data->project_id);
	} else {
		query = g_strdup_printf ("UPDATE project SET default_group_id=NULL WHERE proj_id=%d", 
					 data->project_id);
	}
	
	res = sql_execute_query (data->con, query); 
	g_free (query);
	
	if (res == NULL) {
		g_warning ("UPDATE command failed (default_group_id) %s.",
				sql_get_last_error (data->con));
		goto out;
	}

	return TRUE;

 out:
	if (res) {
		g_object_unref (res);
	}
	
	return FALSE;
}

static gboolean
sql_write_resources (SQLData *data)
{
	GdaDataModel    *res;
	gchar           *query;
	
	GList           *resources, *l;
	gchar           *name, *short_name, *email, *note;	       
	MrpResource     *resource;
	MrpCalendar     *calendar;
	MrpGroup        *group;
	MrpResourceType  type;
	gint             id;
	gint             units;
	gint             cal_id;
	gint             group_id;
	const gchar     *is_worker;
	gchar           *cal_id_string;
	gchar           *group_id_string;
	
	resources = mrp_project_get_resources (data->project);
	for (l = resources; l; l = l->next) {
		resource = l->data;

		g_object_get (resource,
			      "name", &name,
			      "short_name", &short_name,
			      "email", &email,
			      "note", &note,
			      "units", &units,
			      "calendar", &calendar,
			      "group", &group,
			      "type", &type,
			      NULL);

		is_worker = (type == MRP_RESOURCE_TYPE_WORK) ? "true" : "false";

		cal_id = get_hash_data_as_id (data->calendar_hash, calendar);
		group_id = get_hash_data_as_id (data->group_hash, group);

		if (cal_id != -1) {
			cal_id_string = g_strdup_printf ("%d", cal_id);
		} else {
			cal_id_string = g_strdup ("NULL");
		}

		if (group_id != -1) {
			group_id_string = g_strdup_printf ("%d", group_id);
		} else {
			group_id_string = g_strdup ("NULL");
		}

		query = g_strdup_printf ("INSERT INTO resource(proj_id, group_id, name, "
					 "short_name, email, note, is_worker, units, cal_id) "
					 "VALUES(%d, %s, '%s', "
					 "'%s', '%s', '%s', %s, %g, %s)",
					 data->project_id, group_id_string, name,
					 short_name, email, note, is_worker, (double) units,
					 cal_id_string);
		res = sql_execute_query (data->con, query); 
		g_free (query);
		g_free (cal_id_string);
		g_free (group_id_string);
		g_free (note);
		g_free (email);
		g_free (short_name);

		if (res == NULL) {
			g_warning ("INSERT command failed (resource) %s.",
					sql_get_last_error (data->con));
			goto out;
		}

		id = get_inserted_id (data, "resource_res_id_seq");
		d(g_print ("Inserted resource '%s', %d\n", name, id));
		
		g_hash_table_insert (data->resource_hash, resource, GINT_TO_POINTER (id));
		
		g_free (name);
	}

	/* Write resource property values. */
	for (l = resources; l; l = l->next) {
		resource = l->data;

		if (!sql_write_property_values (data, MRP_OBJECT (resource))) {
			goto out;
		}
	}
	
	return TRUE;
	
 out:
	if (res) {
		g_object_unref (res);
	}
	
	return FALSE;
}

static gboolean
sql_write_tasks (SQLData *data)
{
	GdaDataModel    *res;
	gchar           *query;	
	GList           *tasks, *l;
	gchar           *name, *note;	       
	MrpTask         *task;
	MrpTask         *parent;
	MrpTaskType      type;
	MrpTaskSched     sched;
	gint             id, parent_id;
	mrptime          start, finish;
	gint             work, duration;
	gint             percent_complete;
	gint             priority;
	const gchar     *is_fixed_work;
	const gchar     *is_milestone;
	MrpConstraint   *constraint;
	const gchar     *constraint_type;
	gchar           *constraint_time;
	gchar           *parent_id_string;
	gchar           *start_string;
	gchar           *finish_string;
	GList           *predecessors, *p;
	MrpRelation     *relation;
	MrpTask         *predecessor;
	const gchar     *relation_type;
	gint             lag;
	gint             pred_id;
	GList           *assignments, *a;
	gint             units;
	gint             resource_id;
	MrpAssignment   *assignment;
	MrpResource     *resource;
	
	/* Note: we depend on the tasks being returned with parents before
	 * children.
	 */
	tasks = mrp_project_get_all_tasks (data->project);
	for (l = tasks; l; l = l->next) {
		task = l->data;

		g_object_get (task,
			      "name", &name,
			      "note", &note,
			      "work", &work,
			      "percent_complete", &percent_complete,
			      "priority", &priority,
			      "duration", &duration,
			      "start", &start,
			      "finish", &finish,
			      "type", &type,
			      "sched", &sched,
			      "constraint", &constraint,
			      NULL);
		
		parent = mrp_task_get_parent (task);
		parent_id = get_hash_data_as_id (data->task_hash, parent);

		if (parent_id != -1) {
			parent_id_string = g_strdup_printf ("%d", parent_id);
		} else {
			parent_id_string = g_strdup ("NULL");
		}
		
		if (type == MRP_TASK_TYPE_MILESTONE) {
			work = 0;
			duration = 0;
			is_milestone = "true";
		} else {
			is_milestone = "false";
		}
		
		if (sched == MRP_TASK_SCHED_FIXED_WORK) {
			is_fixed_work = "true";
		} else {
			is_fixed_work = "false";
		}
		
		start_string = mrp_time_format ("%Y-%m-%d %H:%M:%S+0", start);
		finish_string = mrp_time_format ("%Y-%m-%d %H:%M:%S+0", finish);

		if (constraint) {
			switch (constraint->type) {
			case MRP_CONSTRAINT_MSO:
				constraint_type = "MSO";
				constraint_time = mrp_time_format ("'%Y-%m-%d %H:%M:%S+0'", constraint->time);
				break;
			case MRP_CONSTRAINT_SNET:
				constraint_type = "SNET";
				constraint_time = mrp_time_format ("'%Y-%m-%d %H:%M:%S+0'", constraint->time);
				break;
			case MRP_CONSTRAINT_FNLT:
				constraint_type = "FNLT";
				constraint_time = mrp_time_format ("'%Y-%m-%d %H:%M:%S+0'", constraint->time);
				break;
			default:
			case MRP_CONSTRAINT_ASAP:
				constraint_type = "ASAP";
				constraint_time = NULL;
				break;
			}
		} else {
			constraint_type = "ASAP";
			constraint_time = NULL;
		}

		if (!constraint_time) {
			constraint_time = g_strdup ("NULL");
		}
		
		query = g_strdup_printf ("INSERT INTO task(proj_id, parent_id, name, "
					 "note, start, finish, work, duration, "
					 "percent_complete, is_milestone, is_fixed_work, "
					 "constraint_type, constraint_time, priority) "
					 "VALUES(%d, %s, '%s', "
					 "'%s', '%s', '%s', %d, %d, "
					 "%d, %s, %s, "
					 "'%s', %s, %d)",
					 data->project_id, parent_id_string, name,
					 note, start_string, finish_string, work, duration,
					 percent_complete, is_milestone, is_fixed_work,
					 constraint_type, constraint_time, priority);

		res = sql_execute_query (data->con, query); 
		g_free (query);
		g_free (start_string);
		g_free (finish_string);
		g_free (parent_id_string);
		g_free (note);
		g_free (constraint_time);
		g_free (constraint);

		if (res == NULL) {
			g_warning ("INSERT command failed (task) %s.",
					sql_get_last_error (data->con));
			goto out;
		}

		id = get_inserted_id (data, "task_task_id_seq");
		d(g_print ("Inserted task '%s', %d under %d\n", name, id, parent_id));

		g_hash_table_insert (data->task_hash, task, GINT_TO_POINTER (id));
		
		g_free (name);
	}

	/* Write predecessor relations. */
	for (l = tasks; l; l = l->next) {
		task = l->data;

		predecessors = mrp_task_get_predecessor_relations (task);

		for (p = predecessors; p; p = p->next) {
			relation = p->data;

			predecessor = mrp_relation_get_predecessor (relation);

			switch (mrp_relation_get_relation_type (relation)) {
			case MRP_RELATION_FS:
				relation_type = "FS";
				break;
			case MRP_RELATION_FF:
				relation_type = "FF";
				break;
			case MRP_RELATION_SF:
				relation_type = "SF";
				break;
			case MRP_RELATION_SS:
				relation_type = "SS";
				break;
			default:
				relation_type = "FS";
				break;
			}

			lag = mrp_relation_get_lag (relation);

			id = get_hash_data_as_id (data->task_hash, task);
			pred_id = get_hash_data_as_id (data->task_hash, predecessor);
			
			query = g_strdup_printf ("INSERT INTO predecessor(task_id, pred_task_id, "
						 "type, lag) "
						 "VALUES(%d, %d, '%s', %d)",
						 id, pred_id, relation_type, lag);
			res = sql_execute_query (data->con, query); 
			g_free (query);
			
			if (res == NULL) {
				g_warning ("INSERT command failed (predecessor) %s.",
						sql_get_last_error (data->con));
				goto out;
			}
		}
	}

	/* Write task property values. */
	for (l = tasks; l; l = l->next) {
		task = l->data;

		if (!sql_write_property_values (data, MRP_OBJECT (task))) {
			goto out;
		}
	}
	
	/* Write resource assignments. */
	for (l = tasks; l; l = l->next) {
		task = l->data;

		assignments = mrp_task_get_assignments (task);
		
		for (a = assignments; a; a = a->next) {
			gchar tmp[G_ASCII_DTOSTR_BUF_SIZE];
			
			assignment = a->data;

			resource = mrp_assignment_get_resource (assignment);
			units = mrp_assignment_get_units (assignment);

			id = get_hash_data_as_id (data->task_hash, task);
			resource_id = get_hash_data_as_id (data->resource_hash, resource);

			g_ascii_dtostr (tmp, G_ASCII_DTOSTR_BUF_SIZE, units / 100.0); 
			
			query = g_strdup_printf ("INSERT INTO allocation(task_id, res_id, units) "
						 "VALUES(%d, %d, %s)",
						 id, resource_id, tmp);
			
			res = sql_execute_query (data->con, query); 
			g_free (query);
			
			if (res == NULL) {
				g_warning ("INSERT command failed (allocation) %s.",
						sql_get_last_error (data->con));
				goto out;
			}
		}
	}
	
	g_list_free (tasks);
	
	return TRUE;
	
 out:
	if (res) {
		g_object_unref (res);
	}
	
	return FALSE;
}

gboolean
mrp_sql_save_project (MrpStorageSQL  *storage,
		      gboolean        force,
		      const gchar    *server,
		      const gchar    *port,
		      const gchar    *db,
		      const gchar    *user,
		      const gchar    *password,
		      gint           *project_id,
		      GError        **error)
{
	SQLData      *data;
	gchar        *db_txt = NULL;
	const gchar  *dsn_name = "planner-auto";
	GdaDataModel *res = NULL;
	GdaClient    *client;
	gboolean      ret = FALSE;
	const gchar  *provider = "PostgreSQL";

	data = g_new0 (SQLData, 1);
	data->project_id = *project_id;
	data->day_id_hash = g_hash_table_new (NULL, NULL);
	data->calendar_id_hash = g_hash_table_new (NULL, NULL);
	data->group_id_hash = g_hash_table_new (NULL, NULL);
	data->task_id_hash = g_hash_table_new (NULL, NULL);
	data->resource_id_hash = g_hash_table_new (NULL, NULL);

	data->calendar_hash = g_hash_table_new (NULL, NULL);
	data->day_hash = g_hash_table_new (NULL, NULL);
	data->group_hash = g_hash_table_new (NULL, NULL);
	data->task_hash = g_hash_table_new (NULL, NULL);
	data->resource_hash = g_hash_table_new (NULL, NULL);
	data->property_type_hash = g_hash_table_new (NULL, NULL);
	
	data->project = storage->project;

	db_txt = g_strdup_printf ("DATABASE=%s",db);
	gda_config_save_data_source (dsn_name, 
                                     provider,
				     db_txt,
                                     "planner project", user, password);
	g_free (db_txt);

	client = gda_client_new ();

	data->con = gda_client_open_connection (client, dsn_name, NULL, NULL, 0);
	
	data->revision = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (data->project), REVISION));

	if (!GDA_IS_CONNECTION (data->con)) {
		g_set_error (error,
			     MRP_ERROR,
			     MRP_ERROR_SAVE_WRITE_FAILED,
			     _("Connection to database '%s' failed."), db);
		goto out;
	}

	res = sql_execute_query (data->con, "BEGIN");

	if (res == NULL) {
		WRITE_ERROR (error, data->con);
		goto out;
	}
	g_object_unref (res);
	res = NULL;

	/* Write project. */
	if (!sql_write_project (storage, data, force, error)) {
		goto out;
	}

	/* Write phases. */
	if (!sql_write_phases (data)) {
		g_warning ("Couldn't write project phases.");
	}

	/* Write project phase. */
	if (!sql_write_phase (data)) {
		g_warning ("Couldn't write project phase id.");
	}

	/* Write custom property specs. */
	if (!sql_write_property_specs (data)) {
		g_warning ("Couldn't write property specs.");
	}

	/* Write project property values. */
	if (!sql_write_property_values (data, MRP_OBJECT (data->project))) {
		g_warning ("Couldn't write project property values.");
	}

	/* Write day types. */
	if (!sql_write_day_types (data)) {
		g_warning ("Couldn't write day types.");
	}
	
	/* Write calendars. */
	if (!sql_write_calendars (data)) {
		g_warning ("Couldn't write calendars.");
	}
	
	/* Write project calendar id. */
	if (!sql_write_calendar_id (data)) {
		g_warning ("Couldn't write project calendar id.");
	}
	
	/* Write resource groups. */
	if (!sql_write_groups (data)) {
		g_warning ("Couldn't write resource groups.");
	}

	/* Write default group id. */
	if (!sql_write_default_group_id (data)) {
		g_warning ("Couldn't write default groups.");
	}
	
	/* Write resources. */
	if (!sql_write_resources (data)) {
		g_warning ("Couldn't write resources.");
	}

	/* Write tasks. */
	if (!sql_write_tasks (data)) {
		g_warning ("Couldn't write tasks.");
	}

	res = sql_execute_query (data->con, "COMMIT");
	g_object_unref (res);
	res = NULL;

	d(g_print ("Write project, set rev to %d\n", data->revision));
	
	g_object_set_data (G_OBJECT (data->project), REVISION, GINT_TO_POINTER (data->revision));

	*project_id = data->project_id;
	
	ret = TRUE;
	
 out:
	if (res) {
		g_object_unref (res);
	}

	if (data->con) {
		g_object_unref (data->con);
	}
	
	/* FIXME: free more data */

	g_hash_table_destroy (data->day_id_hash);
	g_hash_table_destroy (data->calendar_id_hash);
	g_hash_table_destroy (data->group_id_hash);
	g_hash_table_destroy (data->task_id_hash);
	g_hash_table_destroy (data->resource_id_hash);

	g_hash_table_destroy (data->calendar_hash);
	g_hash_table_destroy (data->day_hash);
	g_hash_table_destroy (data->group_hash);
	g_hash_table_destroy (data->task_hash);
	g_hash_table_destroy (data->resource_hash);
	g_hash_table_destroy (data->property_type_hash);

	g_list_free (data->calendars);
	g_list_free (data->tasks);
	
	g_free (data);
	
	return ret;
}

