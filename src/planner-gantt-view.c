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
#include <math.h>
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
#include <bonobo/bonobo-control.h>
#include <libplanner/mrp-task.h>
#include "planner-view.h"
#include "planner-cell-renderer-date.h"
#include "planner-task-dialog.h"
#include "planner-resource-dialog.h"
#include "planner-gantt-model.h"
#include "planner-task-tree.h"
#include "planner-gantt-chart.h"
#include "planner-gantt-print.h"

struct _PlannerViewPriv {
	GtkWidget             *paned;
	GtkWidget             *tree;
	GtkWidget             *gantt;
	PlannerGanttPrintData *print_data;
};

static GtkWidget *gantt_view_create_widget                (PlannerView                  *view);
static void       gantt_view_insert_task_cb               (BonoboUIComponent            *component,
							   gpointer                      data,
							   const char                   *cname);
static void       gantt_view_insert_tasks_cb              (BonoboUIComponent            *component,
							   gpointer                      data,
							   const char                   *cname);
static void       gantt_view_remove_task_cb               (BonoboUIComponent            *component,
							   gpointer                      data,
							   const char                   *cname);
static void       gantt_view_edit_task_cb                 (BonoboUIComponent            *component,
							   gpointer                      data,
							   const char                   *cname);
static void       gantt_view_select_all_cb                (BonoboUIComponent            *component,
							   gpointer                      data,
							   const char                   *cname);
static void       gantt_view_unlink_task_cb               (BonoboUIComponent            *component,
							   gpointer                      data,
							   const char                   *cname);
static void       gantt_view_link_tasks_cb               (BonoboUIComponent            *component,
							  gpointer                      data,
							  const char                   *cname);
static void       gantt_view_indent_task_cb               (BonoboUIComponent            *component,
							   gpointer                      data,
							   const char                   *cname);
static void       gantt_view_unindent_task_cb             (BonoboUIComponent            *component,
							   gpointer                      data,
							   const char                   *cname);
static void       gantt_view_move_task_up_cb              (BonoboUIComponent            *component,
							   gpointer                      data,
							   const char                   *cname);
static void       gantt_view_move_task_down_cb            (BonoboUIComponent            *component,
							   gpointer                      data,
							   const char                   *cname);
static void       gantt_view_reset_constraint_cb          (BonoboUIComponent            *component,
							   gpointer                      data,
							   const char                   *cname);
static void       gantt_view_zoom_to_fit_cb               (BonoboUIComponent            *component,
							   gpointer                      data,
							   const char                   *cname);
static void       gantt_view_zoom_in_cb                   (BonoboUIComponent            *component,
							   gpointer                      data,
							   const char                   *cname);
static void       gantt_view_zoom_out_cb                  (BonoboUIComponent            *component,
							   gpointer                      data,
							   const char                   *cname);
static void       gantt_view_test_cb                      (BonoboUIComponent            *component,
							   gpointer                      data,
							   const char                   *cname);
static void       gantt_view_update_row_and_header_height (PlannerView                  *view);
static void       gantt_view_tree_style_set_cb            (GtkWidget                    *tree,
							   GtkStyle                     *prev_style,
							   PlannerView                  *view);
static void       gantt_view_selection_changed_cb         (PlannerTaskTree              *tree,
							   PlannerView                  *view);
static void       gantt_view_ui_component_event           (BonoboUIComponent            *comp,
							   const gchar                  *path,
							   Bonobo_UIComponent_EventType  type,
							   const gchar                  *state_string,
							   PlannerView                  *view);
static void       gantt_view_update_zoom_sensitivity      (PlannerView                  *view);
static void       gantt_view_gantt_status_updated_cb      (PlannerGanttChart            *gantt,
							   const gchar                  *message,
							   PlannerView                  *view);
static void       gantt_view_relations_changed_cb         (PlannerTaskTree              *tree,
							   MrpTask                      *task,
							   MrpRelation                  *relation,
							   PlannerView                  *view);
static void       gantt_view_gantt_resource_clicked_cb    (PlannerGanttChart            *chart,
							   MrpResource                  *resource,
							   PlannerView                  *view);
static void       gantt_view_update_ui                    (PlannerView                  *view);
void              activate                                (PlannerView                  *view);
void              deactivate                              (PlannerView                  *view);
void              init                                    (PlannerView                  *view,
							   PlannerWindow                *main_window);
const gchar  *    get_label                               (PlannerView                  *view);
const gchar  *    get_menu_label                          (PlannerView                  *view);
const gchar  *    get_icon                                (PlannerView                  *view);
const gchar  *    get_name                                (PlannerView                  *view);
GtkWidget    *    get_widget                              (PlannerView                  *view);
void              print_init                              (PlannerView                  *view,
							   PlannerPrintJob              *job);
void              print                                   (PlannerView                  *view);
gint              print_get_n_pages                       (PlannerView                  *view);
void              print_cleanup                           (PlannerView                  *view);


static BonoboUIVerb verbs[] = {
	BONOBO_UI_VERB ("InsertTask",		gantt_view_insert_task_cb),
	BONOBO_UI_VERB ("InsertTasks",		gantt_view_insert_tasks_cb),
	BONOBO_UI_VERB ("RemoveTask",		gantt_view_remove_task_cb),
	BONOBO_UI_VERB ("EditTask",	        gantt_view_edit_task_cb),
	BONOBO_UI_VERB ("SelectAll",		gantt_view_select_all_cb),
	BONOBO_UI_VERB ("UnlinkTask",		gantt_view_unlink_task_cb),
	BONOBO_UI_VERB ("LinkTasks",		gantt_view_link_tasks_cb),
	BONOBO_UI_VERB ("IndentTask",		gantt_view_indent_task_cb),
	BONOBO_UI_VERB ("UnindentTask",		gantt_view_unindent_task_cb),
	BONOBO_UI_VERB ("MoveTaskUp",           gantt_view_move_task_up_cb),
	BONOBO_UI_VERB ("MoveTaskDown",	        gantt_view_move_task_down_cb),
	BONOBO_UI_VERB ("ResetConstraint",	gantt_view_reset_constraint_cb),
	BONOBO_UI_VERB ("ZoomToFit",		gantt_view_zoom_to_fit_cb),
	BONOBO_UI_VERB ("ZoomIn",		gantt_view_zoom_in_cb),
	BONOBO_UI_VERB ("ZoomOut",		gantt_view_zoom_out_cb),
	BONOBO_UI_VERB ("Test",			gantt_view_test_cb),

	BONOBO_UI_VERB_END
};

G_MODULE_EXPORT void
activate (PlannerView *view)
{
	PlannerViewPriv *priv;
	gboolean         show_critical;

	priv = view->priv;
	
	planner_view_activate_helper (view,
				      DATADIR
				      "/planner/ui/gantt-view.ui",
				      "ganttview",
				      verbs);
	
	/* Set the initial UI state. */
	
	show_critical = planner_gantt_chart_get_highlight_critical_tasks (
		PLANNER_GANTT_CHART (priv->gantt));
	
	planner_task_tree_set_highlight_critical (PLANNER_TASK_TREE (priv->tree),
						  show_critical);
	
	bonobo_ui_component_set_prop (view->ui_component, 
				      "/commands/HighlightCriticalTasks",
				      "state", show_critical ? "1" : "0",
				      NULL);
	
	gantt_view_selection_changed_cb (PLANNER_TASK_TREE (priv->tree), view);
	gantt_view_update_zoom_sensitivity (view);
}

G_MODULE_EXPORT void
deactivate (PlannerView *view)
{
	planner_view_deactivate_helper (view);
}

G_MODULE_EXPORT void
init (PlannerView *view, PlannerWindow *main_window)
{
	PlannerViewPriv     *priv;
	GtkIconFactory *icon_factory;
	GtkIconSet     *icon_set;
	GdkPixbuf      *pixbuf;
	
	priv = g_new0 (PlannerViewPriv, 1);
	view->priv = priv;

	icon_factory = gtk_icon_factory_new ();
	gtk_icon_factory_add_default (icon_factory);

	pixbuf = gdk_pixbuf_new_from_file (IMAGEDIR "/24_insert_task.png", NULL);
	icon_set = gtk_icon_set_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);
	gtk_icon_factory_add (icon_factory,
			      "planner-stock-insert-task",
			      icon_set);
	
	pixbuf = gdk_pixbuf_new_from_file (IMAGEDIR "/24_remove_task.png", NULL);
	icon_set = gtk_icon_set_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);
	gtk_icon_factory_add (icon_factory,
			      "planner-stock-remove-task",
			      icon_set);

	pixbuf = gdk_pixbuf_new_from_file (IMAGEDIR "/24_unlink_task.png", NULL);
	icon_set = gtk_icon_set_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);
	gtk_icon_factory_add (icon_factory,
			      "planner-stock-unlink-task",
			      icon_set);

	pixbuf = gdk_pixbuf_new_from_file (IMAGEDIR "/24_link_task.png", NULL);
	icon_set = gtk_icon_set_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);
	gtk_icon_factory_add (icon_factory,
			      "planner-stock-link-task",
			      icon_set);

	pixbuf = gdk_pixbuf_new_from_file (IMAGEDIR "/24_indent_task.png", NULL);
	icon_set = gtk_icon_set_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);
	gtk_icon_factory_add (icon_factory,
			      "planner-stock-indent-task",
			      icon_set);

	pixbuf = gdk_pixbuf_new_from_file (IMAGEDIR "/24_unindent_task.png", NULL);
	icon_set = gtk_icon_set_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);
	gtk_icon_factory_add (icon_factory,
			      "planner-stock-unindent-task",
			      icon_set);

	pixbuf = gdk_pixbuf_new_from_file (IMAGEDIR "/24_unlink_task.png", NULL);
	icon_set = gtk_icon_set_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);
	gtk_icon_factory_add (icon_factory,
			      "planner-stock-up-task",
			      icon_set);

	g_signal_connect (view->ui_component,
			  "ui_event",
			  G_CALLBACK (gantt_view_ui_component_event),
			  view);
}

G_MODULE_EXPORT const gchar *
get_label (PlannerView *view)
{
	g_return_val_if_fail (PLANNER_IS_VIEW (view), NULL);

	return _("Gantt Chart");
}

G_MODULE_EXPORT const gchar *
get_menu_label (PlannerView *view)
{
	g_return_val_if_fail (PLANNER_IS_VIEW (view), NULL);

	return _("_Gantt Chart");
}

G_MODULE_EXPORT const gchar *
get_icon (PlannerView *view)
{
	g_return_val_if_fail (PLANNER_IS_VIEW (view), NULL);

	return IMAGEDIR "/gantt.png";
}

G_MODULE_EXPORT const gchar *
get_name (PlannerView *view)
{
	g_return_val_if_fail (PLANNER_IS_VIEW (view), NULL);

	return "gantt_view";
}

G_MODULE_EXPORT GtkWidget *
get_widget (PlannerView *view)
{
	PlannerViewPriv *priv;

	g_return_val_if_fail (PLANNER_IS_VIEW (view), NULL);

	priv = view->priv;
	
 	if (priv->paned == NULL) {
		priv->paned = gantt_view_create_widget (view);
		gtk_widget_show_all (priv->paned);
	}

	return view->priv->paned;
}

G_MODULE_EXPORT void
print_init (PlannerView     *view,
	    PlannerPrintJob *job)
{
	PlannerViewPriv *priv;
	gdouble          zoom;
	gboolean         show_critical;
	
	g_return_if_fail (PLANNER_IS_VIEW (view));
	g_return_if_fail (PLANNER_IS_PRINT_JOB (job));

	priv = view->priv;
	
	g_assert (priv->print_data == NULL);

	zoom = planner_gantt_chart_get_zoom (PLANNER_GANTT_CHART (view->priv->gantt));

	show_critical = planner_gantt_chart_get_highlight_critical_tasks (
		PLANNER_GANTT_CHART (view->priv->gantt));
	
	priv->print_data = planner_gantt_print_data_new (view, job,
							 GTK_TREE_VIEW (priv->tree),
							 zoom, show_critical);
}

G_MODULE_EXPORT void
print (PlannerView *view)

{
	g_return_if_fail (PLANNER_IS_VIEW (view));

	g_assert (view->priv->print_data);
	
	planner_gantt_print_do (view->priv->print_data);
}

G_MODULE_EXPORT gint
print_get_n_pages (PlannerView *view)
{
	g_return_val_if_fail (PLANNER_IS_VIEW (view), 0);

	g_assert (view->priv->print_data);
	
	return planner_gantt_print_get_n_pages (view->priv->print_data);
}

G_MODULE_EXPORT void
print_cleanup (PlannerView *view)

{
	g_return_if_fail (PLANNER_IS_VIEW (view));

	g_assert (view->priv->print_data);
	
	planner_gantt_print_data_free (view->priv->print_data);
	view->priv->print_data = NULL;
}

static void
gantt_view_row_expanded (GtkTreeView *tree_view,
			 GtkTreeIter *iter,
			 GtkTreePath *path,
			 gpointer     data)
{
	PlannerGanttChart *gantt = data;

	planner_gantt_chart_expand_row (gantt, path);
}

static void
gantt_view_row_collapsed (GtkTreeView *tree_view,
			  GtkTreeIter *iter,
			  GtkTreePath *path,
			  gpointer     data)
{
	PlannerGanttChart *gantt = data;

	planner_gantt_chart_collapse_row (gantt, path);
}

static void
gantt_view_project_loaded_cb (MrpProject  *project,
			      PlannerView *view)
{
	GtkTreeModel *model;

	model = GTK_TREE_MODEL (planner_gantt_model_new (project));

	planner_task_tree_set_model (PLANNER_TASK_TREE (view->priv->tree),
				     PLANNER_GANTT_MODEL (model));
	
	planner_gantt_chart_set_model (PLANNER_GANTT_CHART (view->priv->gantt),
				       model);
	
	g_object_unref (model);
}

static void
gantt_view_tree_view_size_request_cb (GtkWidget      *widget,
				      GtkRequisition *req,
				      gpointer        data)
{
	req->height = -1;
}

static gboolean
gantt_view_tree_view_scroll_event_cb (GtkWidget      *widget,
				      GdkEventScroll *event,
				      gpointer        data)
{
	GtkAdjustment *adj;
	GtkTreeView   *tv = GTK_TREE_VIEW (widget);
	gdouble        new_value;

	if (event->direction != GDK_SCROLL_UP &&
	    event->direction != GDK_SCROLL_DOWN) {
		return FALSE;
	}

	adj = gtk_tree_view_get_vadjustment (tv);
	
	if (event->direction == GDK_SCROLL_UP) {
		new_value = adj->value - adj->page_increment / 2;
	} else {
		new_value = adj->value + adj->page_increment / 2;
	}
	
	new_value = CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	gtk_adjustment_set_value (adj, new_value);
		
	return TRUE;
}

static GtkWidget *
gantt_view_create_widget (PlannerView *view)
{
	PlannerViewPriv  *priv;
	GtkWidget        *tree;
	GtkWidget        *sw;
	GtkWidget        *frame;
	GtkWidget        *vbox;
	GtkWidget        *hpaned;
	GtkAdjustment    *hadj, *vadj;
	GtkTreeModel     *model;
	MrpProject       *project;
	GtkTreeSelection *selection;
	
	project = planner_window_get_project (view->main_window);

	priv = view->priv;
	
	g_signal_connect (project,
			  "loaded",
			  G_CALLBACK (gantt_view_project_loaded_cb),
			  view);

	model = GTK_TREE_MODEL (planner_gantt_model_new (project));

	tree = planner_task_tree_new (view->main_window,
				      PLANNER_GANTT_MODEL (model),
				      FALSE,
				      /* i18n: WBS is short for work breakdown structure, and is a
				       * project management term. You might want to leave it
				       * untranslated unless there is a localized term for it.
				       */
				      COL_WBS,        _("\nWBS"),
				      COL_NAME,       _("\nName"),
				      /*COL_START,    _("\nStart"),*/
				      COL_WORK,       _("\nWork"), 
				      /*COL_DURATION, _("\nDuration"),*/
				      -1);
	
	priv->tree = tree;
	priv->gantt = planner_gantt_chart_new_with_model (model);
	planner_gantt_chart_set_view (PLANNER_GANTT_CHART (priv->gantt),
				      PLANNER_TASK_TREE (tree));
	
	g_object_unref (model);

	g_signal_connect (priv->gantt,
			  "status_updated",
			  G_CALLBACK (gantt_view_gantt_status_updated_cb),
			  view);

	g_signal_connect (priv->gantt,
			  "resource_clicked",
			  G_CALLBACK (gantt_view_gantt_resource_clicked_cb),
			  view);
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

	g_signal_connect (tree,
			  "style_set",
			  G_CALLBACK (gantt_view_tree_style_set_cb),
			  view);

	g_signal_connect (tree,
			  "selection_changed",
			  G_CALLBACK (gantt_view_selection_changed_cb),
			  view);
	
	g_signal_connect (tree,
			  "relation_added",
			  G_CALLBACK (gantt_view_relations_changed_cb),
			  view);

	g_signal_connect (tree,
			  "relation_removed",
			  G_CALLBACK (gantt_view_relations_changed_cb),
			  view);
	
	vbox = gtk_vbox_new (FALSE, 3);
	
	gtk_box_pack_start (GTK_BOX (vbox), tree, TRUE, TRUE, 0);

	hadj = gtk_tree_view_get_hadjustment (GTK_TREE_VIEW (tree));
	gtk_box_pack_start (GTK_BOX (vbox), gtk_hscrollbar_new (hadj), FALSE, TRUE, 0);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (frame), vbox);

	hpaned = gtk_hpaned_new ();
	gtk_paned_add1 (GTK_PANED (hpaned), frame);

	g_signal_connect_after (tree,
				"size_request",
				G_CALLBACK (gantt_view_tree_view_size_request_cb),
				NULL);

	g_signal_connect_after (tree,
				"scroll_event",
				G_CALLBACK (gantt_view_tree_view_scroll_event_cb),
				view);

	hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 0, 90, 250, 2000));
	vadj = gtk_tree_view_get_vadjustment (GTK_TREE_VIEW (tree));

	sw = gtk_scrolled_window_new (hadj, vadj);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_ALWAYS,
					GTK_POLICY_AUTOMATIC);
  
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (frame), sw);
	
	gtk_container_add (GTK_CONTAINER (sw),
			   GTK_WIDGET (priv->gantt));

	gtk_paned_add2 (GTK_PANED (hpaned), frame);

	gtk_paned_set_position (GTK_PANED (hpaned), 250);
		
	g_signal_connect (tree,
			  "row_expanded",
			  G_CALLBACK (gantt_view_row_expanded),
			  priv->gantt);
	
	g_signal_connect (tree,
			  "row_collapsed",
			  G_CALLBACK (gantt_view_row_collapsed),
			  priv->gantt);

	gtk_tree_view_expand_all (GTK_TREE_VIEW (tree));

	return hpaned;
}

/* Command callbacks. */

static void
gantt_view_insert_task_cb (BonoboUIComponent *component, 
			   gpointer           data, 
			   const char        *cname)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_insert_task (PLANNER_TASK_TREE (view->priv->tree));
}

static void
gantt_view_insert_tasks_cb (BonoboUIComponent *component, 
			    gpointer           data, 
			    const char        *cname)
{
	PlannerView *view = PLANNER_VIEW (data);

	planner_task_tree_insert_tasks (PLANNER_TASK_TREE (view->priv->tree));
}

static void
gantt_view_remove_task_cb (BonoboUIComponent *component, 
			   gpointer           data, 
			   const char        *cname)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_remove_task (PLANNER_TASK_TREE (view->priv->tree));
}

static void
gantt_view_select_all_cb (BonoboUIComponent *component, 
			  gpointer           data, 
			  const char        *cname)
{
	PlannerView *view;
	
	view = PLANNER_VIEW (data);
	
	planner_task_tree_select_all (PLANNER_TASK_TREE (view->priv->tree));
}

static void
gantt_view_edit_task_cb (BonoboUIComponent *component, 
			 gpointer           data, 
			 const char        *cname)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_edit_task (PLANNER_TASK_TREE (view->priv->tree),
				     PLANNER_TASK_DIALOG_PAGE_GENERAL);
}

static void
gantt_view_unlink_task_cb (BonoboUIComponent *component, 
			   gpointer           data, 
			   const char        *cname)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_unlink_task (PLANNER_TASK_TREE (view->priv->tree));
}

static void
gantt_view_link_tasks_cb (BonoboUIComponent *component,
			  gpointer           data,
			  const char        *cname)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_link_tasks (PLANNER_TASK_TREE (view->priv->tree),
				      MRP_RELATION_FS);
}

static void
gantt_view_indent_task_cb (BonoboUIComponent *component, 
			   gpointer           data, 
			   const char        *cname)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_indent_task (PLANNER_TASK_TREE (view->priv->tree));
}

static void
gantt_view_unindent_task_cb (BonoboUIComponent *component, 
			     gpointer           data, 
			     const char        *cname)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_unindent_task (PLANNER_TASK_TREE (view->priv->tree));
}

static void
gantt_view_move_task_up_cb (BonoboUIComponent  *component,
			    gpointer            data,
			    const char 	*cname)
{
	PlannerView *view;
	
	view = PLANNER_VIEW (data);
	
	planner_task_tree_move_task_up (PLANNER_TASK_TREE (view->priv->tree));
}

static void 
gantt_view_move_task_down_cb (BonoboUIComponent *component,
			      gpointer	          data,
			      const char	 *cname)
{
	PlannerView *view;
	
	view = PLANNER_VIEW (data);
	
	planner_task_tree_move_task_down (PLANNER_TASK_TREE (view->priv->tree));
}

static void
gantt_view_reset_constraint_cb (BonoboUIComponent *component, 
				gpointer           data, 
				const char        *cname)
{
	PlannerView *view;

	view = PLANNER_VIEW (data);

	planner_task_tree_reset_constraint (PLANNER_TASK_TREE (view->priv->tree));
}

static void
gantt_view_zoom_to_fit_cb (BonoboUIComponent *component, 
			   gpointer           data, 
			   const char        *cname)
{
	PlannerView     *view;
	PlannerViewPriv *priv;
	
	view = PLANNER_VIEW (data);
	priv = view->priv;

	planner_gantt_chart_zoom_to_fit (PLANNER_GANTT_CHART (priv->gantt));

	gantt_view_update_zoom_sensitivity (view);
}

static void
gantt_view_zoom_in_cb (BonoboUIComponent *component, 
		       gpointer           data, 
		       const char        *cname)
{
	PlannerView     *view;
	PlannerViewPriv *priv;
	
	view = PLANNER_VIEW (data);
	priv = view->priv;

	planner_gantt_chart_zoom_in (PLANNER_GANTT_CHART (priv->gantt));

	gantt_view_update_zoom_sensitivity (view);
}

static void
gantt_view_zoom_out_cb (BonoboUIComponent *component, 
			gpointer           data, 
			const char        *cname)
{
	PlannerView     *view;
	PlannerViewPriv *priv;
	
	view = PLANNER_VIEW (data);
	priv = view->priv;

	planner_gantt_chart_zoom_out (PLANNER_GANTT_CHART (priv->gantt));

	gantt_view_update_zoom_sensitivity (view);
}

static void
gantt_view_ui_component_event (BonoboUIComponent            *comp,
			       const gchar                  *path,
			       Bonobo_UIComponent_EventType  type,
			       const gchar                  *state_string,
			       PlannerView                  *view)
{
	PlannerViewPriv *priv;
	gboolean         state;
	
	priv = view->priv;

	if (!strcmp (path, "HighlightCriticalTasks")) {
		state = !strcmp (state_string, "1");

		planner_gantt_chart_set_highlight_critical_tasks (
			PLANNER_GANTT_CHART (priv->gantt),
			state);

		planner_task_tree_set_highlight_critical (PLANNER_TASK_TREE (priv->tree),
							  state);
	}
}
	
static void
gantt_view_test_cb (BonoboUIComponent *component, 
		    gpointer           data, 
		    const char        *cname)
{
	PlannerView       *view;
	PlannerViewPriv   *priv;
	PlannerGanttModel *model;
	MrpTask           *task;
	MrpProject        *project;
	GList             *list;
	
	view = PLANNER_VIEW (data);
	priv = view->priv;
	project = planner_window_get_project (view->main_window);

	model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (
					     GTK_TREE_VIEW (priv->tree)));
	
	list = planner_task_tree_get_selected_tasks (PLANNER_TASK_TREE (priv->tree));
	if (list == NULL) {
		return;
	}

	task = list->data;

	g_list_free (list);
}

static void
gantt_view_update_row_and_header_height (PlannerView *view)
{
	GtkTreeView       *tv = GTK_TREE_VIEW (view->priv->tree);
	GtkWidget         *gantt = view->priv->gantt;
	gint               row_height;
	gint               header_height;
	gint               height;
	GList             *cols, *l;
	GtkTreeViewColumn *col;
	GtkRequisition     req;

	/* Get the row and header heights. */
	cols = gtk_tree_view_get_columns (tv);
	row_height = 0;
	header_height = 0;

	for (l = cols; l; l = l->next) {
		col = l->data;

		gtk_widget_size_request (col->button, &req);
		header_height = MAX (header_height, req.height);

		gtk_tree_view_column_cell_get_size (col,
						    NULL,
						    NULL,
						    NULL,
						    NULL,
						    &height);
		row_height = MAX (row_height, height);
	}

	/* Sync with the gantt widget. */
	g_object_set (gantt,
		      "header_height", header_height,
		      "row_height", row_height,
		      NULL);
}

static gboolean
idle_update_heights (PlannerView *view)
{
	gantt_view_update_row_and_header_height (view);
	return FALSE;
}

static void
gantt_view_tree_style_set_cb (GtkWidget   *tree,
			      GtkStyle    *prev_style,
			      PlannerView *view)
{
	if (prev_style) {
		g_idle_add ((GSourceFunc) idle_update_heights, view);
	} else {
		idle_update_heights (view);
	}
}

static void 
gantt_view_selection_changed_cb (PlannerTaskTree *tree, PlannerView *view)
{
	g_return_if_fail (PLANNER_IS_VIEW (view));

	gantt_view_update_ui (view);
}

static void
gantt_view_gantt_status_updated_cb (PlannerGanttChart *gantt,
				    const gchar       *message,
				    PlannerView       *view)
{
	bonobo_ui_component_set_status (view->ui_component,
					message,
					NULL);
}

static void
gantt_view_gantt_resource_clicked_cb (PlannerGanttChart *chart,
				      MrpResource       *resource,
				      PlannerView       *view)
{
	GtkWidget *dialog;

	dialog = planner_resource_dialog_new (view->main_window, resource);
	gtk_widget_show (dialog);
}
	
static void
gantt_view_relations_changed_cb (PlannerTaskTree *tree,
				 MrpTask         *task,
				 MrpRelation     *relation,
				 PlannerView     *view)
{
	gantt_view_update_ui (view);
}

static void
gantt_view_update_ui (PlannerView *view)
{
	PlannerViewPriv *priv;
	GList           *list, *l;
	gchar           *value;
	gchar           *rel_value = "0";
	gchar           *link_value = "0";
	gint             count_value = 0;

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

static void
gantt_view_update_zoom_sensitivity (PlannerView *view)
{
	gboolean in, out;

	planner_gantt_chart_can_zoom (PLANNER_GANTT_CHART (view->priv->gantt),
				      &in,
				      &out);

	bonobo_ui_component_freeze (view->ui_component, NULL);
	
	bonobo_ui_component_set_prop (view->ui_component, 
				      "/commands/ZoomIn",
				      "sensitive", in ? "1" : "0", 
				      NULL);

	bonobo_ui_component_set_prop (view->ui_component, 
				      "/commands/ZoomOut",
				      "sensitive", out ? "1" : "0", 
				      NULL);
	
	bonobo_ui_component_thaw (view->ui_component, NULL);
}
