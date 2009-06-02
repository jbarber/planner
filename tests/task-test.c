/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#include <config.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include "libplanner/mrp-project.h"
#include "self-check.h"

#define DAY (60*60*8)

gint
main (gint argc, gchar **argv)
{
	MrpApplication *app;
	MrpProject     *project;
	MrpTask        *task1, *task2, *task3, *task4;
	mrptime         project_start, t1;
	gint            work, duration;
	gboolean        critical;
	gboolean	success;
	MrpTask        *root;
	MrpRelation    *relation;

        g_type_init ();

	app = mrp_application_new ();

	project_start = mrp_time_from_string ("20020218", NULL);

	project = mrp_project_new (app);

	g_object_set (project, "project_start", project_start, NULL);

	task1 = g_object_new (MRP_TYPE_TASK,
			      "name", "T1",
			      "work", DAY,
			      NULL);

	mrp_project_insert_task (project, NULL, -1, task1);

	g_object_get (task1,
		      "critical", &critical,
		      "work", &work,
		      "duration", &duration,
		      NULL);

	/* Check that the critical path is not totally broken. */
	CHECK_BOOLEAN_RESULT (critical, TRUE);

	/* Check that we don't mess with work, it should be as set above. */
	CHECK_INTEGER_RESULT (work, DAY);

	/* Check that duration is calculated correctly. Duration == work if no
	 * resources are assigned.
	 */
        CHECK_INTEGER_RESULT (duration, DAY);

	task2 = g_object_new (MRP_TYPE_TASK,
			      "name", "T2",
			      "work", 10*DAY,
			      NULL);

	mrp_project_insert_task (project, NULL, -1, task2);

	g_object_get (task1,
		      "critical", &critical,
		      NULL);

	/* Task2 is now critical, task1 not. */
	CHECK_BOOLEAN_RESULT (critical, FALSE);

	g_object_get (task2,
		      "critical", &critical,
		      NULL);

	/* Task2 is now critical, task1 not. */
	CHECK_BOOLEAN_RESULT (critical, TRUE);

	/* Check that start and finish are calculated correctly. */
	root = mrp_project_get_root_task (project);

        CHECK_INTEGER_RESULT (mrp_task_get_start (root), project_start);
        CHECK_INTEGER_RESULT (mrp_task_get_finish (root), mrp_task_get_finish (task2));

	/* Test predecessors. */
	relation = mrp_task_add_predecessor (task1,
					     task2,
					     MRP_RELATION_FS,
					     0,
					     NULL);

	/* Now the project finish should be the finish of the successor,
	 * task1.
	 */
	CHECK_INTEGER_RESULT (mrp_task_get_finish (root), mrp_task_get_finish (task1));

	/* Check that we can't add the same relation twice. */
	relation = mrp_task_add_predecessor (task1,
					     task2,
					     MRP_RELATION_FS,
					     0,
					     NULL);

	CHECK_POINTER_RESULT (relation, NULL);

	/* Check that we can't add a loop. */
	relation = mrp_task_add_predecessor (task2,
					     task1,
					     MRP_RELATION_FS,
					     0,
					     NULL);

	CHECK_POINTER_RESULT (relation, NULL);

	/* Check that we can't make task2 the parent of task1 with the
	 * predecessor relation in place
	 */
	success = mrp_project_move_task (project,
					 task1,
					 NULL,
					 task2,
					 FALSE,
					 NULL);

	CHECK_BOOLEAN_RESULT (success, FALSE);

	/* Check that we can't make task1 the parent of task2 with the
	 * predecessor relation in place (bug #382548).
	 * This test verifies that a loop is created in the dependency graph by
	 * this action and that it is detected.
	 */
	success = mrp_project_move_task (project,
					 task2,
					 NULL,
					 task1,
					 FALSE,
					 NULL);

	CHECK_BOOLEAN_RESULT (success, FALSE);

	/* Add task3 as parent of task2 and see if we can't make task1 the
	 * parent of task3. This test verifies that loops that include direct
	 * children but not the parent, are detected.
	 */
	task3 = g_object_new (MRP_TYPE_TASK,
			      "name", "T3",
			      "work", 10*DAY,
			      NULL);

	mrp_project_insert_task (project, NULL, -1, task3);

	success = mrp_project_move_task (project,
					 task2,
					 NULL,
					 task3,
					 FALSE,
					 NULL);
	CHECK_BOOLEAN_RESULT (success, TRUE);

	success = mrp_project_move_task (project,
					 task3,
					 NULL,
					 task1,
					 FALSE,
					 NULL);

	CHECK_BOOLEAN_RESULT (success, FALSE);

	/* Add task4 as parent of task3 and see if we can't make task1 the
	 * parent of task3. This test verifies that loops that include indirect
	 * children but not the parent or a direct child, are detected. If this
	 * works, we'll assume detection is recursive as it should be.
	 */
	task4 = g_object_new (MRP_TYPE_TASK,
			      "name", "T4",
			      "work", 10*DAY,
			      NULL);

	mrp_project_insert_task (project, NULL, -1, task4);

	success = mrp_project_move_task (project,
					 task3,
					 NULL,
					 task4,
					 FALSE,
					 NULL);
	CHECK_BOOLEAN_RESULT (success, TRUE);

	success = mrp_project_move_task (project,
					 task4,
					 NULL,
					 task1,
					 FALSE,
					 NULL);

	CHECK_BOOLEAN_RESULT (success, FALSE);

	/* Retrieve a relation and see that it's correct. */
	relation = mrp_task_get_relation (task1, task2);

	CHECK_POINTER_RESULT (mrp_relation_get_successor (relation), task1);
	CHECK_POINTER_RESULT (mrp_relation_get_predecessor (relation), task2);

	t1 = mrp_task_get_start (task1);

	g_object_set (relation, "lag", DAY, NULL);

	/* Check for correct lag set. */
	CHECK_INTEGER_RESULT (mrp_relation_get_lag (relation), DAY);

	/* Check if the lag effects the task correctly. */
	CHECK_INTEGER_RESULT (t1 + DAY, mrp_task_get_start (task1));

	/* Milestone. */
	g_object_set (task2, "type", MRP_TASK_TYPE_MILESTONE, NULL);
	CHECK_INTEGER_RESULT (mrp_task_get_finish (task2) - mrp_task_get_start (task2), 0);
	CHECK_INTEGER_RESULT (mrp_task_get_work (task2), 0);
	CHECK_INTEGER_RESULT (mrp_task_get_duration (task2), 0);

	/* More tests needed... */


	return EXIT_SUCCESS;
}

