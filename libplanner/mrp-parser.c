/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2003  CodeFactory AB
 * Copyright (C) 2001-2003  Richard Hult <richard@imendio.com>
 * Copyright (C) 2001-2002  Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2002       Alvaro del Castillo <acs@barrapunto.com>
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
 *
 */

#include <config.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <glib.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include "mrp-private.h"
#include "mrp-error.h"
#include "mrp-intl.h"
#include "mrp-task.h"
#include "mrp-resource.h"
#include "mrp-group.h"
#include "mrp-relation.h"
#include "mrp-parser.h"

/* WARNING: This code is a hack just to have a file loader/saver for the old
 * format. Don't expect to understand any of the code or anything. It sucks.
 */

typedef struct {
        xmlDocPtr   doc;

	gint        version;
	
	MrpProject *project;
	
	MrpTask    *root_task;
	GList      *resources;
	GList      *groups;
	GList      *assignments;

	mrptime     project_start;
	
	MrpGroup   *default_group;

	gint        last_id;

	gint        next_day_type_id;
	gint        next_calendar_id;
	
	GHashTable *task_hash;
	GHashTable *resource_hash;
	GHashTable *group_hash;
	GHashTable *day_hash;
	GHashTable *calendar_hash;
	GList      *delayed_relations;
} MrpParser;

typedef struct {
	xmlNodePtr node;
	gint       id;
} NodeEntry;


static void             mpp_xml_set_date              (xmlNodePtr   node,
						       const gchar *prop,
						       mrptime      time);
static void             mpp_xml_set_int               (xmlNodePtr   node,
						       const gchar *prop,
						       gint         value);
static void             mpp_xml_set_float             (xmlNodePtr   node,
						       const gchar *prop, 
						       gfloat       value);
static void             mpp_xml_set_task_type         (xmlNodePtr   node,
						       const gchar *prop,
						       MrpTaskType  type);
static void             mpp_xml_set_task_sched        (xmlNodePtr   node,
						       const gchar *prop,
						       MrpTaskSched sched);
static xmlNodePtr       mpp_xml_search_child          (xmlNodePtr   node, 
						       const gchar *name);
static gchar           *mpp_property_to_string        (MrpObject   *object,
						       MrpProperty *property);

static void
mpp_write_project_properties (MrpParser *parser, xmlNodePtr node)
{
	gchar       *name;
	gchar       *org;
	gchar       *manager;
	gchar       *phase;
	mrptime      pstart;
	MrpCalendar *calendar;
	gint         id;

	g_object_get (parser->project,
		      "name", &name,
		      "organization", &org,
		      "manager", &manager,
		      "project-start", &pstart,
		      "calendar", &calendar,
		      "phase", &phase,
		      NULL);

	xmlSetProp (node, "name", name);
	xmlSetProp (node, "company", org);
	xmlSetProp (node, "manager", manager);
	xmlSetProp (node, "phase", phase);

	mpp_xml_set_date (node, "project-start", pstart);
	mpp_xml_set_int (node, "mrproject-version", 2);

	if (calendar) {
		id = GPOINTER_TO_INT (g_hash_table_lookup (parser->calendar_hash,
							   calendar));
		
		if (id) {
			mpp_xml_set_int (node, "calendar", id);
		}
	}
	
	g_free (name);
	g_free (org);
	g_free (manager);
	g_free (phase);
}

static const gchar *
mpp_property_type_to_string (MrpPropertyType type)
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

	return NULL;
}

static gchar *
mpp_property_to_string (MrpObject   *object,
			MrpProperty *property)
{
	const gchar *name;
	gchar       *str;
	gchar        buffer[G_ASCII_DTOSTR_BUF_SIZE];
	gint         i;
	gfloat       f;
	mrptime      date;
	GValueArray *array;
	
	name = mrp_property_get_name (property);
	
	switch (mrp_property_get_property_type (property)) {
	case MRP_PROPERTY_TYPE_STRING:
		mrp_object_get (object, name, &str, NULL);
		return str;
	case MRP_PROPERTY_TYPE_STRING_LIST:
		mrp_object_get (object, name, &array, NULL);
		if (!array) {
			return NULL;
		}

		/* FIXME */
		str = g_strdup ("text-list-foo");

		return str;
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
		/* FIXME: implement cost */
		return NULL;
	default:
		g_warning ("Not implemented support for type %d",
			   mrp_property_get_property_type (property));
		break;
	}

	return NULL;
}

static void
mpp_write_property_specs (MrpParser *parser, xmlNodePtr node)
{
	GList           *properties, *l;
	xmlNodePtr       child;
	MrpProperty     *property;
	MrpPropertyType  type;
	
	properties = mrp_project_get_properties_from_type (parser->project, 
							   MRP_TYPE_PROJECT);
	
	node = xmlNewChild (node, NULL, "properties", NULL);

	for (l = properties; l; l = l->next) {
		property = l->data;

		child = xmlNewChild (node, NULL, "property", NULL);
		
		xmlSetProp (child, "name", mrp_property_get_name (property));

		type = mrp_property_get_property_type (property);
		xmlSetProp (child, "type", mpp_property_type_to_string (type));
		
		xmlSetProp (child, "owner", "project");
		xmlSetProp (child, "label", mrp_property_get_label (property));
		xmlSetProp (child, "description", mrp_property_get_description (property));
	}

	g_list_free (properties);

	properties = mrp_project_get_properties_from_type (parser->project, 
							   MRP_TYPE_TASK);
	
	for (l = properties; l; l = l->next) {
		property = l->data;

		child = xmlNewChild (node, NULL, "property", NULL);
		
		xmlSetProp (child, "name", mrp_property_get_name (property));

		type = mrp_property_get_property_type (property);
		xmlSetProp (child, "type", mpp_property_type_to_string (type));
		
		xmlSetProp (child, "owner", "task");
		xmlSetProp (child, "label", mrp_property_get_label (property));
		xmlSetProp (child, "description", mrp_property_get_description (property));
	}

	g_list_free (properties);

	properties = mrp_project_get_properties_from_type (parser->project, 
							   MRP_TYPE_RESOURCE);
	
	for (l = properties; l; l = l->next) {
		property = l->data;

		child = xmlNewChild (node, NULL, "property", NULL);
		
		xmlSetProp (child, "name", mrp_property_get_name (property));

		type = mrp_property_get_property_type (property);
		xmlSetProp (child, "type", mpp_property_type_to_string (type));
		
		xmlSetProp (child, "owner", "resource");
		xmlSetProp (child, "label", mrp_property_get_label (property));
		xmlSetProp (child, "description", mrp_property_get_description (property));
	}

	g_list_free (properties);
}

static void
mpp_write_phases (MrpParser *parser, xmlNodePtr node)
{
	GList      *phases, *l;
	xmlNodePtr  child;
	
	g_object_get (parser->project, "phases", &phases, NULL);
	
	node = xmlNewChild (node, NULL, "phases", NULL);

	for (l = phases; l; l = l->next) {
		child = xmlNewChild (node, NULL, "phase", NULL);
		
		xmlSetProp (child, "name", l->data);
	}

	mrp_string_list_free (phases);
}

static void
mpp_write_predecessor (MrpParser   *parser,
		       xmlNodePtr   node,
		       MrpRelation *relation)
{
	gchar     *str;
	NodeEntry *entry;
	gint       lag;

	node = xmlNewChild (node, NULL, "predecessor", NULL);

	xmlSetProp (node, "id", "1"); /* Don't need id here. */

	entry = g_hash_table_lookup (parser->task_hash,
				     mrp_relation_get_predecessor (relation));
	mpp_xml_set_int (node, "predecessor-id", entry->id);

	switch (mrp_relation_get_relation_type (relation)) {
	case MRP_RELATION_FS:
		str = "FS";
		break;
	case MRP_RELATION_FF:
		str = "FF";
		break;
	case MRP_RELATION_SF:
		str = "SF";
		break;
	case MRP_RELATION_SS:
		str = "SS";
		break;
	default:
		str = "FS";
	}

	xmlSetProp (node, "type", str);

	lag = mrp_relation_get_lag (relation);
	if (lag) {
		mpp_xml_set_int (node, "lag", lag);
	}
}

static gboolean
mpp_hash_insert_task_cb (MrpTask *task, MrpParser *parser)
{
	NodeEntry *entry;
	
	/* Don't want the root task. */
	if (task == parser->root_task) {
		return FALSE;
	}
	
	entry = g_new0 (NodeEntry, 1);
	entry->id = parser->last_id++;
	
	g_hash_table_insert (parser->task_hash, task, entry);
	
	return FALSE;
}

static void
mpp_write_constraint (xmlNodePtr node, MrpConstraint *constraint)
{
	xmlNodePtr   child;
	const gchar *str = NULL;
	
	/* No need to save if we have ASAP. */
	if (constraint->type == MRP_CONSTRAINT_ASAP) {
		return;
	}

	child = xmlNewChild (node, NULL, "constraint", NULL);

	switch (constraint->type) {
	case MRP_CONSTRAINT_MSO:
		str = "must-start-on";
		break;
	case MRP_CONSTRAINT_SNET:
		str = "start-no-earlier-than";
		break;
	case MRP_CONSTRAINT_FNLT:
		str = "finish-no-later-than";
		break;
	case MRP_CONSTRAINT_ASAP:
	case MRP_CONSTRAINT_ALAP:
		g_assert_not_reached ();
		break;
	}	

	xmlSetProp (child, "type", str);
	mpp_xml_set_date (child, "time", constraint->time);
}

static void
mpp_write_string_list (xmlNodePtr   node,
		       MrpProperty *property,
		       MrpObject   *object)
{
	xmlNodePtr   child;
	GValueArray *array;
	GValue      *value;
	gint         i;
	
	mrp_object_get (object, mrp_property_get_name (property), &array, NULL);
	if (!array) {
		return;
	}
	
	for (i = 0; i < array->n_values; i++) {
		value = g_value_array_get_nth (array, i);
		
		child = xmlNewChild (node, NULL, "list-item", NULL);
		xmlSetProp (child, "value", g_value_get_string (value));
	}

	g_value_array_free (array);
}

static void
mpp_write_custom_properties (MrpParser  *parser,
			     xmlNodePtr  node,
			     MrpObject  *object)
{
	GList           *properties, *l;
	xmlNodePtr       child;
	MrpProperty     *property;
	gchar           *value;

	properties = mrp_project_get_properties_from_type (parser->project,
							   G_OBJECT_TYPE (object));
	
	if (properties == NULL) {
		return;
	}
	
	node = xmlNewChild (node, NULL, "properties", NULL);

	for (l = properties; l; l = l->next) {
		property = l->data;
		
		child = xmlNewChild (node, NULL, "property", NULL);
		
		xmlSetProp (child, "name", mrp_property_get_name (property));

		if (mrp_property_get_property_type (property) == MRP_PROPERTY_TYPE_STRING_LIST) {
			mpp_write_string_list (child, property, object);
		} else {
			value = mpp_property_to_string (object, property);
			
			xmlSetProp (child, "value", value);

			g_free (value);
		}
	}

	g_list_free (properties);
}

static gboolean
mpp_write_task_cb (MrpTask *task, MrpParser *parser)
{
	MrpTask       *parent;
	NodeEntry     *entry;
	xmlNodePtr     node, parent_node;
	gchar         *name;
	gchar         *note;
	mrptime        start, finish;
	MrpConstraint *constraint;
	gint           duration;
	gint           work;
	gint           complete;
	gint           priority;
	MrpTaskType    type;
	MrpTaskSched   sched;
	GList         *predecessors, *l;

	/* Don't want the root task. */
	if (task == parser->root_task) {
		return FALSE;
	}
	
	parent = mrp_task_get_parent (task);

	entry = g_hash_table_lookup (parser->task_hash, parent);
	parent_node = entry->node;

	node = xmlNewChild (parent_node, NULL, "task", NULL);

	entry = g_hash_table_lookup (parser->task_hash, task);
	entry->node = node;
	
	g_object_get (task,
		      "name", &name,
		      "note", &note,
		      "start", &start,
		      "finish", &finish,
		      "duration", &duration,
		      "work", &work,
		      "constraint", &constraint,
		      "percent-complete", &complete,
		      "priority", &priority,
		      "type", &type,
		      "sched", &sched,
		      NULL);

	if (type == MRP_TASK_TYPE_MILESTONE) {
		finish = start;
		work = 0;
		duration = 0;
	}
	
	mpp_xml_set_int (node, "id", entry->id);
	xmlSetProp (node, "name", name);
	xmlSetProp (node, "note", note);
	mpp_xml_set_int (node, "work", work);

	if (sched == MRP_TASK_SCHED_FIXED_DURATION) {
		mpp_xml_set_int (node, "duration", duration);
	}
	
	mpp_xml_set_date (node, "start", start);
	mpp_xml_set_date (node, "end", finish);

	mpp_xml_set_int (node, "percent-complete", complete);

	mpp_xml_set_int (node, "priority", priority);

	mpp_xml_set_task_type (node, "type", type);

	mpp_xml_set_task_sched (node, "scheduling", sched);

	mpp_write_custom_properties (parser, node, MRP_OBJECT (task));
	
	mpp_write_constraint (node, constraint);
	
	predecessors = mrp_task_get_predecessor_relations (task);
	if (predecessors != NULL) {
		node = xmlNewChild (node, NULL, "predecessors", NULL);
		for (l = predecessors; l; l = l->next) {
			mpp_write_predecessor (parser, node, l->data);
		}
	}

	g_free (name);
	g_free (note);
	
	return FALSE;
}

static void
mpp_hash_insert_group (MrpParser *parser, MrpGroup *group)
{
	NodeEntry *entry;

	entry = g_new0 (NodeEntry, 1);

	entry->id = parser->last_id++;

	g_hash_table_insert (parser->group_hash, group, entry);
}

static void
mpp_write_group (MrpParser *parser, xmlNodePtr parent, MrpGroup *group)
{
	NodeEntry  *entry;
	xmlNodePtr  node;
	gchar      *name, *admin_name, *admin_phone, *admin_email;

	g_return_if_fail (MRP_IS_GROUP (group));

	node = xmlNewChild (parent,
                            NULL,
			    "group",
			    NULL);

	entry = g_hash_table_lookup (parser->group_hash, group);
	entry->node = node;
	
	mpp_xml_set_int (node, "id", entry->id);
	
	g_object_get (group,
		      "name", &name,
		      "manager-name", &admin_name,
		      "manager-phone", &admin_phone,
		      "manager-email", &admin_email,
		      NULL);

	xmlSetProp (node, "name", name);	
	xmlSetProp (node, "admin-name", admin_name);	
	xmlSetProp (node, "admin-phone", admin_phone);	
	xmlSetProp (node, "admin-email", admin_email);

	g_free (name);
	g_free (admin_name);
	g_free (admin_phone);
	g_free (admin_email);
}

static void
mpp_hash_insert_resource (MrpParser *parser, MrpResource *resource)
{
	NodeEntry  *entry;

	entry = g_new0 (NodeEntry, 1);
	entry->id = parser->last_id++;

	g_hash_table_insert (parser->resource_hash, resource, entry);
}

static void
mpp_write_resource (MrpParser   *parser, 
		    xmlNodePtr   parent,
		    MrpResource *resource)
{
	xmlNodePtr   node;
	gchar       *name, *short_name, *email;
	gchar       *note;
	gint         type, units;
	gfloat       std_rate; /*, ovt_rate;*/
	NodeEntry   *group_entry;
	NodeEntry   *resource_entry;
	MrpGroup    *group;
	MrpCalendar *calendar;
	gint         id;

	g_return_if_fail (MRP_IS_RESOURCE (resource));

	node = xmlNewChild (parent,
                            NULL,
			    "resource",
			    NULL);

	mrp_object_get (MRP_OBJECT (resource),
			"name", &name,
			"short_name", &short_name,
			"email", &email,
			"type", &type,
			"units", &units,
			"group", &group,
			"cost", &std_rate,
			"note", &note,
			/*"cost-overtime", &ovt_rate,*/
			NULL);

	group_entry = g_hash_table_lookup (parser->group_hash, group);

	/* FIXME: should group really be able to be NULL? Should always 
	 * be default group? */
	if (group_entry != NULL) {
		mpp_xml_set_int (node, "group", group_entry->id);
	}

	resource_entry = g_hash_table_lookup (parser->resource_hash, resource);
	mpp_xml_set_int (node, "id", resource_entry->id);
	
	xmlSetProp (node, "name", name);
	xmlSetProp (node, "short-name", short_name);
	
	mpp_xml_set_int (node, "type", type);
	
	mpp_xml_set_int (node, "units", units);
	xmlSetProp (node, "email", email);
	
	xmlSetProp (node, "note", note);

	mpp_xml_set_float (node, "std-rate", std_rate);
	/*mpp_xml_set_float (node, "ovt-rate", ovt_rate);*/

	calendar = mrp_resource_get_calendar (resource);
	if (calendar) {
		id = GPOINTER_TO_INT (g_hash_table_lookup (parser->calendar_hash,
							   calendar));
		
		if (id) {
			mpp_xml_set_int (node, "calendar", id);
		}
	}

	mpp_write_custom_properties (parser, node, MRP_OBJECT (resource));

	g_free (name);
	g_free (short_name);
	g_free (email);
	g_free (note);
}

static void
mpp_write_assignment (MrpParser     *parser,
		      xmlNodePtr     parent,
		      MrpAssignment *assignment)
{
	xmlNodePtr   node;
	MrpTask     *task;
	MrpResource *resource;
	NodeEntry   *resource_entry;
	NodeEntry   *task_entry;
	gint         assigned_units;

	g_return_if_fail (MRP_IS_ASSIGNMENT (assignment));

	node = xmlNewChild (parent,
                            NULL,
			    "allocation",
			    NULL);

	g_object_get (assignment,
		      "task", &task,
		      "resource", &resource,
		      "units", &assigned_units,
		      NULL);

	task_entry = g_hash_table_lookup (parser->task_hash, task);
	resource_entry = g_hash_table_lookup (parser->resource_hash, resource);

	mpp_xml_set_int (node, "task-id", task_entry->id);
	mpp_xml_set_int (node, "resource-id", resource_entry->id);
	mpp_xml_set_int (node, "units", assigned_units);
}

static void
mpp_write_interval (xmlNodePtr parent, MrpInterval *interval)
{
	xmlNodePtr  child;
	mrptime     start, end;
	gchar      *str;
	
	child = xmlNewChild (parent, NULL, "interval", NULL);

	mrp_interval_get_absolute (interval, 0, &start, &end);

	str = mrp_time_format ("%H%M", start);
	xmlSetProp (child, "start", str);
	g_free (str);
	
	str = mrp_time_format ("%H%M", end);
	xmlSetProp (child, "end", str);
	g_free (str);
}

static void
mpp_write_day (MrpParser *parser, xmlNodePtr parent, MrpDay *day)
{
	xmlNodePtr  node;
	NodeEntry  *day_entry;
	
	g_return_if_fail (day != NULL);

	node = xmlNewChild (parent, NULL, "day-type", NULL);

	day_entry = g_new0 (NodeEntry, 1);
	if (day == mrp_day_get_work ()) {
		day_entry->id = MRP_DAY_WORK;
	}
	else if (day == mrp_day_get_nonwork ()) {
		day_entry->id = MRP_DAY_NONWORK;
	}
	else if (day == mrp_day_get_use_base ()) {
		day_entry->id = MRP_DAY_USE_BASE;
	} else {
		day_entry->id = parser->next_day_type_id++;
	}
	
	g_hash_table_insert (parser->day_hash, day, day_entry);

	mpp_xml_set_int (node, "id", day_entry->id);
	xmlSetProp (node, "name", mrp_day_get_name (day));
	xmlSetProp (node, "description", mrp_day_get_description (day));
}

static void 
mpp_write_default_day (MrpParser   *parser,
		       xmlNode     *node, 
		       MrpCalendar *calendar, 
		       const gchar *name, 
		       gint         week_day)
{
	MrpDay    *day;
	NodeEntry *day_entry;
	
	day = mrp_calendar_get_default_day (calendar, week_day);
	day_entry = (NodeEntry *) g_hash_table_lookup (parser->day_hash, day);
	
	if (!day_entry) {
		return;
	}
	
	mpp_xml_set_int (node, name, day_entry->id);
}

static void
mpp_write_overridden_day (MrpParser           *parser,
			  xmlNodePtr           parent,
			  MrpDayWithIntervals *di)
{
	NodeEntry  *entry;
	xmlNodePtr  child;
	GList      *l;

	entry = g_hash_table_lookup (parser->day_hash, di->day);
	if (entry) {
		child = xmlNewChild (parent, NULL, "overridden-day-type", NULL);
		mpp_xml_set_int (child, "id", entry->id);

		for (l = di->intervals; l; l = l->next) {
			mpp_write_interval (child, (MrpInterval *)l->data);
		}
	}

	g_free (di);
}

static void
mpp_write_overridden_date (MrpParser      *parser,
			   xmlNodePtr      parent,
			   MrpDateWithDay *dd)
{
	NodeEntry  *entry;
	xmlNodePtr  child;
	gchar      *str;

	entry = g_hash_table_lookup (parser->day_hash, dd->day);
	if (entry) {
		child = xmlNewChild (parent, NULL, "day", NULL);

		str = mrp_time_format ("%Y%m%d", dd->date);
		xmlSetProp (child, "date", str);
		g_free (str);
		
		xmlSetProp (child, "type", "day-type");
 		mpp_xml_set_int (child, "id", entry->id);
	}

 	g_free (dd);
}

static void
mpp_write_calendar (MrpParser   *parser, 
		    xmlNodePtr   parent, 
		    MrpCalendar *calendar)
{
	xmlNodePtr  node, child;
	GList      *l, *days, *dates;
	gint        id;
	
	g_return_if_fail (MRP_IS_CALENDAR (calendar));

	node = xmlNewChild (parent, NULL, "calendar", NULL);

	id = parser->next_calendar_id++;
	mpp_xml_set_int (node, "id", id);

	g_hash_table_insert (parser->calendar_hash,
			     calendar,
			     GINT_TO_POINTER (id));
	
	xmlSetProp (node, "name", mrp_calendar_get_name (calendar));
	
	/* Write the default week */
	child = xmlNewChild (node, NULL, "default-week", NULL);
	
	mpp_write_default_day (parser, child, calendar, 
			       "mon", MRP_CALENDAR_DAY_MON);
	mpp_write_default_day (parser, child, calendar,
			       "tue", MRP_CALENDAR_DAY_TUE);
	mpp_write_default_day (parser, child, calendar, 
			       "wed", MRP_CALENDAR_DAY_WED);
	mpp_write_default_day (parser, child, calendar,
			       "thu", MRP_CALENDAR_DAY_THU);
	mpp_write_default_day (parser, child, calendar, 
			       "fri", MRP_CALENDAR_DAY_FRI);
	mpp_write_default_day (parser, child, calendar, 
			       "sat", MRP_CALENDAR_DAY_SAT);
	mpp_write_default_day (parser, child, calendar, 
			       "sun", MRP_CALENDAR_DAY_SUN);
	
	/* Override days */
	child = xmlNewChild (node, NULL, "overridden-day-types", NULL);
	days = mrp_calendar_get_overridden_days (calendar);
	
	for (l = days; l; l = l->next) {
		MrpDayWithIntervals *day_ival =l->data;
		
		mpp_write_overridden_day (parser, child,  day_ival);
	}
	g_list_free (days);
	
	/* Write the overriden dates */
	child = xmlNewChild (node, NULL, "days", NULL);
	dates = mrp_calendar_get_all_overridden_dates (calendar);
	for (l = dates; l; l = l->next) {
		MrpDateWithDay *date_day = l->data;
		
		mpp_write_overridden_date (parser, child, date_day);
	}
	g_list_free (dates);

	/* Add special dates */
	for (l = mrp_calendar_get_children (calendar); l; l = l->next) {
		MrpCalendar *child_calendar = l->data;
		
		mpp_write_calendar (parser, node, child_calendar);
	}
}

static gboolean
mpp_write_project (MrpParser *parser)
{
	xmlNodePtr   node, child, calendars_node;
	GList       *list, *l;
	GList       *assignments = NULL;
	MrpGroup    *default_group = NULL;
	NodeEntry   *entry;
	MrpCalendar *root_calendar;
	
	node = xmlNewDocNode (parser->doc, NULL, "project", NULL);
	parser->doc->xmlRootNode = node;

	mpp_write_property_specs (parser, node);
	mpp_write_custom_properties (parser, node, MRP_OBJECT (parser->project));

	mpp_write_phases (parser, node);
	
	/* Write calendars */
	calendars_node = xmlNewChild (node, NULL, "calendars", NULL);
	child = xmlNewChild (calendars_node, NULL, "day-types", NULL);
	
	mpp_write_day (parser, child, mrp_day_get_work ());
	mpp_write_day (parser, child, mrp_day_get_nonwork ());
	mpp_write_day (parser, child, mrp_day_get_use_base ());

	for (l = mrp_day_get_all (parser->project); l; l = l->next) {
		mpp_write_day (parser, child, MRP_DAY (l->data));
	}

	/* Get the calendars */
	root_calendar = mrp_project_get_root_calendar (parser->project);
	
	for (l = mrp_calendar_get_children (root_calendar); l; l = l->next) {
		mpp_write_calendar (parser, calendars_node, l->data);
	}

	/* Write project properties now that we have the calendar id. */
	mpp_write_project_properties (parser, node);

	/* Write tasks. */
 	child = xmlNewChild (node, NULL, "tasks",NULL);

	entry = g_new0 (NodeEntry, 1);
	entry->id = 0;
	entry->node = child;

	g_hash_table_insert (parser->task_hash, parser->root_task, entry);
	
	/* Generate IDs and hash table. */
	parser->last_id = 1;
	mrp_project_task_traverse (parser->project,
				   parser->root_task,
				   (MrpTaskTraverseFunc) mpp_hash_insert_task_cb,
				   parser);
	
	mrp_project_task_traverse (parser->project,
				   parser->root_task,
				   (MrpTaskTraverseFunc) mpp_write_task_cb,
				   parser);

	/* Write resource groups. */
 	child = xmlNewChild (node, NULL, "resource-groups",NULL);
	list = mrp_project_get_groups (parser->project);

	/* Generate IDs and hash table. */
	parser->last_id = 1;
	for (l = list; l; l = l->next) {
		mpp_hash_insert_group (parser, l->data); 
	}

	g_object_get (parser->project, "default-group", &default_group, NULL);

	if (default_group) {
		entry = g_hash_table_lookup (parser->group_hash, 
					     default_group);
		mpp_xml_set_int (child, "default_group", entry->id);
	}
	
	for (l = list; l; l = l->next) {
		mpp_write_group (parser, child, l->data); 
	}
	
	/* Write resources. */
 	child = xmlNewChild (node, NULL, "resources",NULL);
	list = mrp_project_get_resources (parser->project);

	/* Generate IDs and hash table. */
	parser->last_id = 1;
	for (l = list; l; l = l->next) {
		GList *r_list;
		mpp_hash_insert_resource (parser, l->data); 
		r_list = mrp_resource_get_assignments (MRP_RESOURCE (l->data));
		assignments = g_list_concat (assignments, 
					     g_list_copy (r_list));
	}

	for (l = list; l; l = l->next) {
		mpp_write_resource (parser, child, l->data); 
	}

	/* Write assignments. */
 	child = xmlNewChild (node, NULL, "allocations", NULL);

	for (l = assignments; l; l = l->next) {
		mpp_write_assignment (parser, child, l->data);
	}
	g_list_free (assignments);

	return TRUE;
}

static xmlDocPtr
parser_build_xml_doc (MrpStorageMrproject  *module,
		      GError              **error)
{
	MrpParser parser;
	
	g_return_val_if_fail (MRP_IS_STORAGE_MRPROJECT (module), FALSE);

	/* We want indentation. */
	xmlKeepBlanksDefault (0);
	
	memset (&parser, 0, sizeof (parser));

	parser.project = module->project;
	parser.task_hash = g_hash_table_new_full (NULL, NULL, NULL, g_free);
	parser.group_hash = g_hash_table_new_full (NULL, NULL, NULL, g_free);
	parser.resource_hash = g_hash_table_new_full (NULL, NULL, NULL, g_free);
	parser.day_hash = g_hash_table_new (NULL, NULL);
	parser.calendar_hash = g_hash_table_new (NULL, NULL);
	parser.root_task = mrp_project_get_root_task (parser.project);

	parser.next_day_type_id = MRP_DAY_NEXT;
	parser.next_calendar_id = 1;

	parser.doc = xmlNewDoc ("1.0");

	if (!mpp_write_project (&parser)) {
		g_set_error (error,
			     MRP_ERROR,
			     MRP_ERROR_SAVE_WRITE_FAILED,
			     _("Could not create XML tree"));
		xmlFreeDoc (parser.doc);
		parser.doc = NULL;
	}

	g_hash_table_destroy (parser.task_hash);
	g_hash_table_destroy (parser.group_hash);
	g_hash_table_destroy (parser.resource_hash);
	g_hash_table_destroy (parser.day_hash);
	g_hash_table_destroy (parser.calendar_hash);
	
	return parser.doc;
}

gboolean
mrp_parser_save (MrpStorageMrproject  *module,
		 const gchar          *filename,
		 gboolean              force,
		 GError              **error)
{
	gchar     *real_filename;
	gint       ret;
	gboolean   file_exist;
	xmlDocPtr  doc;
	
	g_return_val_if_fail (MRP_IS_STORAGE_MRPROJECT (module), FALSE);
	g_return_val_if_fail (filename != NULL && filename[0] != 0, FALSE);

	if (!strstr (filename, ".mrproject") && !strstr (filename, ".planner")) {
		real_filename = g_strconcat (filename, ".planner", NULL);
	} else {
		real_filename = g_strdup (filename);
	}
	
	file_exist = g_file_test (
		real_filename, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR);
	
	if (file_exist && !force) {
		g_set_error (error,
			     MRP_ERROR,
			     MRP_ERROR_SAVE_FILE_EXIST,
			     "%s", real_filename);
		
		g_free (real_filename);
		return FALSE;
	}

	doc = parser_build_xml_doc (module, error);
	if (!doc) {
		g_free (real_filename);
		return FALSE;
	}

	ret = xmlSaveFormatFile (real_filename, doc, 1);
	g_free (real_filename);
	xmlFreeDoc (doc);

	if (ret == -1) {
		g_set_error (error,
			     MRP_ERROR,
			     MRP_ERROR_SAVE_WRITE_FAILED,
			     _("Could not write XML file"));

		return FALSE;
	}
	
	return TRUE;
}

gboolean
mrp_parser_to_xml (MrpStorageMrproject  *module,
		   gchar               **str,
		   GError              **error)
{
	xmlDocPtr  doc;
	xmlChar   *buf;
	gsize      len;
	
	g_return_val_if_fail (MRP_IS_STORAGE_MRPROJECT (module), FALSE);

	doc = parser_build_xml_doc (module, error);
	if (!doc) {
		return FALSE;
	}

	xmlDocDumpFormatMemory (doc, &buf, &len, 1);
	xmlFreeDoc (doc);

	*str = g_strdup (buf);
	xmlFree (buf);
	
	if (len <= 0) {
		g_set_error (error,
			     MRP_ERROR,
			     MRP_ERROR_SAVE_WRITE_FAILED,
			     _("Could not create XML tree"));
		
		return FALSE;
	}
	
	return TRUE;
}

gboolean
mrp_parser_from_xml (MrpStorageMrproject  *module,
		     const gchar          *str,
		     GError              **error)
{
	g_warning ("mrp_parser_from_xml is unimplemented.") ;

	return FALSE;
}


/***************
 * XML helpers.
 */


/* Search a child by name, if needed go down the tree to find it.
 */
static xmlNodePtr
mpp_xml_search_child (xmlNodePtr node, const gchar *name)
{
	xmlNodePtr ret;
	xmlNodePtr child;

	child = node->children;
	while (child != NULL) {
		if (!strcmp (child->name, name))
			return child;
		child = child->next;
	}
	child = node->children;
	while (child != NULL) {
		ret = mpp_xml_search_child (child, name);
		if (ret != NULL)
			return ret;
		child = child->next;
	}
	return NULL;
}

static void
mpp_xml_set_date (xmlNodePtr node, const gchar *prop, mrptime time)
{
	gchar *str;

	str = mrp_time_to_string (time);
	xmlSetProp (node, prop, str);
	g_free (str);
}

static void
mpp_xml_set_int (xmlNodePtr node, const gchar *prop, gint value)
{
	gchar *str;

	str = g_strdup_printf ("%d", value);
	xmlSetProp (node, prop, str);
	g_free (str);
}

static void
mpp_xml_set_float (xmlNodePtr node, const gchar *prop, gfloat value)
{
	gchar  buf[128];
	gchar *str;

	str = g_ascii_dtostr (buf, sizeof(buf) - 1, value);
	xmlSetProp (node, prop, str);
}

static void
mpp_xml_set_task_type (xmlNodePtr node, const gchar *prop, MrpTaskType type)
{
	gchar *str;

	switch (type) {
	case MRP_TASK_TYPE_MILESTONE:
		str = "milestone";
		break;

	case MRP_TASK_TYPE_NORMAL:
	default:
		str = "normal";
		break;
	}		
	
	xmlSetProp (node, prop, str);
}

static void
mpp_xml_set_task_sched (xmlNodePtr node, const gchar *prop, MrpTaskSched sched)
{
	gchar *str;

	switch (sched) {
	case MRP_TASK_SCHED_FIXED_DURATION:
		str = "fixed-duration";
		break;

	case MRP_TASK_SCHED_FIXED_WORK:
	default:
		str = "fixed-work";
		break;
	}		
	
	xmlSetProp (node, prop, str);
}
