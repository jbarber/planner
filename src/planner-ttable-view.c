/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 * Copyright (C) 2003-2004 Imendio HB
 * Copyright (C) 2003 Benjamin BAYART <benjamin@sitadelle.com>
 * Copyright (C) 2003 Xavier Ordoquy <xordoquy@wanadoo.fr>
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
#include <glib.h>
#include <gmodule.h>
#include <gtk/gtk.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkhpaned.h>
#include <libgnome/gnome-i18n.h>
#include <libplanner/mrp-task.h>
#include <libplanner/mrp-resource.h>
#include "planner-view.h"
#include "planner-ttable-print.h"
#include "planner-ttable-model.h"
#include "planner-ttable-tree.h"
#include "planner-ttable-chart.h"

struct _PlannerViewPriv {
        GtkWidget              *paned;
        GtkWidget              *tree;
        GtkWidget              *gantt;
        MrpProject             *project;

        PlannerTtableChart     *chart;
        PlannerTtablePrintData *print_data;
};

static void       ttable_view_zoom_out_cb             (BonoboUIComponent            *component,
						       gpointer                      data,
						       const char                   *cname);
static void       ttable_view_zoom_in_cb              (BonoboUIComponent            *component,
						       gpointer                      data,
						       const char                   *cname);
static void       ttable_view_zoom_to_fit_cb          (BonoboUIComponent            *component,
						       gpointer                      data,
						       const char                   *cname);
static void       ttable_view_ui_component_event      (BonoboUIComponent            *component,
						       const gchar                  *path,
						       Bonobo_UIComponent_EventType  type,
						       const gchar                  *state_string,
						       PlannerView                  *view);
static GtkWidget *ttable_view_create_widget           (PlannerView                  *view);
static void       ttable_view_project_loaded_cb       (MrpProject                   *project,
						       PlannerView                  *view);
static void       ttable_view_tree_view_realize_cb    (GtkWidget                    *w,
						       gpointer                      data);
static void       ttable_view_row_expanded            (GtkTreeView                  *tree_view,
						       GtkTreeIter                  *iter,
						       GtkTreePath                  *path,
						       gpointer                      data);
static void       ttable_view_row_collapsed           (GtkTreeView                  *tree_view,
						       GtkTreeIter                  *iter,
						       GtkTreePath                  *path,
						       gpointer                      data);
static void       ttable_view_expand_all              (PlannerTtableTree            *tree,
						       PlannerTtableChart           *chart);
static void       ttable_view_collapse_all            (PlannerTtableTree            *tree,
						       PlannerTtableChart           *chart);
static void       ttable_view_ttable_status_updated   (PlannerTtableChart           *chart,
						       const gchar                  *message,
						       PlannerView                  *view);
static void       ttable_view_update_zoom_sensitivity (PlannerView                  *view);
void              activate                            (PlannerView                  *view);
void              deactivate                          (PlannerView                  *view);
void              init                                (PlannerView                  *view,
						       PlannerWindow                *window);
gchar *          get_label                           (PlannerView                  *view);
gchar *          get_menu_label                      (PlannerView                  *view);
gchar *          get_icon                            (PlannerView                  *view);
const gchar *    get_name                            (PlannerView                  *view);
GtkWidget *      get_widget                          (PlannerView                  *view);
void              print_init                          (PlannerView                  *view,
						       PlannerPrintJob              *job);
void              print                               (PlannerView                  *view);
gint              print_get_n_pages                   (PlannerView                  *view);
void              print_cleanup                       (PlannerView                  *view);



static BonoboUIVerb verbs[] = {
        BONOBO_UI_VERB ("ZoomOut", ttable_view_zoom_out_cb),
        BONOBO_UI_VERB ("ZoomIn", ttable_view_zoom_in_cb),
        BONOBO_UI_VERB ("ZoomToFit", ttable_view_zoom_to_fit_cb),
        BONOBO_UI_VERB_END
};


G_MODULE_EXPORT void
activate (PlannerView *view)
{
        PlannerViewPriv *priv;

        priv = view->priv;
        planner_view_activate_helper (view,
                                      DATADIR
                                      "/planner/ui/time-table-view.ui",
                                      "timetableview", verbs);

        ttable_view_update_zoom_sensitivity (view);
}

G_MODULE_EXPORT void
deactivate (PlannerView *view)
{
        planner_view_deactivate_helper (view);
}

G_MODULE_EXPORT void
init (PlannerView *view, PlannerWindow *window)
{
        PlannerViewPriv *priv;

        priv = g_new0 (PlannerViewPriv, 1);

        view->priv = priv;

        g_signal_connect (view->ui_component,
                          "ui-event",
                          G_CALLBACK (ttable_view_ui_component_event), view);
}

G_MODULE_EXPORT gchar *
get_label (PlannerView *view)
{
        g_return_val_if_fail (PLANNER_IS_VIEW (view), NULL);
        return _("Resource Usage");
}

G_MODULE_EXPORT gchar *
get_menu_label (PlannerView *view)
{
        g_return_val_if_fail (PLANNER_IS_VIEW (view), NULL);
        return _("Resource _Usage");
}

G_MODULE_EXPORT gchar *
get_icon (PlannerView *view)
{
        g_return_val_if_fail (PLANNER_IS_VIEW (view), NULL);
        return IMAGEDIR "/resources_usage.png";
}

G_MODULE_EXPORT const gchar *
get_name (PlannerView *view)
{
        g_return_val_if_fail (PLANNER_IS_VIEW (view), NULL);

        return "resource_usage_view";
}

G_MODULE_EXPORT GtkWidget *
get_widget (PlannerView *view)
{
        PlannerViewPriv *priv;

        g_return_val_if_fail (PLANNER_IS_VIEW (view), NULL);

        priv = view->priv;
        if (priv->paned == NULL) {
                priv->paned = ttable_view_create_widget (view);
                gtk_widget_show_all (priv->paned);
        }

        return view->priv->paned;
}

G_MODULE_EXPORT void
print_init (PlannerView *view, PlannerPrintJob *job)
{
        PlannerViewPriv *priv;

        g_return_if_fail (PLANNER_IS_VIEW (view));
        g_return_if_fail (PLANNER_IS_PRINT_JOB (job));

        priv = view->priv;

        g_assert (priv->print_data == NULL);

        priv->print_data = planner_ttable_print_data_new (view, job);
}

G_MODULE_EXPORT void
print (PlannerView *view)
{
        g_return_if_fail (PLANNER_IS_VIEW (view));

        g_assert (view->priv->print_data);

        planner_ttable_print_do (view->priv->print_data);
}

G_MODULE_EXPORT gint
print_get_n_pages (PlannerView *view)
{
        g_return_val_if_fail (PLANNER_IS_VIEW (view), 0);

        g_assert (view->priv->print_data);

        return planner_ttable_print_get_n_pages (view->priv->print_data);
}

G_MODULE_EXPORT void
print_cleanup (PlannerView *view)
{
        g_return_if_fail (PLANNER_IS_VIEW (view));

        g_assert (view->priv->print_data);

        planner_ttable_print_data_free (view->priv->print_data);

        view->priv->print_data = NULL;
}

static void
ttable_view_zoom_out_cb (BonoboUIComponent *component,
                         gpointer           data,
			 const char        *cname)
{
        PlannerView *view;

        view = PLANNER_VIEW (data);

        planner_ttable_chart_zoom_out (view->priv->chart);
        ttable_view_update_zoom_sensitivity (view);
}

static void
ttable_view_zoom_in_cb (BonoboUIComponent *component,
                        gpointer           data,
			const char        *cname)
{
        PlannerView *view;

        view = PLANNER_VIEW (data);

        planner_ttable_chart_zoom_in (view->priv->chart);
        ttable_view_update_zoom_sensitivity (view);
}

static void
ttable_view_zoom_to_fit_cb (BonoboUIComponent *component,
                            gpointer           data,
			    const char        *cname)
{
        PlannerView *view;

        view = PLANNER_VIEW (data);

        planner_ttable_chart_zoom_to_fit (view->priv->chart);
        ttable_view_update_zoom_sensitivity (view);
}

static void
ttable_view_ui_component_event (BonoboUIComponent            *component,
                                const gchar                  *path,
                                Bonobo_UIComponent_EventType  type,
                                const gchar                  *state_string,
                                PlannerView                  *view)
{
        PlannerViewPriv *priv;

        priv = view->priv;
}

static void
ttable_view_tree_view_size_request_cb (GtkWidget      *widget,
                                       GtkRequisition *req,
				       gpointer        data)
{
        req->height = -1;
}

static gboolean
ttable_view_tree_view_scroll_event_cb (GtkWidget      *widget,
                                       GdkEventScroll *event,
				       gpointer        data)
{
        GtkAdjustment *adj;
        GtkTreeView *tv = GTK_TREE_VIEW (widget);
        gdouble new_value;
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
        new_value =
                CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
        gtk_adjustment_set_value (adj, new_value);
        return TRUE;
}

static GtkWidget *
ttable_view_create_widget (PlannerView *view)
{
        PlannerViewPriv    *priv;
        MrpProject         *project;
        GtkWidget          *hpaned;
        GtkWidget          *left_frame;
        GtkWidget          *right_frame;
        PlannerTtableModel *model;
        GtkWidget          *tree;
        GtkWidget          *vbox;
        GtkWidget          *sw;
        GtkWidget          *chart;
	GtkAdjustment      *hadj, *vadj;

        project = planner_window_get_project (view->main_window);
        priv = view->priv;
        priv->project = project;

        g_signal_connect (project,
                          "loaded",
                          G_CALLBACK (ttable_view_project_loaded_cb),
			  view);

        model = planner_ttable_model_new (project);
        tree = planner_ttable_tree_new (view->main_window, model);
        priv->tree = tree;
        left_frame = gtk_frame_new (NULL);
        right_frame = gtk_frame_new (NULL);

        vbox = gtk_vbox_new (FALSE, 3);
        gtk_box_pack_start (GTK_BOX (vbox), tree, TRUE, TRUE, 0);
        hadj = gtk_tree_view_get_hadjustment (GTK_TREE_VIEW (tree));
        gtk_box_pack_start (GTK_BOX (vbox), gtk_hscrollbar_new (hadj), FALSE,
                            TRUE, 0);
        gtk_container_add (GTK_CONTAINER (left_frame), vbox);

        hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 0, 90, 250, 2000));
        vadj = gtk_tree_view_get_vadjustment (GTK_TREE_VIEW (tree));

        chart = planner_ttable_chart_new_with_model (GTK_TREE_MODEL (model));
        priv->chart = PLANNER_TTABLE_CHART (chart);
        sw = gtk_scrolled_window_new (hadj, vadj);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                        GTK_POLICY_ALWAYS,
                                        GTK_POLICY_AUTOMATIC);
        gtk_container_add (GTK_CONTAINER (right_frame), sw);
        gtk_container_add (GTK_CONTAINER (sw), chart);

        hpaned = gtk_hpaned_new ();
        gtk_frame_set_shadow_type (GTK_FRAME (left_frame), GTK_SHADOW_IN);
        gtk_frame_set_shadow_type (GTK_FRAME (right_frame), GTK_SHADOW_IN);
        gtk_paned_add1 (GTK_PANED (hpaned), left_frame);
        gtk_paned_add2 (GTK_PANED (hpaned), right_frame);

        g_signal_connect (tree, "realize",
                          G_CALLBACK (ttable_view_tree_view_realize_cb),
                          chart);
        g_signal_connect (tree, "row_expanded",
                          G_CALLBACK (ttable_view_row_expanded), chart);
        g_signal_connect (tree, "row_collapsed",
                          G_CALLBACK (ttable_view_row_collapsed), chart);
        g_signal_connect (tree, "expand_all",
                          G_CALLBACK (ttable_view_expand_all), chart);
        g_signal_connect (tree, "collapse_all",
                          G_CALLBACK (ttable_view_collapse_all), chart);
        g_signal_connect (chart, "status_updated",
                          G_CALLBACK (ttable_view_ttable_status_updated),
                          view);
        g_signal_connect_after (tree, "size_request",
                                G_CALLBACK
                                (ttable_view_tree_view_size_request_cb),
                                NULL);
        g_signal_connect_after (tree, "scroll_event",
                                G_CALLBACK
                                (ttable_view_tree_view_scroll_event_cb),
                                view);

        gtk_tree_view_expand_all (GTK_TREE_VIEW (tree));

	planner_ttable_chart_expand_all (PLANNER_TTABLE_CHART (chart));

	g_object_unref (model);

        return hpaned;
}

static void
ttable_view_ttable_status_updated (PlannerTtableChart *chart,
                                   const gchar        *message,
				   PlannerView        *view)
{
        bonobo_ui_component_set_status (view->ui_component, message, NULL);
}

static void
ttable_view_project_loaded_cb (MrpProject *project, PlannerView *view)
{
        GtkTreeModel *model;

	/* FIXME: This is not working so well. Look at how the gantt view
	 * handles this. (The crux is that the root task for example might
	 * change when a project is loaded, so we need to rconnect signals
	 * etc.)
	 */
        if (project == view->priv->project) {
		/* FIXME: Due to the above, we have this hack. */
		planner_ttable_chart_setup_root_task (view->priv->chart);
		
                gtk_tree_view_expand_all (GTK_TREE_VIEW (view->priv->tree));
                planner_ttable_chart_expand_all (view->priv->chart);
                return;
        }

	model = GTK_TREE_MODEL (planner_ttable_model_new (project));

	planner_ttable_tree_set_model (PLANNER_TTABLE_TREE (view->priv->tree),
                                       PLANNER_TTABLE_MODEL (model));
        planner_ttable_chart_set_model (PLANNER_TTABLE_CHART
                                        (view->priv->chart), model);

	g_object_unref (model);

	gtk_tree_view_expand_all (GTK_TREE_VIEW (view->priv->tree));
        planner_ttable_chart_expand_all (view->priv->chart);
}

static void
ttable_view_tree_view_realize_cb (GtkWidget *w, gpointer data)
{
        GtkTreeView       *tv = GTK_TREE_VIEW (w);
        GtkWidget         *chart = data;
        gint               row_height;
        gint               header_height;
        gint               height;
        GList             *cols, *l;
        GtkTreeViewColumn *col;
        GtkRequisition     req;

        cols = gtk_tree_view_get_columns (tv);
        row_height = 0;
        header_height = 0;
        for (l = cols; l; l = l->next) {
                col = l->data;
                gtk_widget_size_request (col->button, &req);
                header_height = MAX (header_height, req.height);
                gtk_tree_view_column_cell_get_size (col, NULL, NULL, NULL,
                                                    NULL, &height);
                row_height = MAX (row_height, height);
        }
        g_object_set (chart,
		      "header_height", header_height,
		      "row_height", row_height,
		      NULL);
}

static void
ttable_view_row_expanded (GtkTreeView *tree_view,
                          GtkTreeIter *iter,
                          GtkTreePath *path,
			  gpointer     data)
{
        PlannerTtableChart *chart = data;

        planner_ttable_chart_expand_row (chart, path);
}

static void
ttable_view_row_collapsed (GtkTreeView *tree_view,
                           GtkTreeIter *iter,
                           GtkTreePath *path,
			   gpointer     data)
{
        PlannerTtableChart *chart = data;

	planner_ttable_chart_collapse_row (chart, path);
}

static void
ttable_view_expand_all (PlannerTtableTree *tree, PlannerTtableChart *chart)
{
        g_signal_handlers_block_by_func (tree,
                                         ttable_view_row_expanded, chart);

        planner_ttable_tree_expand_all (tree);
        planner_ttable_chart_expand_all (chart);

        g_signal_handlers_unblock_by_func (tree,
                                           ttable_view_row_expanded, chart);
}

static void
ttable_view_collapse_all (PlannerTtableTree  *tree,
                          PlannerTtableChart *chart)
{
        g_signal_handlers_block_by_func (tree,
                                         ttable_view_row_collapsed, chart);

        planner_ttable_tree_collapse_all (tree);
        planner_ttable_chart_collapse_all (chart);

        g_signal_handlers_unblock_by_func (tree,
                                           ttable_view_row_collapsed, chart);
}

static void
ttable_view_update_zoom_sensitivity (PlannerView *view)
{
        gboolean in, out;

        planner_ttable_chart_can_zoom (view->priv->chart, &in, &out);

        bonobo_ui_component_freeze (view->ui_component, NULL);

        bonobo_ui_component_set_prop (view->ui_component,
                                      "/commands/ZoomIn",
                                      "sensitive", in ? "1" : "0", NULL);

        bonobo_ui_component_set_prop (view->ui_component,
                                      "/commands/ZoomOut",
                                      "sensitive", out ? "1" : "0", NULL);

        bonobo_ui_component_thaw (view->ui_component, NULL);
}


/*
TODO:
planner_ttable_print_data_new
planner_ttable_print_do
planner_ttable_print_get_n_pages
*/
