/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003-2004 Imendio AB
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
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
#include <time.h>
#include <glib.h>
#include <gmodule.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkiconfactory.h>
#include <gtk/gtkitemfactory.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkhpaned.h>
#include <gtk/gtkframe.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtktreemodel.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtktoggleaction.h>
#include <glib/gi18n.h>
#include <libplanner/mrp-task.h>
#include "planner-view.h"
#include "planner-conf.h"
#include "planner-cell-renderer-date.h"
#include "planner-task-dialog.h"
#include "planner-property-dialog.h"
#include "planner-gantt-model.h"
#include "planner-task-tree.h"
#include "planner-table-print-sheet.h"


struct _PlannerViewPriv {
	GtkWidget              *tree;
	GtkWidget              *frame;
	PlannerTablePrintSheet *print_sheet;
	GtkUIManager           *ui_manager;
	GtkActionGroup         *actions;
	guint                   merged_id;
};

void          activate                           (PlannerView     *view);
void          deactivate                         (PlannerView     *view);
void          init                               (PlannerView     *view,
						  PlannerWindow   *main_window);
const gchar  *get_label                          (PlannerView     *view);
const gchar  *get_menu_label                     (PlannerView     *view);
const gchar  *get_icon                           (PlannerView     *view);
const gchar  *get_name                           (PlannerView     *view);
GtkWidget    *get_widget                         (PlannerView     *view);
static void   task_view_project_loaded_cb        (MrpProject      *project,
						  PlannerView     *view);
static void   task_view_insert_task_cb           (GtkAction       *action,
						  gpointer         data);
static void   task_view_insert_tasks_cb          (GtkAction       *action,
						  gpointer         data);
static void   task_view_remove_task_cb           (GtkAction       *action,
						  gpointer         data);
static void   task_view_edit_task_cb             (GtkAction       *action,
						  gpointer         data);
static void   task_view_select_all_cb            (GtkAction       *action,
						  gpointer         data);
static void   task_view_unlink_task_cb           (GtkAction       *action,
						  gpointer         data);
static void   task_view_link_tasks_cb            (GtkAction       *action,
						  gpointer         data);
static void   task_view_indent_task_cb           (GtkAction       *action,
						  gpointer         data);
static void   task_view_move_task_up_cb          (GtkAction       *action,
						  gpointer         data);
static void   task_view_move_task_down_cb        (GtkAction       *action,
						  gpointer         data);
static void   task_view_unindent_task_cb         (GtkAction       *action,
						  gpointer         data);
static void   task_view_reset_constraint_cb      (GtkAction       *action,
						  gpointer         data);
static void   task_view_edit_custom_props_cb     (GtkAction       *action,
						  gpointer         data);
static void   task_view_highlight_critical_cb    (GtkAction       *action,
						  gpointer         data);

static void   task_view_selection_changed_cb     (PlannerTaskTree *tree,
						  PlannerView     *view);
static void   task_view_relations_changed_cb     (PlannerTaskTree *tree,
						  MrpTask         *task,
						  MrpRelation     *relation,
						  PlannerView     *view);

static void   task_view_update_ui                (PlannerView     *view);
void          print_init                         (PlannerView     *view,
						  PlannerPrintJob *job);
void          print                              (PlannerView     *view);
gint          print_get_n_pages                  (PlannerView     *view);
void          print_cleanup                      (PlannerView     *view);


static GtkActionEntry entries[] = {
	{ "InsertTask",      "planner-stock-insert-task",      N_("_Insert Task"),               "<Control>i",        N_("Insert a new task"),                 G_CALLBACK (task_view_insert_task_cb) },
	{ "InsertTasks",     "planner-stock-insert-task",      N_("In_sert Tasks..."),           NULL,                NULL,                                    G_CALLBACK (task_view_insert_tasks_cb) },
	{ "RemoveTask",      "planner-stock-remove-task",      N_("_Remove Task"),               "<Control>d",        N_("Remove the selected tasks"),         G_CALLBACK (task_view_remove_task_cb) },
	{ "EditTask",        NULL,                             N_("_Edit Task"),                 "<Shift><Control>e", NULL,                                    G_CALLBACK (task_view_edit_task_cb) },
	{ "SelectAll",       NULL,                             N_("Select _All"),                "<Control>a",        N_("Select all tasks"),                  G_CALLBACK (task_view_select_all_cb) },
	{ "UnlinkTask",      "planner-stock-unlink-task",      N_("_Unlink Task"),               NULL,                N_("Unlink the selected tasks"),         G_CALLBACK (task_view_unlink_task_cb) },
	{ "LinkTasks",       "planner-stock-link-task",        N_("_Link Tasks"),                NULL,                N_("Link the selected tasks"),           G_CALLBACK (task_view_link_tasks_cb) },
	{ "IndentTask",      "planner-stock-indent-task",      N_("I_ndent Task"),               "<Shift><Control>i", N_("Indent the selected tasks"),         G_CALLBACK (task_view_indent_task_cb) },
	{ "UnindentTask",    "planner-stock-unindent-task",    N_("Unin_dent Task"),             "<Shift><Control>u", N_("Unindent the selected tasks"),       G_CALLBACK (task_view_unindent_task_cb) },
	{ "MoveTaskUp",      "planner-stock-move-task-up",     N_("Move Task _Up"),              NULL,                N_("Move the selected tasks upwards"),   G_CALLBACK (task_view_move_task_up_cb) },
	{ "MoveTaskDown",    "planner-stock-move-task-down",   N_("Move Task Do_wn"),            NULL,                N_("Move the selected tasks downwards"), G_CALLBACK (task_view_move_task_down_cb) },
	{ "ResetConstraint", "planner-stock-reset-constraint", N_("Reset _Constraint"),          NULL,                NULL,                                    G_CALLBACK (task_view_reset_constraint_cb) },
	{ "EditCustomProps", GTK_STOCK_PROPERTIES,             N_("_Edit Custom Properties..."), NULL,                NULL,                                    G_CALLBACK (task_view_edit_custom_props_cb) },
};

static GtkToggleActionEntry toggle_entries[] = {
	{ "HighlightCriticalTasks", NULL, "_HighlightCriticalPath", NULL, NULL, G_CALLBACK (task_view_highlight_critical_cb), FALSE },
};

static guint n_entries        = G_N_ELEMENTS (entries);
static guint n_toggle_entries = G_N_ELEMENTS (toggle_entries);


#define CRITICAL_PATH_KEY "/views/task_view/highlight_critical_path"

G_MODULE_EXPORT void
activate (PlannerView *view)
{
	PlannerViewPriv *priv;
	gboolean         show_critical;
	GError          *error = NULL;

	priv = view->priv;
	
	priv->actions = gtk_action_group_new ("TaskView");
	gtk_action_group_set_translation_domain (priv->actions, GETTEXT_PACKAGE);

	gtk_action_group_add_actions (priv->actions, entries, n_entries, view);
	gtk_action_group_add_toggle_actions (priv->actions, toggle_entries, n_toggle_entries, view);

	gtk_ui_manager_insert_action_group (priv->ui_manager, priv->actions, 0);
	priv->merged_id = gtk_ui_manager_add_ui_from_file (priv->ui_manager,
							   DATADIR"/planner/ui/task-view.ui",
							   &error);
	if (error != NULL) {
		g_message("Building menu failed: %s", error->message);
		g_message ("Couldn't load: %s",DATADIR"/planner/ui/task-view.ui");
                g_error_free(error);
	}
	gtk_ui_manager_ensure_update(priv->ui_manager);

	/* Set the initial UI state. */

	show_critical = planner_conf_get_bool (CRITICAL_PATH_KEY, NULL);

	planner_task_tree_set_highlight_critical (PLANNER_TASK_TREE (priv->tree),
						  show_critical);

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION(gtk_action_group_get_action (priv->actions, "HighlightCriticalTasks")),
				      show_critical);
	
	task_view_selection_changed_cb (PLANNER_TASK_TREE (view->priv->tree), view);
}

G_MODULE_EXPORT void
deactivate (PlannerView *view)
{
	PlannerViewPriv *priv;

	priv = view->priv;
	gtk_ui_manager_remove_ui (priv->ui_manager, priv->merged_id);
}

G_MODULE_EXPORT void
init (PlannerView *view, PlannerWindow *main_window)
{
	PlannerViewPriv *priv;
	
	priv = g_new0 (PlannerViewPriv, 1);
	view->priv = priv;

	priv->ui_manager = planner_window_get_ui_manager(main_window);
}

G_MODULE_EXPORT const gchar *
get_label (PlannerView *view)
{
	g_return_val_if_fail (PLANNER_IS_VIEW (view), NULL);

	return _("Tasks");
}

G_MODULE_EXPORT const gchar *
get_menu_label (PlannerView *view)
{
	g_return_val_if_fail (PLANNER_IS_VIEW (view), NULL);

	return _("_Tasks");
}

G_MODULE_EXPORT const gchar *
get_icon (PlannerView *view)
{
	g_return_val_if_fail (PLANNER_IS_VIEW (view), NULL);

	return IMAGEDIR "/tasks.png";
}

G_MODULE_EXPORT const gchar *
get_name (PlannerView *view)
{
	g_return_val_if_fail (PLANNER_IS_VIEW (view), NULL);

	return "task_view";
}

G_MODULE_EXPORT GtkWidget *
get_widget (PlannerView *view)
{
	PlannerViewPriv   *priv;
	MrpProject   *project;
	GtkWidget    *sw;
	PlannerGanttModel *model;

	g_return_val_if_fail (PLANNER_IS_VIEW (view), NULL);

	priv = view->priv;
	
	if (priv->tree == NULL) {
		project = planner_window_get_project (view->main_window);

		g_signal_connect (project,
				  "loaded",
				  G_CALLBACK (task_view_project_loaded_cb),
				  view);

		sw = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
						GTK_POLICY_AUTOMATIC,
						GTK_POLICY_AUTOMATIC);
		
		priv->frame = gtk_frame_new (NULL);
		gtk_frame_set_shadow_type (GTK_FRAME (priv->frame), GTK_SHADOW_IN);

		gtk_container_add (GTK_CONTAINER (priv->frame), sw);

		model = planner_gantt_model_new (project);

		priv->tree = planner_task_tree_new (view->main_window,
						    model,
						    TRUE,
						    /* i18n: WBS is sort for work breakdown structure, and is a
						     * project management term. You might want to leave it
						     * untranslated unless there is a localized term for it.
						     */
						    COL_WBS,    _("WBS"),
						    COL_NAME,   _("Name"), 
						    COL_START,  _("Start"), 
						    COL_FINISH, _("Finish"),
						    COL_WORK,   _("Work"),
						    COL_SLACK,  _("Slack"),
						    COL_COST,   _("Cost"),
						    COL_ASSIGNED_TO, _("Assigned to"),
						    -1);

		g_object_unref (model);

		gtk_container_add (GTK_CONTAINER (sw), priv->tree);

		g_signal_connect (priv->tree,
				  "selection-changed",
				  G_CALLBACK (task_view_selection_changed_cb),
				  view);

		g_signal_connect (priv->tree,
				  "relation-added",
				  G_CALLBACK (task_view_relations_changed_cb),
				  view);

		g_signal_connect (priv->tree,
				  "relation-removed",
				  G_CALLBACK (task_view_relations_changed_cb),
				  view);

		gtk_widget_show (priv->tree);
		gtk_widget_show (sw);
		gtk_widget_show (priv->frame);
	}

	return priv->frame;
}

static void
task_view_project_loaded_cb (MrpProject *project,
			     PlannerView     *view)
{
	GtkTreeModel *model;
	PlannerViewPriv   *priv;

 	priv = view->priv;

	model = GTK_TREE_MODEL (planner_gantt_model_new (project));

	planner_task_tree_set_model (PLANNER_TASK_TREE (priv->tree),
				PLANNER_GANTT_MODEL (model));

	g_object_unref (model);
}

/* Command callbacks. */

static void
task_view_insert_task_cb (GtkAction *action,
			  gpointer   data)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_insert_task (PLANNER_TASK_TREE (view->priv->tree));
}

static void
task_view_insert_tasks_cb (GtkAction *action,
			   gpointer   data)
{
	PlannerView *view = PLANNER_VIEW (data);

	planner_task_tree_insert_tasks (PLANNER_TASK_TREE (view->priv->tree));
}

static void
task_view_remove_task_cb (GtkAction *action, 
			  gpointer   data)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_remove_task (PLANNER_TASK_TREE (view->priv->tree));
}

static void
task_view_edit_task_cb (GtkAction *action, 
			gpointer   data)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_edit_task (PLANNER_TASK_TREE (view->priv->tree),
				     PLANNER_TASK_DIALOG_PAGE_GENERAL);
}

static void
task_view_select_all_cb (GtkAction *action, 
			 gpointer   data)
{
	PlannerView *view;
	
	view = PLANNER_VIEW (data);
	
	planner_task_tree_select_all (PLANNER_TASK_TREE (view->priv->tree));
}

static void
task_view_unlink_task_cb (GtkAction *action, 
			  gpointer   data)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_unlink_task (PLANNER_TASK_TREE (view->priv->tree));
}

static void
task_view_link_tasks_cb (GtkAction *action,
			 gpointer   data)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_link_tasks (PLANNER_TASK_TREE (view->priv->tree),
				      MRP_RELATION_FS);
}

static void
task_view_indent_task_cb (GtkAction *action, 
			  gpointer   data)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_indent_task (PLANNER_TASK_TREE (view->priv->tree));
}

static void 
task_view_move_task_up_cb (GtkAction *action,
			   gpointer   data)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);
	
	planner_task_tree_move_task_up (PLANNER_TASK_TREE (view->priv->tree));
}

static void 
task_view_move_task_down_cb (GtkAction *action,
			     gpointer   data)
{
	PlannerView *view;
	
	view = PLANNER_VIEW (data);

	planner_task_tree_move_task_down (PLANNER_TASK_TREE (view->priv->tree));
}

static void
task_view_unindent_task_cb (GtkAction *action, 
			    gpointer   data)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_unindent_task (PLANNER_TASK_TREE (view->priv->tree));
}

static void
task_view_reset_constraint_cb (GtkAction *action, 
			       gpointer   data)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_reset_constraint (PLANNER_TASK_TREE (view->priv->tree));
}

static void
task_view_edit_custom_props_cb (GtkAction *action, 
				gpointer   data)
{
	PlannerView     *view;
	GtkWidget  *dialog;
	MrpProject *project;

	view = PLANNER_VIEW (data);
	
	project = planner_window_get_project (view->main_window);
	
	dialog = planner_property_dialog_new (view->main_window,
					 project,
					 MRP_TYPE_TASK,
					 _("Edit custom task properties"));
	
	gtk_window_set_default_size (GTK_WINDOW (dialog), 500, 300);
	gtk_widget_show (dialog);
}

static void
task_view_highlight_critical_cb (GtkAction *action,
				 gpointer   data)
{
	PlannerViewPriv *priv;
	gboolean         state;
	
	priv = PLANNER_VIEW (data)->priv;

	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION(action));

	planner_task_tree_set_highlight_critical (
		PLANNER_TASK_TREE (priv->tree),
		state);

	planner_conf_set_bool (CRITICAL_PATH_KEY, state, NULL);
}

static void 
task_view_selection_changed_cb (PlannerTaskTree *tree, PlannerView *view)
{
	g_return_if_fail (PLANNER_IS_VIEW (view));

	task_view_update_ui (view);
}

static void
task_view_relations_changed_cb (PlannerTaskTree  *tree,
				MrpTask     *task,
				MrpRelation *relation,
				PlannerView      *view)
{
	g_return_if_fail (PLANNER_IS_VIEW (view));

	task_view_update_ui (view);
}

	
G_MODULE_EXPORT void
print_init (PlannerView     *view,
	    PlannerPrintJob *job)
{
	PlannerViewPriv *priv;
	
	g_return_if_fail (PLANNER_IS_VIEW (view));
	g_return_if_fail (PLANNER_IS_PRINT_JOB (job));

	priv = view->priv;
	
	g_assert (priv->print_sheet == NULL);

	priv->print_sheet = planner_table_print_sheet_new (PLANNER_VIEW (view), job, 
						      GTK_TREE_VIEW (priv->tree));
}

G_MODULE_EXPORT void
print (PlannerView *view)

{
	g_return_if_fail (PLANNER_IS_VIEW (view));

	g_assert (view->priv->print_sheet);
	
	planner_table_print_sheet_output (view->priv->print_sheet);
}

G_MODULE_EXPORT gint
print_get_n_pages (PlannerView *view)
{
	g_return_val_if_fail (PLANNER_IS_VIEW (view), 0);

	g_assert (view->priv->print_sheet);
	
	return planner_table_print_sheet_get_n_pages (view->priv->print_sheet);
}

G_MODULE_EXPORT void
print_cleanup (PlannerView *view)

{
	g_return_if_fail (PLANNER_IS_VIEW (view));

	g_assert (view->priv->print_sheet);
	
	planner_table_print_sheet_free (view->priv->print_sheet);
	view->priv->print_sheet = NULL;
}

static void
task_view_update_ui (PlannerView *view)
{
	PlannerViewPriv *priv;
	GList      *list, *l;
	gboolean    value;
	gboolean    rel_value  = FALSE;
	gboolean    link_value = FALSE;
	gint	    count_value = 0;
	
	if (!view->activated) {
		return;
	}
	
	priv = view->priv;

	list = planner_task_tree_get_selected_tasks (PLANNER_TASK_TREE (priv->tree));

	for (l = list; l; l = l->next) {
		if (mrp_task_has_relation (MRP_TASK (l->data))) {
			rel_value = TRUE;
			break;
		}
	}

	for (l = list; l; l = l->next) {
		count_value++;
	}

	value = (list != NULL);
	link_value = (count_value >= 2);


	g_object_set (gtk_action_group_get_action (priv->actions, "EditTask"),
		      "sensitive", value, 
		      NULL);

	g_object_set (gtk_action_group_get_action (priv->actions, "RemoveTask"),
		      "sensitive", value, 
		      NULL);

	g_object_set (gtk_action_group_get_action (priv->actions, "UnlinkTask"),
		      "sensitive", rel_value, 
		      NULL);

	g_object_set (gtk_action_group_get_action (priv->actions, "LinkTasks"),
		      "sensitive", link_value, 
		      NULL);

	g_object_set (gtk_action_group_get_action (priv->actions, "IndentTask"),
		      "sensitive", value, 
		      NULL);

	g_object_set (gtk_action_group_get_action (priv->actions, "UnindentTask"),
		      "sensitive", value, 
		      NULL);

	g_object_set (gtk_action_group_get_action (priv->actions, "MoveTaskUp"),
		      "sensitive", value, 
		      NULL);

	g_object_set (gtk_action_group_get_action (priv->actions, "MoveTaskDown"),
		      "sensitive", value, 
		      NULL);

	g_object_set (gtk_action_group_get_action (priv->actions, "ResetConstraint"),
		      "sensitive", value, 
		      NULL);

	g_list_free (list);
}

	
