/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Evolution planner - iCalendar planner backend
 *
 * Copyright 2004 (C) Alvaro del Castillo <acs@barrapunto.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * We use the acronyms:
 * - cbp: e_cal_backend_planner
 * - comp: cal_component/component
 */

#include <config.h>
#include <string.h>
#include <libgnome/gnome-i18n.h>
#include <libedataserver/e-uid.h>
#include "e-cal-backend-planner.h"
#include <libplanner/mrp-application.h>
#include <libplanner/mrp-project.h>
#include <libplanner/mrp-task.h>

/* Private part of the ECalBackendPlanner structure */
struct _ECalBackendPlannerPrivate {
	/* uri to get planner data from */
	gchar      *uri;

	MrpProject *project;

	/* Local/remote mode */
	CalMode mode;

	/* The calendar's default timezone, used for resolving DATE and
	   floating DATE-TIME values. */
	icaltimezone *default_zone;

	/* All the tasks in the calendar, hashed by UID.  The
	 * hash key *is* the uid returned by cal_component_get_uid(); it is not
	 * copied, so don't free it when you remove an object from the hash
	 * table. Each item in the hash table is a ECalComponent.
	 */
	GHashTable *tasks_comp;

	gboolean project_loaded;
};

/* Backend implementation */
static void                  cbp_dispose                 (GObject          *object);
static void                  cbp_finalize                (GObject          *object);
static ECalBackendSyncStatus cbp_is_read_only            (ECalBackendSync  *backend,
							  EDataCal         *cal,
							  gboolean         *read_only);
static ECalBackendSyncStatus cbp_get_cal_address         (ECalBackendSync  *backend,
							  EDataCal         *cal,
							  char            **address);
static ECalBackendSyncStatus cbp_get_ldap_attribute      (ECalBackendSync  *backend,
							  EDataCal         *cal,
							  char            **attribute);
static ECalBackendSyncStatus cbp_get_alarm_email_address (ECalBackendSync  *backend,
							  EDataCal         *cal,
							  char            **address);
static ECalBackendSyncStatus cbp_get_static_capabilities (ECalBackendSync  *backend,
							  EDataCal         *cal,
							  char            **capabilities);

static ECalBackendSyncStatus cbp_open                    (ECalBackendSync  *backend,
							  EDataCal         *cal,
							  gboolean          only_if_exists,
							  const gchar      *username,
							  const gchar      *password);
static ECalBackendSyncStatus cbp_remove                  (ECalBackendSync  *backend,
							  EDataCal         *cal);
static gboolean              cbp_is_loaded               (ECalBackend      *backend);
static CalMode               cbp_get_mode                (ECalBackend      *backend);
static void                  cbp_set_mode                (ECalBackend      *backend,
							  CalMode           mode);
static ECalBackendSyncStatus cbp_get_default_object      (ECalBackendSync  *backend,
							  EDataCal         *cal,
							  char            **object);
static ECalBackendSyncStatus cbp_get_object              (ECalBackendSync  *backend,
							  EDataCal         *cal,
							  const gchar      *uid,
							  const gchar      *rid,
							  gchar           **object);
static ECalBackendSyncStatus cbp_get_timezone            (ECalBackendSync  *backend,
							  EDataCal         *cal,
							  const gchar      *tzid,
							  char            **object);
static ECalBackendSyncStatus cbp_add_timezone            (ECalBackendSync  *backend,
							  EDataCal         *cal,
							  const gchar      *tzobj);
static ECalBackendSyncStatus cbp_set_default_timezone    (ECalBackendSync  *backend,
							  EDataCal         *cal,
							  const gchar      *tzid);
static ECalBackendSyncStatus cbp_get_object_list         (ECalBackendSync  *backend,
							  EDataCal         *cal,
							  const gchar      *sexp,
							  GList           **objects);
static void cbp_start_query                              (ECalBackend      *backend,
							  EDataCalView     *query);
static ECalBackendSyncStatus cbp_get_free_busy           (ECalBackendSync  *backend,
							  EDataCal         *cal,
							  GList            *users,
							  time_t            start,
							  time_t            end,
							  GList           **freebusy);
static ECalBackendSyncStatus cbp_get_changes             (ECalBackendSync  *backend,
							  EDataCal         *cal,
							  const char       *change_id,
							  GList           **adds,
							  GList           **modifies,
							  GList           **deletes);
static ECalBackendSyncStatus cbp_discard_alarm           (ECalBackendSync  *backend,
							  EDataCal         *cal,
							  const gchar      *uid,
							  const gchar      *auid);
static ECalBackendSyncStatus cbp_create_object           (ECalBackendSync  *backend,
							  EDataCal         *cal,
							  gchar           **calobj,
							  gchar           **uid);
static ECalBackendSyncStatus cbp_modify_object           (ECalBackendSync  *backend,
							  EDataCal         *cal,
							  const gchar      *calobj,
							  CalObjModType     mod,
							  char            **old_object);
static ECalBackendSyncStatus cbp_remove_object           (ECalBackendSync  *backend,
							  EDataCal         *cal,
							  const gchar      *uid,
							  const gchar      *rid,
							  CalObjModType     mod,
							  gchar           **object);
static ECalBackendSyncStatus cbp_receive_objects         (ECalBackendSync  *backend,
							  EDataCal         *cal,
							  const gchar      *calobj);

static ECalBackendSyncStatus cbp_send_objects            (ECalBackendSync  *backend,
							  EDataCal         *cal,
							  const gchar      *calobj,
							  GList           **users,
							  gchar           **modified_calobj);
static icaltimezone * cbp_internal_get_default_timezone  (ECalBackend      *backend);

static icaltimezone * cbp_internal_get_timezone          (ECalBackend      *backend,
							  const gchar      *tzid);

/* Utilities */
static ECalComponent *task_to_comp                       (MrpTask          *task);
static mrptime        datetime_to_mrptime     (const ECalComponentDateTime *dt);
static gchar *        comp_categories_as_char            (ECalComponent    *comp);
static gchar *        comp_desc_as_char                  (ECalComponent    *comp);
static MrpTask *      comp_to_task                       (MrpProject       *project,
							  ECalComponent    *comp);
static gboolean       get_planner_tasks_cb               (ECalBackendPlanner *cbplanner);
static void           add_comp_to_list                   (gpointer          key,
							  gpointer          value,
							  GList           **comps);
static void            dump_print                        (gpointer          key,
							  gpointer          value,
							  gpointer          user_data);
static void            dump_cache                        (ECalBackendPlanner *backend);
static ECalComponent * lookup_component                  (ECalBackendPlanner *backend,
							  const char       *uid);
static MrpTask *       lookup_task                       (ECalBackendPlanner *backend,
							  const char       *uid);
static void            task_replace                      (MrpProject       *project,
							  MrpTask          *task,
							  MrpTask          *task_new);
static void            task_add_properties               (MrpProject       *project);
static char *          form_uri                          (ESource          *source);



static ECalBackendSyncClass *parent_class;

#define d(x)

/* For debugging cache hash table */
static void
dump_print  (gpointer key,
	     gpointer value,
	     gpointer user_data)
{
	g_message ("Key in hash table: %s (%p), %p\n", (gchar *) key, key, value);
	if (E_IS_CAL_COMPONENT (value)) {
		g_message ("The key as a right component ...");
	}
}

static void
dump_cache (ECalBackendPlanner *backend)
{
	ECalBackendPlannerPrivate *priv;

	priv = backend->priv;
	g_hash_table_foreach (priv->tasks_comp, dump_print, NULL);
}

/* Looks up a component by its UID on the backend's component hash table */
static ECalComponent *
lookup_component (ECalBackendPlanner *backend,
		  const char         *uid)
{
	ECalBackendPlannerPrivate *priv;
	ECalComponent             *comp;

	g_return_val_if_fail (uid != NULL, NULL);

	priv = backend->priv;

	/* dump_cache (backend); */
	g_message ("Searching the uid: %s", uid);
	comp = g_hash_table_lookup (priv->tasks_comp, uid);
	if (comp == NULL) {
		g_warning ("Key not found: %s (%p)\n", uid, uid);
	}
	return comp;
}

/* Looks up a component by its UID on the backend's component hash table */
static MrpTask *
lookup_task (ECalBackendPlanner *backend,
	     const char         *uid)
{
	ECalBackendPlannerPrivate *priv;
	GList                     *tasks, *l;

	priv = backend->priv;
	tasks = mrp_project_get_all_tasks (priv->project);
	for (l = tasks; l; l = l->next) {
		gchar *task_uid;

		mrp_object_get (l->data, "eds-uid", &task_uid, NULL);
		if (!strcmp (uid, task_uid)) {
			g_free (task_uid);
			return l->data;
		}
		g_free (task_uid);
	}
	g_message ("Task not found: %s", uid);
	return NULL;
}

/* Replace a MrpTask in a MrpProject */
static void
task_replace (MrpProject *project,
	      MrpTask    *task,
	      MrpTask    *task_new)
{
	GList *resources, *l;

	g_return_if_fail (MRP_IS_TASK (task));
	g_return_if_fail (MRP_IS_TASK (task_new));

	resources = mrp_task_get_assigned_resources (task);
	for (l = resources; l; l = l->next) {
		mrp_project_remove_resource (project, l->data);
	}
	mrp_project_remove_task (project, task);
	/* g_object_unref (task); */
	mrp_project_insert_task (project, NULL, -1, task_new);
}

/* Create custom properties for tasks in Planner
   so we can save more data from tasks in e-d-s */
static void
task_add_properties (MrpProject *project)
{
	/* We need some custom properties in Planner */
	/* I use EDS and not Evolution Data Server because the text is shown
	   in the Planner task editor columns */
	MrpProperty *property;

	/* eds-uid */
	if (!mrp_project_has_property (project,
				       MRP_TYPE_TASK, "eds-uid")) {
		property = mrp_property_new ("eds-uid",
					     MRP_PROPERTY_TYPE_STRING,
					     _("EDS UID"),
					     _("Identifier used by Evolution Data Server for tasks"),
					     FALSE);
		mrp_project_add_property (project, MRP_TYPE_TASK, property, FALSE);
	}
	/* Categories */
	if (!mrp_project_has_property (project,
				       MRP_TYPE_TASK, "eds-categories")) {
		property = mrp_property_new ("eds-categories",
					     MRP_PROPERTY_TYPE_STRING,
					     _("EDS Categories"),
					     _("Categories for a task used by Evolution Data Server"),
					     FALSE);
		mrp_project_add_property (project, MRP_TYPE_TASK, property, FALSE);
	}
	/* Classification */
	if (!mrp_project_has_property (project,
				       MRP_TYPE_TASK, "eds-classification")) {
		property = mrp_property_new ("eds-classification",
					     MRP_PROPERTY_TYPE_INT,
					     _("EDS Classification"),
					     _("Task access classification used by Evolution Data Server"),
					     FALSE);
		mrp_project_add_property (project, MRP_TYPE_TASK, property, FALSE);
	}
	/* Web page */
	if (!mrp_project_has_property (project,
				       MRP_TYPE_TASK, "eds-url")) {
		property = mrp_property_new ("eds-url",
					     MRP_PROPERTY_TYPE_STRING,
					     _("EDS URL"),
					     _("URL for a Task used by Evolution Data Server"),
					     FALSE);
		mrp_project_add_property (project, MRP_TYPE_TASK, property, FALSE);
	}
}



/* Dispose handler for the file backend */
static void
cbp_dispose (GObject *object)
{
	ECalBackendPlanner        *cbplanner;
	ECalBackendPlannerPrivate *priv;

	cbplanner = E_CAL_BACKEND_PLANNER (object);
	priv = cbplanner->priv;

	if (G_OBJECT_CLASS (parent_class)->dispose)
		(* G_OBJECT_CLASS (parent_class)->dispose) (object);
}

/* Finalize handler for the file backend */
static void
cbp_finalize (GObject *object)
{
	ECalBackendPlanner        *cbplanner;
	ECalBackendPlannerPrivate *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (E_IS_CAL_BACKEND_PLANNER (object));

	cbplanner = E_CAL_BACKEND_PLANNER (object);
	priv = cbplanner->priv;

	/* Clean up */

	if (priv->uri) {
	        g_free (priv->uri);
		priv->uri = NULL;
	}

	if (priv->tasks_comp) {
		g_hash_table_destroy (priv->tasks_comp);
	}

	g_free (priv);
	cbplanner->priv = NULL;

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

/* Calendar backend methods */

/* Is_read_only handler for the file backend */
/* It will be read and write in the future */
static ECalBackendSyncStatus
cbp_is_read_only (ECalBackendSync *backend,
				    EDataCal        *cal,
				    gboolean        *read_only)
{
	*read_only = FALSE;

	return GNOME_Evolution_Calendar_Success;
}

/* FIXME: The only address for the Planner backend is the file
   for the moment. Later we will have a URI */
static ECalBackendSyncStatus
cbp_get_cal_address (ECalBackendSync *backend,
				       EDataCal        *cal,
				       char           **address)
{
	*address = NULL;

	return GNOME_Evolution_Calendar_Success;
}

static ECalBackendSyncStatus
cbp_get_ldap_attribute (ECalBackendSync *backend,
					  EDataCal        *cal,
					  char           **attribute)
{
	*attribute = NULL;

	return GNOME_Evolution_Calendar_Success;
}

static ECalBackendSyncStatus
cbp_get_alarm_email_address (ECalBackendSync *backend,
					       EDataCal        *cal,
					       char           **address)
{
	*address = NULL;

	return GNOME_Evolution_Calendar_Success;
}

static ECalBackendSyncStatus
cbp_get_static_capabilities (ECalBackendSync *backend,
					       EDataCal        *cal,
					       char           **capabilities)
{
	*capabilities = g_strdup (CAL_STATIC_CAPABILITY_NO_EMAIL_ALARMS);

	return GNOME_Evolution_Calendar_Success;
}

static ECalComponent *
task_to_comp (MrpTask *task)
{
	ECalComponent         *comp;
	ECalComponentText      text;
	GList                 *resources = NULL, *l = NULL;
	GSList                *attendee_list = NULL;
	ECalComponentDateTime  dt_start, dt_end;
	struct icaltimetype    istart,   iend;
	/* FIXME: mrptime is like a time_t: check it! */
	mrptime                start,      end;
	gchar                 *uid;
	gchar                 *name;
	gchar                 *note;
	gchar                 *url;
	gchar                 *categories;
	gint                   complete;
	gint                   priority;
	gint                   classification;

	comp = e_cal_component_new ();
	e_cal_component_set_new_vtype (comp, E_CAL_COMPONENT_TODO);

	mrp_object_get (task,
			"eds-uid", &uid,
			"name", &name,
			"note", &note,
			"eds-url", &url,
			"eds-categories", &categories,
			"start", &start,
			"finish", &end,
			"percent-complete", &complete,
			"priority", &priority,
			"eds-classification", &classification,
			NULL);

	istart = icaltime_from_timet (start, 1);
	/* g_message ("Task start: %s", icaltime_as_ical_string (istart)); */
	dt_start.value = &istart;
	dt_start.tzid = NULL;
	iend = icaltime_from_timet (end, 1);
	/* g_message ("Task end: %s", icaltime_as_ical_string (iend)); */
	dt_end.value = &iend;
	dt_end.tzid = NULL;
	if (!uid) {
		uid = e_uid_new ();
		mrp_object_set (task, "eds-uid", uid, NULL);
	}
	g_message ("New task found: %s %s", uid, name);
	e_cal_component_set_uid (comp, uid);
	e_cal_component_set_dtstart (comp, &dt_start);
	e_cal_component_set_dtend (comp, &dt_end);
	text.value = name;
	text.altrep = NULL;
	e_cal_component_set_summary (comp, &text);

	/* url */
	e_cal_component_set_url (comp, url);
	g_free (url);

	/* description */
	if (note) {
		GSList            l;
		ECalComponentText text_dsc;

		text_dsc.value = note;
		text_dsc.altrep = NULL;
		l.data = &text_dsc;
		l.next = NULL;

		e_cal_component_set_description_list (comp, &l);
	}

	/* classification: we don't have it in Planner */
	/* e_cal_component_set_classification (comp, E_CAL_COMPONENT_CLASS_PUBLIC); */

	/* resources */
	resources = mrp_task_get_assigned_resources (task);
	for (l = resources; l; l = l->next) {
		MrpResource           *resource;
		const gchar           *name;
		gchar                 *email;
		ECalComponentAttendee *attendee;

		attendee = g_new0 (ECalComponentAttendee, 1);
		resource = l->data;
		mrp_object_get (resource, "email", &email, NULL);
		name = mrp_resource_get_name (resource);
		g_message ("New resource: %s", name);
		attendee->cn = g_strdup (name);
		attendee->value = g_strconcat("MAILTO:", email, NULL);
		attendee->cutype = ICAL_CUTYPE_INDIVIDUAL;
		attendee->role = ICAL_ROLE_REQPARTICIPANT;
		/* In Planner we can't be sure a resource has accepted */
		attendee->status = ICAL_PARTSTAT_ACCEPTED;

		attendee_list = g_slist_append (attendee_list, attendee);
	}
	e_cal_component_set_attendee_list (comp, attendee_list);

	/* priority */
	/* FIXME: In planner we use a open field. In evo, only from 0-9
	   and it is translated to a string */
	e_cal_component_set_priority (comp, &priority);

	/* complete percent */
	e_cal_component_set_percent (comp, &complete);

	/* categories */
	if (categories) {
		gchar **categories_array;
		GSList  *categories_list = NULL;
		guint    i;

		g_message ("Categories: %s", categories);
		categories_array = g_strsplit (categories, ",", -1);
		g_message ("Category 0: %s", categories_array[0]);
		for (i = 0; categories_array[i]; i++) {
			categories_list = g_slist_append (categories_list,
							  g_strdup (categories_array[i]));
		}
		e_cal_component_set_categories_list (comp, categories_list);
		g_strfreev(categories_array);
	}

	/* classification */
	e_cal_component_set_classification (comp, classification);

	/* FIXME: we need to do commit? Yes */
        e_cal_component_commit_sequence (comp);

	return comp;
}

/* FIXME: timezones */
static mrptime
datetime_to_mrptime (const ECalComponentDateTime *dt)
{
	g_return_val_if_fail (dt != NULL, 0);
	g_return_val_if_fail (dt->value != NULL, 0);

	return icaltime_as_timet (*(*dt).value);
}

/*
static * ECalComponentDateTime
mrptime_to_datetime (const mrptime date)
{
	// FIXME: will be destroyed exiting function ...
	struct icaltimetype    idate;
	ECalComponentDateTime *dt;

	idate = icaltime_from_timet (date, 1);
	dt = g_new0 (ECalComponentDateTime, 1);
	dt->value = &idate;
	dt->tzid = NULL;
} */

static gchar *
comp_categories_as_char (ECalComponent *comp)
{
	GSList *slist_cat, *sl;
	gchar  *cat = NULL;

	e_cal_component_get_categories_list (comp, &slist_cat);
	if (slist_cat) {
		GString *str = g_string_new ("");

		for (sl = slist_cat; sl != NULL; sl = sl->next) {
			str = g_string_append (str, sl->data);
			str = g_string_append (str, ",");
		}

		cat = g_strdup (str->str);
		g_string_free (str, TRUE);
		e_cal_component_free_text_list (slist_cat);
	}
	return cat;
}

static gchar *
comp_desc_as_char (ECalComponent *comp)
{
	GSList *slist_desc, *sl;
	gchar  *desc = NULL;

	e_cal_component_get_description_list (comp, &slist_desc);
	if (slist_desc) {
		GString *str = g_string_new ("");

		for (sl = slist_desc; sl != NULL; sl = sl->next) {
			ECalComponentText *pt = sl->data;

			if (pt && pt->value)
				str = g_string_append (str, pt->value);
		}

		desc = g_strdup (str->str);
		g_string_free (str, TRUE);
		e_cal_component_free_text_list (slist_desc);
	}
	return desc;
}

static MrpTask *
comp_to_task (MrpProject    *project,
	      ECalComponent *comp)
{
	/* Task data to be filled */
	MrpTask                     *task;
	mrptime                      start, end;
	/* ECalComp data */
	ECalComponentText            summary;
	ECalComponentDateTime        dt_start, dt_end;
	ECalComponentClassification  classification;
	gint                        *priority;
	gint                        *complete;
	gchar                       *note;
	gchar                       *categories;
	const gchar                 *url;
	const gchar                 *uid;

	g_return_val_if_fail (MRP_IS_PROJECT (project), NULL);

	task = mrp_task_new ();

	mrp_project_insert_task (project, NULL, -1, task);
	e_cal_component_get_uid (comp, &uid);
	g_message ("UID for new task: %s", uid);
	e_cal_component_get_summary (comp, &summary);
	g_message ("Summary: %s", summary.value);
	e_cal_component_get_url (comp, &url);
	g_message ("URL: %s", url);
	e_cal_component_get_priority (comp, &priority);
	e_cal_component_get_percent (comp, &complete);
	e_cal_component_get_classification (comp, &classification);
	e_cal_component_get_dtstart (comp, &dt_start);
	start = datetime_to_mrptime (&dt_start);
	e_cal_component_get_dtend (comp, &dt_end);
	end = datetime_to_mrptime (&dt_end);
	note = comp_desc_as_char (comp);
	categories = comp_categories_as_char (comp);

	mrp_object_set (task,
			"eds-uid", uid,
			"name", summary.value ? summary.value : "",
			"eds-url", url ? url : "",
			"note", note ? note : "",
			"eds-categories", categories ? categories : "",
			"eds-classification", classification,
			/* "start", &start,
			   "finish", &end, */
			NULL);
	if (!priority) {
		mrp_object_set (task, "priority", 0, NULL);
	} else {
		mrp_object_set (task, "priority", *priority, NULL);

	}
	if (!complete) {
		mrp_object_set (task,"percent-complete", 0, NULL);
	} else {
		mrp_object_set (task,"percent-complete", *complete, NULL);
	}
	/* Time to save the resources */
	if (e_cal_component_has_attendees (comp)) {
		GSList *attendee_list, *al;
		GError *error = NULL;
		GList  *resources = NULL, *l = NULL;

		/* We remove all the old resources: we are loosing here the data
		   that Planner can store and evo can't
		 */
		resources = mrp_task_get_assigned_resources (task);
		for (l = resources; l; l = l->next) {
			g_message ("Removing resource: %s", mrp_resource_get_name (l->data));
			mrp_project_remove_resource (project, l->data);
			g_object_unref (l->data);
		}

		g_message ("The comp has attendee");
		e_cal_component_get_attendee_list (comp, &attendee_list);

		for (al = attendee_list; al != NULL; al = al->next) {
			MrpResource           *resource;
			ECalComponentAttendee *attendee;
			gchar                 *email;

			attendee = (ECalComponentAttendee *) al->data;
			resource = mrp_resource_new ();

			/* Clean MAILTO: */
			email = g_strdup (attendee->value + 7);
			if (attendee->cn != NULL) {
				mrp_resource_set_name (resource, attendee->cn);
			}
			if (attendee->cutype == ICAL_CUTYPE_ROOM ||
			    attendee->cutype == ICAL_CUTYPE_RESOURCE) {
				mrp_object_set (resource, "type",
						MRP_RESOURCE_TYPE_MATERIAL, NULL);
			} else {
				mrp_object_set (resource, "type",
						MRP_RESOURCE_TYPE_WORK, NULL);
			}

			if (email) {
				mrp_object_set (resource, "email", email, NULL);
			}

			g_message ("Adding new resource ...");
			mrp_project_add_resource (project, resource);
			mrp_resource_assign (resource, task, 100);
		}
		mrp_project_save (project, TRUE, &error);
	}
	return task;
}


static gboolean
get_planner_tasks_cb (ECalBackendPlanner *cbplanner)
{
	ECalBackendPlannerPrivate *priv;
	icalcomponent_kind         kind;
	GList                     *tasks, *l;

	priv = cbplanner->priv;

	d(g_message ("Getting planner tasks ...\n"));
	g_message ("Getting planner tasks ...");

	tasks = mrp_project_get_all_tasks (priv->project);

	kind = e_cal_backend_get_kind (E_CAL_BACKEND (cbplanner));

	for (l = tasks; l; l = l->next) {
		ECalComponent *comp;
		const gchar   *uid;

		comp = task_to_comp (l->data);
		e_cal_component_get_uid (comp, &uid);
		g_hash_table_insert (priv->tasks_comp, (gpointer) uid, comp);
	}

	d(g_message ("Planner task retrieval done.\n"));

	return FALSE;
}

static char*
form_uri (ESource *source)
{
	guint  len;
	gchar *uri;
	gchar *formed_uri;

	uri = e_source_get_uri (source);
	if (uri == NULL) {
		return NULL;
	}

	g_message ("URI to open: %s", uri);

	/* Remove planer:// */
	len = strlen (uri);
	if (len > 10) {
		/* Naively strip method. */
		formed_uri = g_strdup (uri + 10);
	} else {
		return NULL;
	}
	g_message ("Parsed URI to open: %s", formed_uri);

	g_free (uri);
	return formed_uri;

}

/* Open handler for the file backend */
static ECalBackendSyncStatus
cbp_open (ECalBackendSync *backend,
	  EDataCal        *cal,
	  gboolean         only_if_exists,
	  const gchar     *username,
	  const gchar     *password)
{
	ECalBackendPlanner        *cbplanner;
	ECalBackendPlannerPrivate *priv;
	ESource                   *source;
	GError                    *error = NULL;
	gchar                     *uri = NULL;

	g_message ("Open planner tasks ...");


	source = e_cal_backend_get_source (E_CAL_BACKEND (backend));
	if (source) {
		uri = form_uri (source);
	}
	if (!uri) {
		e_cal_backend_notify_error (E_CAL_BACKEND (backend), _("Invalid server URI"));
		return GNOME_Evolution_Calendar_NoSuchCal;
	}

	cbplanner = E_CAL_BACKEND_PLANNER (backend);
	priv = cbplanner->priv;

	if (priv->project_loaded) {
		g_warning ("Reopening project ... we need to check for new tasks ...");
		return GNOME_Evolution_Calendar_Success;
	} else {
		g_warning ("Openinig project for the first time ...");
	}

	priv->uri = uri;
	g_message ("Opening planner file: %s", priv->uri);

	priv->mode = CAL_MODE_LOCAL;
	if (mrp_project_load (priv->project, priv->uri, &error)) {
		task_add_properties (priv->project);
		g_idle_add ((GSourceFunc) get_planner_tasks_cb, cbplanner);
		priv->project_loaded = TRUE;
		return GNOME_Evolution_Calendar_Success;
	} else {
		g_warning ("Problem opening planner project: %s", error->message);
		g_error_free (error);
		return GNOME_Evolution_Calendar_ObjectNotFound;
	}
}

static ECalBackendSyncStatus
cbp_remove (ECalBackendSync *backend,
			      EDataCal        *cal)
{
	ECalBackendPlanner        *cbplanner;
	ECalBackendPlannerPrivate *priv;

	cbplanner = E_CAL_BACKEND_PLANNER (backend);
	priv = cbplanner->priv;

	/* e_file_cache_remove (E_FILE_CACHE (priv->cache)); */
	/* FIXME: we need to remove the task from Planner */
	return GNOME_Evolution_Calendar_Success;
}

/* is_loaded handler for the planner backend */
static gboolean
cbp_is_loaded (ECalBackend *backend)
{
	ECalBackendPlanner        *cbplanner;
	ECalBackendPlannerPrivate *priv;

	cbplanner = E_CAL_BACKEND_PLANNER (backend);
	priv = cbplanner->priv;

	return TRUE;
}

/* is_remote handler for the planner backend */
static CalMode
cbp_get_mode (ECalBackend *backend)
{
	ECalBackendPlanner        *cbplanner;
	ECalBackendPlannerPrivate *priv;

	cbplanner = E_CAL_BACKEND_PLANNER (backend);
	priv = cbplanner->priv;

	return priv->mode;
}

/* Set_mode handler for the planner backend */
static void
cbp_set_mode (ECalBackend *backend,
				CalMode      mode)
{
	ECalBackendPlanner               *cbplanner;
	ECalBackendPlannerPrivate        *priv;
	GNOME_Evolution_Calendar_CalMode  set_mode;

	cbplanner = E_CAL_BACKEND_PLANNER (backend);
	priv = cbplanner->priv;

	switch (mode) {
		case CAL_MODE_LOCAL:
		case CAL_MODE_REMOTE:
			priv->mode = mode;
			set_mode = cal_mode_to_corba (mode);
			break;
		case CAL_MODE_ANY:
			priv->mode = CAL_MODE_REMOTE;
			set_mode = GNOME_Evolution_Calendar_MODE_REMOTE;
			break;
		default:
			set_mode = GNOME_Evolution_Calendar_MODE_ANY;
			break;
	}

	if (set_mode == GNOME_Evolution_Calendar_MODE_ANY)
		e_cal_backend_notify_mode (backend,
					   GNOME_Evolution_Calendar_CalListener_MODE_NOT_SUPPORTED,
					   cal_mode_to_corba (priv->mode));
	else
		e_cal_backend_notify_mode (backend,
					   GNOME_Evolution_Calendar_CalListener_MODE_SET,
					   set_mode);
}

static ECalBackendSyncStatus
cbp_get_default_object (ECalBackendSync *backend,
					  EDataCal        *cal,
					  char           **object)
{
	ECalBackendPlanner        *cbplanner;
	ECalBackendPlannerPrivate *priv;
	icalcomponent             *icalcomp;
	icalcomponent_kind         kind;

	cbplanner = E_CAL_BACKEND_PLANNER (backend);
	priv = cbplanner->priv;

	kind = e_cal_backend_get_kind (E_CAL_BACKEND (backend));
	icalcomp = e_cal_util_new_component (kind);
	*object = g_strdup (icalcomponent_as_ical_string (icalcomp));
	icalcomponent_free (icalcomp);

	return GNOME_Evolution_Calendar_Success;
}

/* Get_object_component handler for the planner backend */
static ECalBackendSyncStatus
cbp_get_object (ECalBackendSync *backend,
				  EDataCal        *cal,
				  const gchar     *uid,
				  const gchar     *rid,
				  gchar          **object)
{
	ECalBackendPlanner        *cbplanner;
	ECalBackendPlannerPrivate *priv;
	ECalComponent             *comp = NULL;

	cbplanner = E_CAL_BACKEND_PLANNER (backend);
	priv = cbplanner->priv;

	g_return_val_if_fail (uid != NULL, GNOME_Evolution_Calendar_ObjectNotFound);

	comp = g_hash_table_lookup (priv->tasks_comp, uid);
	/* comp = e_cal_backend_cache_get_component (priv->cache, uid, rid); */
	/* We take the comp directly from libplanner */
	if (!comp)
		return GNOME_Evolution_Calendar_ObjectNotFound;

	*object = e_cal_component_get_as_string (comp);

	return GNOME_Evolution_Calendar_Success;
}

/* Get_timezone_object handler for the file backend */
static ECalBackendSyncStatus
cbp_get_timezone (ECalBackendSync *backend,
				    EDataCal        *cal,
				    const gchar     *tzid,
				    char           **object)
{
	ECalBackendPlanner *cbplanner;
	ECalBackendPlannerPrivate *priv;
	icaltimezone *zone;
	icalcomponent *icalcomp;

	cbplanner = E_CAL_BACKEND_PLANNER (backend);
	priv = cbplanner->priv;

	g_return_val_if_fail (tzid != NULL, GNOME_Evolution_Calendar_ObjectNotFound);

	zone = icaltimezone_get_builtin_timezone_from_tzid (tzid);
	if (!zone)
		return GNOME_Evolution_Calendar_ObjectNotFound;

	icalcomp = icaltimezone_get_component (zone);
	if (!icalcomp)
		return GNOME_Evolution_Calendar_InvalidObject;

	*object = g_strdup (icalcomponent_as_ical_string (icalcomp));

	return GNOME_Evolution_Calendar_Success;
}

/* Add_timezone handler for the file backend */
static ECalBackendSyncStatus
cbp_add_timezone (ECalBackendSync *backend,
				    EDataCal        *cal,
				    const gchar     *tzobj)
{
	ECalBackendPlanner *cbplanner;
	ECalBackendPlannerPrivate *priv;

	cbplanner = (ECalBackendPlanner *) backend;

	g_return_val_if_fail (E_IS_CAL_BACKEND_PLANNER (cbplanner),
			      GNOME_Evolution_Calendar_OtherError);
	g_return_val_if_fail (tzobj != NULL, GNOME_Evolution_Calendar_OtherError);

	priv = cbplanner->priv;

	/* FIXME: add the timezone to the cache */
	return GNOME_Evolution_Calendar_Success;
}

static ECalBackendSyncStatus
cbp_set_default_timezone (ECalBackendSync *backend,
					    EDataCal        *cal,
					    const gchar     *tzid)
{
	ECalBackendPlanner *cbplanner;
	ECalBackendPlannerPrivate *priv;

	cbplanner = E_CAL_BACKEND_PLANNER (backend);
	priv = cbplanner->priv;

	/* FIXME */
	return GNOME_Evolution_Calendar_Success;
}

static void
add_comp_to_list (gpointer   key,
		  gpointer   value,
		  GList    **comps)
{
	g_message ("Adding component to list ...");
	*comps = g_list_append (*comps, value);
}



/* Get_objects_in_range handler for the planner backend */
static ECalBackendSyncStatus
cbp_get_object_list (ECalBackendSync *backend,
				       EDataCal        *cal,
				       const gchar     *sexp,
				       GList          **objects)
{
	ECalBackendPlanner         *cbplanner;
	ECalBackendPlannerPrivate  *priv;
	GList                      *comps = NULL, *l = NULL;
	ECalBackendSExp            *cbsexp;

	cbplanner = E_CAL_BACKEND_PLANNER (backend);
	priv = cbplanner->priv;

	g_message ("Getting the object list ...");

	cbsexp = e_cal_backend_sexp_new (sexp);

	*objects = NULL;
	g_hash_table_foreach (priv->tasks_comp, (GHFunc) add_comp_to_list, &comps);
	for (l = comps; l; l = l->next) {
		if (e_cal_backend_sexp_match_comp (cbsexp,
						   E_CAL_COMPONENT (l->data),
						   E_CAL_BACKEND (backend))) {
			*objects = g_list_append (*objects,
						  e_cal_component_get_as_string (l->data));
		}
	}
	g_list_free (comps);
	g_object_unref (cbsexp);

	return GNOME_Evolution_Calendar_Success;
}

/* get_query handler for the planner backend */
static void
cbp_start_query (ECalBackend  *backend,
				   EDataCalView *query)
{
	ECalBackendPlanner         *cbplanner;
	ECalBackendPlannerPrivate  *priv;
	GList                      *comps = NULL, *l, *objects = NULL;
	ECalBackendSExp            *cbsexp;

	cbplanner = E_CAL_BACKEND_PLANNER (backend);
	priv = cbplanner->priv;

	d(g_message (G_STRLOC ": Starting query (%s)", e_data_cal_view_get_text (query)));
	g_message ("Doing query: %s", e_data_cal_view_get_text (query));

	/* process all components in the cache */
	cbsexp = e_cal_backend_sexp_new (e_data_cal_view_get_text (query));

	objects = NULL;
	/* FIXME: get components from Planner */
	g_hash_table_foreach (priv->tasks_comp, (GHFunc) add_comp_to_list, &comps);
	for (l = comps; l; l = l->next) {
		if (e_cal_backend_sexp_match_comp (cbsexp,
						   E_CAL_COMPONENT (l->data),
						   E_CAL_BACKEND (backend))) {
			objects = g_list_append (objects,
						 e_cal_component_get_as_string (l->data));
		}
	}

	e_data_cal_view_notify_objects_added (query, (const GList *) objects);

	g_list_free (comps);
	g_list_foreach (objects, (GFunc) g_free, NULL);
	g_list_free (objects);
	g_object_unref (cbsexp);

	e_data_cal_view_notify_done (query, GNOME_Evolution_Calendar_Success);
}

/* Get_free_busy handler for the planner backend */
static ECalBackendSyncStatus
cbp_get_free_busy (ECalBackendSync *backend,
				     EDataCal        *cal,
				     GList           *users,
				     time_t           start,
				     time_t           end,
				     GList          **freebusy)
{
	ECalBackendPlanner        *cbplanner;
	ECalBackendPlannerPrivate *priv;

	cbplanner = E_CAL_BACKEND_PLANNER (backend);
	priv = cbplanner->priv;

	g_return_val_if_fail (start != -1 && end != -1,
			      GNOME_Evolution_Calendar_InvalidRange);
	g_return_val_if_fail (start <= end, GNOME_Evolution_Calendar_InvalidRange);

	/* FIXME */
	return GNOME_Evolution_Calendar_Success;
}

/* Get_changes handler for the planner backend */
static ECalBackendSyncStatus
cbp_get_changes (ECalBackendSync *backend,
				   EDataCal        *cal,
				   const char      *change_id,
				   GList          **adds,
				   GList          **modifies,
				   GList          **deletes)
{
	ECalBackendPlanner        *cbplanner;
	ECalBackendPlannerPrivate *priv;

	cbplanner = E_CAL_BACKEND_PLANNER (backend);
	priv = cbplanner->priv;

	g_return_val_if_fail (change_id != NULL, GNOME_Evolution_Calendar_ObjectNotFound);

	/* FIXME */
	return GNOME_Evolution_Calendar_Success;
}

/* Discard_alarm handler for the planner backend */
static ECalBackendSyncStatus
cbp_discard_alarm (ECalBackendSync *backend,
				     EDataCal        *cal,
				     const gchar     *uid,
				     const gchar     *auid)
{
	ECalBackendPlanner        *cbplanner;
	ECalBackendPlannerPrivate *priv;

	cbplanner = E_CAL_BACKEND_PLANNER (backend);
	priv = cbplanner->priv;

	/* FIXME */
	return GNOME_Evolution_Calendar_Success;
}

static ECalBackendSyncStatus
cbp_create_object (ECalBackendSync *backend,
				     EDataCal        *cal,
				     gchar          **calobj,
				     gchar          **uid)
{
	ECalBackendPlanner        *cbplanner;
	ECalBackendPlannerPrivate *priv;
	icalcomponent             *icalcomp;
	ECalComponent             *comp;
	const gchar               *comp_uid;
	struct icaltimetype        current;
	MrpTask                   *task;
	GError                    *error = NULL;

	cbplanner = E_CAL_BACKEND_PLANNER (backend);
	priv = cbplanner->priv;

	g_return_val_if_fail (E_IS_CAL_BACKEND_PLANNER (backend), GNOME_Evolution_Calendar_InvalidObject);
	g_return_val_if_fail (*calobj != NULL, GNOME_Evolution_Calendar_ObjectNotFound);

	icalcomp = icalparser_parse_string (*calobj);
	if (!icalcomp)
		return GNOME_Evolution_Calendar_InvalidObject;

	/* Check kind with the parent */
	if (icalcomponent_isa (icalcomp) != e_cal_backend_get_kind (E_CAL_BACKEND (backend))) {
		icalcomponent_free (icalcomp);
		return GNOME_Evolution_Calendar_InvalidObject;
	}

	/* Get the UID to try to find the object */
	comp_uid = icalcomponent_get_uid (icalcomp);
	if (!comp_uid) {
		char *new_uid;

		new_uid = e_cal_component_gen_uid ();
		if (!new_uid) {
			icalcomponent_free (icalcomp);
			return GNOME_Evolution_Calendar_InvalidObject;
		}

		icalcomponent_set_uid (icalcomp, new_uid);
		comp_uid = icalcomponent_get_uid (icalcomp);

		g_free (new_uid);
	}

	g_message ("Creating a new object %s", comp_uid);


	/* check the object is not in our cache */
	if (lookup_component (cbplanner, comp_uid)) {
		icalcomponent_free (icalcomp);
		return GNOME_Evolution_Calendar_ObjectIdAlreadyExists;
	}

	/* Create the cal component */
	comp = e_cal_component_new ();
	e_cal_component_set_icalcomponent (comp, icalcomp);

	/* Set the created and last modified times on the component */
	current = icaltime_from_timet (time (NULL), 0);
	e_cal_component_set_created (comp, &current);
	e_cal_component_set_last_modified (comp, &current);

	/* Add the object */
	g_hash_table_insert (priv->tasks_comp, (gpointer) comp_uid, comp);

	/* Add the object to Planner project */
	task = comp_to_task (priv->project, comp);

	/* Save the planner project */
	mrp_project_save (priv->project, TRUE, &error);

	return GNOME_Evolution_Calendar_Success;
	/* return GNOME_Evolution_Calendar_PermissionDenied; */
}

/* FIXME: not finished.
   This method is called for the moment when:
   - You check the done checkbox
 */
static ECalBackendSyncStatus
cbp_modify_object (ECalBackendSync *backend,
				     EDataCal        *cal,
				     const gchar     *calobj,
				     CalObjModType    mod,
				     char           **old_object)
{
	ECalBackendPlanner        *cbplanner;
	ECalBackendPlannerPrivate *priv;
	icalcomponent             *icalcomp;
	ECalComponent             *comp;
	ECalComponent             *cache_comp;
	const gchar               *comp_uid;
	MrpTask                   *task, *task_new;
	GError                    *error = NULL;

	*old_object = NULL;
	cbplanner = E_CAL_BACKEND_PLANNER (backend);
	priv = cbplanner->priv;

	g_return_val_if_fail (E_IS_CAL_BACKEND_PLANNER (cbplanner),
			      GNOME_Evolution_Calendar_InvalidObject);
	g_return_val_if_fail (calobj != NULL, GNOME_Evolution_Calendar_ObjectNotFound);

	/* check the component for validity */
	icalcomp = icalparser_parse_string (calobj);
	if (!icalcomp)
		return GNOME_Evolution_Calendar_InvalidObject;

	comp_uid = icalcomponent_get_uid (icalcomp);
	g_message ("Modifying calendar object %s\n%s", comp_uid, calobj);

	dump_cache (cbplanner);

	/* check if the object exists */
	if (!(cache_comp = lookup_component (cbplanner, comp_uid))) {
		g_message ("CRITICAL : Could not find the object in cache %s", comp_uid);
		icalcomponent_free (icalcomp);
		return GNOME_Evolution_Calendar_ObjectNotFound;
	} else {
		g_message ("Cache object found %s... modifying it", comp_uid);
	}

	comp = e_cal_component_new ();
	e_cal_component_set_icalcomponent (comp, icalcomp);

	task = lookup_task (cbplanner, comp_uid);
	if (task == NULL) {
		g_message ("Cache fail: can't find task for component %s", comp_uid);
		g_object_unref (cache_comp);
		return GNOME_Evolution_Calendar_InvalidObject;
	}
	task_new = comp_to_task (priv->project, comp);
	if (task_new == NULL) {
		g_message ("Can't create a task from %s", comp_uid);
		g_object_unref (cache_comp);
		return GNOME_Evolution_Calendar_InvalidObject;
	}
	g_hash_table_replace (priv->tasks_comp, (gpointer) comp_uid, comp);
	task_replace (priv->project, task, task_new);
	mrp_project_save (priv->project, TRUE, &error);

	/* Inform evolution about the object removed */
	if (!E_IS_CAL_COMPONENT (cache_comp)) {
		g_warning ("we don't have the old component ...");
	}

	*old_object = e_cal_component_get_as_string (cache_comp);
	g_object_unref (cache_comp);

	/* return GNOME_Evolution_Calendar_PermissionDenied; */
	return GNOME_Evolution_Calendar_Success;
}

/* Remove_object handler for the planner backend */
static ECalBackendSyncStatus
cbp_remove_object (ECalBackendSync *backend,
				     EDataCal        *cal,
				     const gchar     *uid,
				     const gchar     *rid,
				     CalObjModType    mod,
				     gchar          **object)
{
	ECalBackendPlanner        *cbplanner;
	ECalBackendPlannerPrivate *priv;
	MrpTask                   *task;
	ECalComponent             *comp;
	GError                    *error = NULL;

	cbplanner = E_CAL_BACKEND_PLANNER (backend);
	priv = cbplanner->priv;

	g_return_val_if_fail (uid != NULL, GNOME_Evolution_Calendar_ObjectNotFound);

	g_message ("Removing object %s ...", uid);
	comp = lookup_component (cbplanner, uid);
	g_hash_table_remove (priv->tasks_comp, (gpointer) uid);
	g_object_unref (comp);
	task = lookup_task (cbplanner, uid);
	mrp_project_remove_task (priv->project, task);
	g_object_unref (task);
	mrp_project_save (priv->project, TRUE, &error);

	return GNOME_Evolution_Calendar_Success;;
}

/* Update_objects handler for the planner backend. */
static ECalBackendSyncStatus
cbp_receive_objects (ECalBackendSync *backend,
				       EDataCal        *cal,
				       const gchar     *calobj)
{
	ECalBackendPlanner        *cbplanner;
	ECalBackendPlannerPrivate *priv;
	icalcomponent             *icalcomp;
	/* icalcomponent             *subcomp; */

	cbplanner = E_CAL_BACKEND_PLANNER (backend);
	priv = cbplanner->priv;

	g_return_val_if_fail (calobj != NULL, GNOME_Evolution_Calendar_InvalidObject);

	icalcomp = icalparser_parse_string (calobj);
	if (!icalcomp)
		return GNOME_Evolution_Calendar_InvalidObject;

	g_message ("Modifying object: %s", calobj);


	/* return GNOME_Evolution_Calendar_PermissionDenied;*/
	return GNOME_Evolution_Calendar_Success;

}

static ECalBackendSyncStatus
cbp_send_objects (ECalBackendSync *backend,
				    EDataCal        *cal,
				    const gchar     *calobj,
				    GList          **users,
				    gchar          **modified_calobj)
{
	ECalBackendPlanner        *cbplanner;
	ECalBackendPlannerPrivate *priv;

	cbplanner = E_CAL_BACKEND_PLANNER (backend);
	priv = cbplanner->priv;

	*users = NULL;
	*modified_calobj = NULL;

	g_message ("Sending objects ...");

	return GNOME_Evolution_Calendar_PermissionDenied;
}

static icaltimezone *
cbp_internal_get_default_timezone (ECalBackend *backend)
{
	/* Planner stores date in UTC */
	return icaltimezone_get_utc_timezone ();

}

static icaltimezone *
cbp_internal_get_timezone (ECalBackend *backend,
					     const gchar *tzid)
{
	ECalBackendPlanner        *cbplanner;
	ECalBackendPlannerPrivate *priv;
	icaltimezone              *zone;

	cbplanner = E_CAL_BACKEND_PLANNER (backend);
	priv = cbplanner->priv;

	if (!strcmp (tzid, "UTC"))
	        zone = icaltimezone_get_utc_timezone ();
	else {
		zone = icaltimezone_get_builtin_timezone_from_tzid (tzid);
	}

	return zone;
}

/* Object initialization function for the file backend */
static void
cbp_init (ECalBackendPlanner      *cbplanner,
	  ECalBackendPlannerClass *class)
{
	ECalBackendPlannerPrivate *priv;

	g_message ("In cbp init ...");

	priv = g_new0 (ECalBackendPlannerPrivate, 1);
	cbplanner->priv = priv;
	priv->tasks_comp = g_hash_table_new (g_str_hash, g_str_equal);

	priv->project = mrp_project_new (class->mrp_app);

	priv->uri = NULL;
	priv->project_loaded = FALSE;
}

/* Class initialization function for the file backend */
static void
cbp_class_init (ECalBackendPlannerClass *class)
{
	GObjectClass         *object_class;
	ECalBackendClass     *backend_class;
	ECalBackendSyncClass *sync_class;

	object_class = (GObjectClass *) class;
	backend_class = (ECalBackendClass *) class;
	sync_class = (ECalBackendSyncClass *) class;

	class->mrp_app = mrp_application_new ();


	parent_class = (ECalBackendSyncClass *) g_type_class_peek_parent (class);

	object_class->dispose = cbp_dispose;
	object_class->finalize = cbp_finalize;

	sync_class->is_read_only_sync = cbp_is_read_only;
	sync_class->get_cal_address_sync = cbp_get_cal_address;
 	sync_class->get_alarm_email_address_sync = cbp_get_alarm_email_address;
 	sync_class->get_ldap_attribute_sync = cbp_get_ldap_attribute;
 	sync_class->get_static_capabilities_sync = cbp_get_static_capabilities;
	sync_class->open_sync = cbp_open;
	sync_class->remove_sync = cbp_remove;
	sync_class->create_object_sync = cbp_create_object;
	sync_class->modify_object_sync = cbp_modify_object;
	sync_class->remove_object_sync = cbp_remove_object;
	sync_class->discard_alarm_sync = cbp_discard_alarm;
	sync_class->receive_objects_sync = cbp_receive_objects;
	sync_class->send_objects_sync = cbp_send_objects;
 	sync_class->get_default_object_sync = cbp_get_default_object;
	sync_class->get_object_sync = cbp_get_object;
	sync_class->get_object_list_sync = cbp_get_object_list;
	sync_class->get_timezone_sync = cbp_get_timezone;
	sync_class->add_timezone_sync = cbp_add_timezone;
	sync_class->set_default_timezone_sync = cbp_set_default_timezone;
	sync_class->get_freebusy_sync = cbp_get_free_busy;
	sync_class->get_changes_sync = cbp_get_changes;

	backend_class->is_loaded = cbp_is_loaded;
	backend_class->start_query = cbp_start_query;
	backend_class->get_mode = cbp_get_mode;
	backend_class->set_mode = cbp_set_mode;

	backend_class->internal_get_default_timezone = cbp_internal_get_default_timezone;
	backend_class->internal_get_timezone = cbp_internal_get_timezone;
}


/**
 * cbp_get_type:
 * @void:
 *
 * Registers the #ECalBackendPlanner class if necessary, and returns the type ID
 * associated to it.
 *
 * Return value: The type ID of the #ECalBackendPlanner class.
 **/
GType
e_cal_backend_planner_get_type (void)
{
	static GType cbp_type = 0;

	if (!cbp_type) {
		static GTypeInfo info = {
                        sizeof (ECalBackendPlannerClass),
                        (GBaseInitFunc) NULL,
                        (GBaseFinalizeFunc) NULL,
                        (GClassInitFunc) cbp_class_init,
                        NULL, NULL,
                        sizeof (ECalBackendPlanner),
                        0,
                        (GInstanceInitFunc) cbp_init
                };
		cbp_type = g_type_register_static (E_TYPE_CAL_BACKEND_SYNC,
								     "ECalBackendPlanner",
								     &info, 0);
	}

	return cbp_type;
}
