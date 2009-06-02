#include <config.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/parser.h>
#include "libplanner/mrp-project.h"
#include "self-check.h"

typedef struct {
	mrptime     project_start;

	GHashTable *task_hash;
} ProjectData;

typedef struct {
	gchar     *name;
	mrptime    start;
	mrptime    finish;
	gint       duration;
	gint       work;
} TaskData;



/* Get a value for a node either carried as an attibute or as
 * the content of a child.
 *
 * Returns xmlChar, must be xmlFreed.
 */
static gchar *
old_xml_get_value (xmlNodePtr node, const char *name)
{
	gchar      *val;
	xmlNodePtr  child;

	val = (gchar *) xmlGetProp (node, name);

	if (val != NULL) {
		return val;
	}

	child = node->children;

	while (child != NULL) {
		if (!strcmp (child->name, name)) {
		        /*
			 * !!! Inefficient, but ...
			 */
			val = xmlNodeGetContent(child);
			if (val != NULL) {
				return val;
			}
		}
		child = child->next;
	}

	return NULL;
}

/*
 * Get a value for a node either carried as an attibute or as
 * the content of a child.
 *
 * Returns a g_malloc'ed string.  Caller must free.
 */
static gchar *
old_xml_get_string (xmlNodePtr node, const char *name)
{
	char *ret, *val;

	g_return_val_if_fail (node != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	val = old_xml_get_value (node, name);
	ret = g_strdup (val);
	xmlFree (val);

	return ret;
}

static mrptime
old_xml_get_date (xmlNodePtr node, const char *name)
{
	gchar   *val;
	mrptime  t;

	g_return_val_if_fail (node != NULL, MRP_TIME_INVALID);
	g_return_val_if_fail (name != NULL, MRP_TIME_INVALID);

	val = old_xml_get_value (node, name);

	if (val) {
		t = mrp_time_from_string (val, NULL);
		xmlFree (val);
	} else {
		t = 0;
	}

	return t;
}

/* Search a child by name, if needed go down the tree to find it.
 */
static xmlNodePtr
old_xml_search_child (xmlNodePtr node, const gchar *name)
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
		ret = old_xml_search_child (child, name);
		if (ret != NULL)
			return ret;
		child = child->next;
	}
	return NULL;
}

static TaskData *
read_task (xmlNodePtr node)
{
	TaskData *task_data;

	task_data = g_new0 (TaskData, 1);

	task_data->name = old_xml_get_string (node, "name");

	task_data->start = old_xml_get_date (node, "start");
	task_data->finish = old_xml_get_date (node, "end");
	task_data->duration = old_xml_get_date (node, "duration");
	task_data->work = old_xml_get_date (node, "work");

	return task_data;
}

static void
read_tasks (ProjectData *data, xmlNodePtr node)
{
	xmlNodePtr  task;
	TaskData   *task_data;

	for (task = node->children; task; task = task->next) {
		if (strcmp (task->name, "task") == 0){
			task_data = read_task (task);

			if (g_hash_table_lookup (data->task_hash, task_data->name)) {
				g_print ("Duplicate name %s\n", task_data->name);
				g_assert_not_reached ();
			}

			g_hash_table_insert (data->task_hash, task_data->name, task_data);

			read_tasks (data, task);
		}
	}
}

static ProjectData *
read_project (const gchar *filename)
{
	ProjectData *data;
	xmlDocPtr    doc;
	xmlNodePtr   tasks;

	data = g_new0 (ProjectData, 1);

	data->task_hash = g_hash_table_new (g_str_hash, g_str_equal);

	doc = xmlParseFile (filename);
	g_assert (doc != NULL);

	data->project_start = old_xml_get_date (doc->children, "project-start");

	tasks = old_xml_search_child (doc->children, "tasks");
	read_tasks (data, tasks);

	return data;
}

static void
check_project (ProjectData *data, MrpProject *project)
{
	MrpTask  *task;
	GList    *tasks, *l;
	TaskData *task_data;

	/* Project start. */
	CHECK_INTEGER_RESULT (mrp_project_get_project_start (project), data->project_start);

	tasks = mrp_project_get_all_tasks (project);

	/* Sanity check. */
	CHECK_INTEGER_RESULT (g_list_length (tasks), g_hash_table_size (data->task_hash));

	for (l = tasks; l; l = l->next) {
		task = l->data;

		task_data = g_hash_table_lookup (data->task_hash, mrp_task_get_name (task));
		g_assert (task_data != NULL);

		/* Check start and finish. */
		CHECK_INTEGER_RESULT (mrp_task_get_start (task), task_data->start);
		CHECK_INTEGER_RESULT (mrp_task_get_finish (task), task_data->finish);
	}

	g_list_free (tasks);
}

gint
main (gint argc, gchar **argv)
{
	MrpApplication  *app;
	ProjectData     *data;
	MrpProject      *project;
	gchar           *buf;
	gchar           *tmp;
	gint             i;
	const gchar    *filenames[] = {
		"test-1.planner",
		"test-2.planner",
		NULL
	};

        g_type_init ();

	app = mrp_application_new ();

	i = 0;
	while (filenames[i]) {
		tmp = g_build_filename (EXAMPLESDIR, filenames[i], NULL);

		/* Just parse the XML. */
		data = read_project (tmp);

		/* Create a project from the same file. */
		project = mrp_project_new (app);
		g_assert (g_file_get_contents (tmp, &buf, NULL, NULL));
		g_assert (mrp_project_load_from_xml (project, buf, NULL));

		g_free (buf);
		g_free (tmp);

		/* Reschedule the project and check that the info is correct. */
		mrp_project_reschedule (project);
		check_project (data, project);

		i++;
	}

	return EXIT_SUCCESS;
}

