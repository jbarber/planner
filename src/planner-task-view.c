/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003-2004 Imendio HB
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
#include <libgnome/gnome-i18n.h>
#include <bonobo/bonobo-ui-component.h>
#include <bonobo/bonobo-ui-util.h>
#include <libplanner/mrp-task.h>
#include "planner-view.h"
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
};

void          activate                           (PlannerView                  *view);
void          deactivate                         (PlannerView                  *view);
void          init                               (PlannerView                  *view,
						  PlannerWindow                *main_window);
const gchar  *get_label                          (PlannerView                  *view);
const gchar  *get_menu_label                     (PlannerView                  *view);
const gchar  *get_icon                           (PlannerView                  *view);
const gchar  *get_name                           (PlannerView                  *view);
GtkWidget    *get_widget                         (PlannerView                  *view);
static void   task_view_project_loaded_cb        (MrpProject                   *project,
						  PlannerView                  *view);
static void   task_view_insert_task_cb           (BonoboUIComponent            *component,
						  gpointer                      data,
						  const char                   *cname);
static void   task_view_insert_tasks_cb          (BonoboUIComponent            *component,
						  gpointer                      data,
						  const char                   *cname);
static void   task_view_remove_task_cb           (BonoboUIComponent            *component,
						  gpointer                      data,
						  const char                   *cname);
static void   task_view_edit_task_cb             (BonoboUIComponent            *component,
						  gpointer                      data,
						  const char                   *cname);
static void   task_view_select_all_cb            (BonoboUIComponent            *component,
						  gpointer                      data,
						  const char                   *cname);
static void   task_view_unlink_task_cb           (BonoboUIComponent            *component,
						  gpointer                      data,
						  const char                   *cname);
static void   task_view_link_tasks_cb           (BonoboUIComponent            *component,
						  gpointer                      data,
						  const char                   *cname);
static void   task_view_indent_task_cb           (BonoboUIComponent            *component,
						  gpointer                      data,
						  const char                   *cname);
static void   task_view_move_task_up_cb          (BonoboUIComponent            *component,
						  gpointer                      data,
						  const char                   *cname);
static void   task_view_move_task_down_cb        (BonoboUIComponent            *component,
						  gpointer                      data,
						  const char                   *cname);
static void   task_view_unindent_task_cb         (BonoboUIComponent            *component,
						  gpointer                      data,
						  const char                   *cname);
static void   task_view_reset_constraint_cb      (BonoboUIComponent            *component,
						  gpointer                      data,
						  const char                   *cname);
static void   task_view_edit_custom_props_cb     (BonoboUIComponent            *component,
						  gpointer                      data,
						  const char                   *cname);
static void   task_view_selection_changed_cb     (PlannerTaskTree              *tree,
						  PlannerView                  *view);
static void   task_view_relations_changed_cb     (PlannerTaskTree              *tree,
						  MrpTask                      *task,
						  MrpRelation                  *relation,
						  PlannerView                  *view);
static void   task_view_ui_component_event       (BonoboUIComponent            *comp,
						  const gchar                  *path,
						  Bonobo_UIComponent_EventType  type,
						  const gchar                  *state_string,
						  PlannerView                  *view);
static void   task_view_update_ui                (PlannerView                  *view);
void          print_init                         (PlannerView                  *view,
						  PlannerPrintJob              *job);
void          print                              (PlannerView                  *view);
gint          print_get_n_pages                  (PlannerView                  *view);
void          print_cleanup                      (PlannerView                  *view);


static BonoboUIVerb verbs[] = {
	BONOBO_UI_VERB ("InsertTask",		task_view_insert_task_cb),
	BONOBO_UI_VERB ("InsertTasks",		task_view_insert_tasks_cb),
	BONOBO_UI_VERB ("RemoveTask",		task_view_remove_task_cb),
	BONOBO_UI_VERB ("EditTask",   	        task_view_edit_task_cb),
	BONOBO_UI_VERB ("SelectAll",		task_view_select_all_cb),
	BONOBO_UI_VERB ("UnlinkTask",		task_view_unlink_task_cb),
	BONOBO_UI_VERB ("LinkTasks",		task_view_link_tasks_cb),
	BONOBO_UI_VERB ("IndentTask",		task_view_indent_task_cb),
	BONOBO_UI_VERB ("UnindentTask",		task_view_unindent_task_cb),
	BONOBO_UI_VERB ("MoveTaskUp",		task_view_move_task_up_cb),
	BONOBO_UI_VERB ("MoveTaskDown",		task_view_move_task_down_cb),
	BONOBO_UI_VERB ("ResetConstraint",	task_view_reset_constraint_cb),
	BONOBO_UI_VERB ("EditCustomProps",	task_view_edit_custom_props_cb),

	BONOBO_UI_VERB_END
};

#define CRITICAL_PATH_KEY "/apps/planner/views/task_view/highlight_critical_path"

G_MODULE_EXPORT void
activate (PlannerView *view)
{
	PlannerViewPriv *priv;
	gboolean         show_critical;
	GConfClient     *gconf_client;

	planner_view_activate_helper (view,
				      DATADIR
				      "/planner/ui/task-view.ui",
				      "taskview",
				      verbs);
	
	priv = view->priv;
	
	/* Set the initial UI state. */

	gconf_client = planner_application_get_gconf_client ();
	show_critical = gconf_client_get_bool (gconf_client,
					       CRITICAL_PATH_KEY,
					       NULL);

	planner_task_tree_set_highlight_critical (PLANNER_TASK_TREE (priv->tree),
						  show_critical);

	bonobo_ui_component_set_prop (view->ui_component, 
				      "/commands/HighlightCriticalTasks",
				      "state", show_critical ? "1" : "0",
				      NULL);
	
	task_view_selection_changed_cb (PLANNER_TASK_TREE (view->priv->tree), view);
}

G_MODULE_EXPORT void
deactivate (PlannerView *view)
{
	planner_view_deactivate_helper (view);
}

G_MODULE_EXPORT void
init (PlannerView *view, PlannerWindow *main_window)
{
	PlannerViewPriv *priv;
	
	priv = g_new0 (PlannerViewPriv, 1);
	view->priv = priv;

	g_signal_connect (view->ui_component,
			  "ui_event",
			  G_CALLBACK (task_view_ui_component_event),
			  view);
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
task_view_insert_task_cb (BonoboUIComponent *component, 
			  gpointer           data, 
			  const char        *cname)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_insert_task (PLANNER_TASK_TREE (view->priv->tree));
}

static void
task_view_insert_tasks_cb (BonoboUIComponent *component, 
			   gpointer           data, 
			   const char        *cname)
{
	PlannerView *view = PLANNER_VIEW (data);

	planner_task_tree_insert_tasks (PLANNER_TASK_TREE (view->priv->tree));
}

static void
task_view_remove_task_cb (BonoboUIComponent *component, 
			  gpointer           data, 
			  const char        *cname)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_remove_task (PLANNER_TASK_TREE (view->priv->tree));
}

static void
task_view_edit_task_cb (BonoboUIComponent *component, 
			gpointer           data, 
			const char        *cname)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_edit_task (PLANNER_TASK_TREE (view->priv->tree),
				     PLANNER_TASK_DIALOG_PAGE_GENERAL);
}

static void
task_view_select_all_cb (BonoboUIComponent *component, 
			 gpointer           data, 
			 const char        *cname)
{
	PlannerView *view;
	
	view = PLANNER_VIEW (data);
	
	planner_task_tree_select_all (PLANNER_TASK_TREE (view->priv->tree));
}

static void
task_view_unlink_task_cb (BonoboUIComponent *component, 
			  gpointer           data, 
			  const char        *cname)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_unlink_task (PLANNER_TASK_TREE (view->priv->tree));
}

static void
task_view_link_tasks_cb (BonoboUIComponent *component,
			  gpointer           data,
			  const char        *cname)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_link_tasks (PLANNER_TASK_TREE (view->priv->tree),
				      MRP_RELATION_FS);
}

static void
task_view_indent_task_cb (BonoboUIComponent *component, 
			  gpointer           data, 
			  const char        *cname)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_indent_task (PLANNER_TASK_TREE (view->priv->tree));
}

static void 
task_view_move_task_up_cb (BonoboUIComponent *component,
			   gpointer	        data,
			   const char	       *cname)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);
	
	planner_task_tree_move_task_up (PLANNER_TASK_TREE (view->priv->tree));
}

static void 
task_view_move_task_down_cb (BonoboUIComponent *component,
			     gpointer	          data,
			     const char	 *cname)
{
	PlannerView *view;
	
	view = PLANNER_VIEW (data);

	planner_task_tree_move_task_down (PLANNER_TASK_TREE (view->priv->tree));
}

static void
task_view_unindent_task_cb (BonoboUIComponent *component, 
			    gpointer           data, 
			    const char        *cname)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_unindent_task (PLANNER_TASK_TREE (view->priv->tree));
}

static void
task_view_reset_constraint_cb (BonoboUIComponent *component, 
			       gpointer           data, 
			       const char        *cname)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_reset_constraint (PLANNER_TASK_TREE (view->priv->tree));
}

static void
task_view_edit_custom_props_cb (BonoboUIComponent *component, 
				gpointer           data, 
				const char        *cname)
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

static void
task_view_ui_component_event (BonoboUIComponent            *comp,
			      const gchar                  *path,
			      Bonobo_UIComponent_EventType  type,
			      const gchar                  *state_string,
			      PlannerView                  *view)
{
	PlannerViewPriv *priv;
	gboolean         state;
	GConfClient     *gconf_client;
	
	priv = view->priv;

	if (!strcmp (path, "HighlightCriticalTasks")) {
		state = !strcmp (state_string, "1");

		planner_task_tree_set_highlight_critical (PLANNER_TASK_TREE (priv->tree),
							  state);

		gconf_client = planner_application_get_gconf_client ();
		gconf_client_set_bool (gconf_client,
				       CRITICAL_PATH_KEY,
				       state,
				       NULL);
	}
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
	gchar      *value;
	gchar      *rel_value = "0";
	gchar	   *link_value = "0";
	gint	    count_value = 0;
	
	if (!view->activated) {
		return;
	}
	
	priv = view->priv;

	list = planner_task_tree_get_selected_tasks (PLANNER_TASK_TREE (priv->tree));

	for (l = list; l; l = l->next) {
		if (mrp_task_has_relation (MRP_TASK (l->data))) {
			rel_value = "1";
			break;
		}
	}

	for (l = list; l; l = l->next) {
		count_value++;
	}

	value = (list != NULL) ? "1" : "0";
	link_value = (count_value >= 2) ? "1" : "0";

	bonobo_ui_component_freeze (view->ui_component, NULL);

	bonobo_ui_component_set_prop (view->ui_component, 
				      "/commands/EditTask",
				      "sensitive", value, 
				      NULL);

	bonobo_ui_component_set_prop (view->ui_component, 
				      "/commands/RemoveTask",
				      "sensitive", value, 
				      NULL);

	bonobo_ui_component_set_prop (view->ui_component, 
				      "/commands/UnlinkTask",
				      "sensitive", rel_value, 
				      NULL);

	bonobo_ui_component_set_prop (view->ui_component, 
				      "/commands/LinkTasks",
				      "sensitive", link_value, 
				      NULL);

	bonobo_ui_component_set_prop (view->ui_component, 
				      "/commands/IndentTask",
				      "sensitive", value, 
				      NULL);

	bonobo_ui_component_set_prop (view->ui_component, 
				      "/commands/UnindentTask",
				      "sensitive", value, 
				      NULL);

	bonobo_ui_component_set_prop (view->ui_component, 
				      "/commands/MoveTaskUp",
				      "sensitive", value, 
				      NULL);
	
	bonobo_ui_component_set_prop (view->ui_component, 
				      "/commands/MoveTaskDown",
				      "sensitive", value, 
				      NULL);

	bonobo_ui_component_set_prop (view->ui_component, 
				      "/commands/ResetConstraint",
				      "sensitive", value, 
				      NULL);

	bonobo_ui_component_thaw (view->ui_component, NULL);

	g_list_free (list);
}

	
