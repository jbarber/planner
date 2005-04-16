/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003-2005 Imendio AB
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
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libplanner/mrp-task.h>
#include <libplanner/mrp-resource.h>
#include "planner-usage-view.h"
/*#include "planner-usage-print.h"*/
#include "planner-usage-model.h"
#include "planner-usage-tree.h"
#include "planner-usage-chart.h"

struct _PlannerUsageViewPriv {
        GtkWidget              *paned;
        GtkWidget              *tree;
        MrpProject             *project;

        PlannerUsageChart     *chart;
        /*PlannerUsagePrintData *print_data;*/
	
	GtkUIManager           *ui_manager;
	GtkActionGroup         *actions;
	guint                   merged_id;
};

static void         usage_view_zoom_out_cb             (GtkAction         *action,
							gpointer           data);
static void         usage_view_zoom_in_cb              (GtkAction         *action,
							gpointer           data);
static void         usage_view_zoom_to_fit_cb          (GtkAction         *action,
							gpointer           data);
static GtkWidget *  usage_view_create_widget           (PlannerView       *view);
static void         usage_view_project_loaded_cb       (MrpProject        *project,
							PlannerView       *view);
static void         usage_view_tree_style_set_cb       (GtkWidget         *tree,
							GtkStyle          *prev_style,
							PlannerView       *view);
static void         usage_view_row_expanded            (GtkTreeView       *tree_view,
							GtkTreeIter       *iter,
							GtkTreePath       *path,
							gpointer           data);
static void         usage_view_row_collapsed           (GtkTreeView       *tree_view,
							GtkTreeIter       *iter,
							GtkTreePath       *path,
							gpointer           data);
static void         usage_view_expand_all              (PlannerUsageTree  *tree,
							PlannerUsageChart *chart);
static void         usage_view_collapse_all            (PlannerUsageTree  *tree,
							PlannerUsageChart *chart);
static void         usage_view_usage_status_updated    (PlannerUsageChart *chart,
							const gchar       *message,
							PlannerView       *view);
static void         usage_view_update_zoom_sensitivity (PlannerView       *view);
static void         usage_view_activate                (PlannerView       *view);
static void         usage_view_deactivate              (PlannerView       *view);
static void         usage_view_setup                   (PlannerView       *view,
							PlannerWindow     *window);
static const gchar *usage_view_get_label               (PlannerView       *view);
static const gchar *usage_view_get_menu_label          (PlannerView       *view);
static const gchar *usage_view_get_icon                (PlannerView       *view);
static const gchar *usage_view_get_name                (PlannerView       *view);
static GtkWidget *  usage_view_get_widget              (PlannerView       *view);
static void         usage_view_print_init              (PlannerView       *view,
							PlannerPrintJob   *job);
static void         usage_view_print                   (PlannerView       *view);
static gint         usage_view_print_get_n_pages       (PlannerView       *view);
static void         usage_view_print_cleanup           (PlannerView       *view);




static GtkActionEntry entries[] = {
        { "ZoomOut",   GTK_STOCK_ZOOM_OUT, N_("Zoom out"),
	  NULL, N_("Zoom out"),
	  G_CALLBACK (usage_view_zoom_out_cb) },
        { "ZoomIn",    GTK_STOCK_ZOOM_IN,  N_("Zoom in"),
	  NULL, N_("Zoom in"),
	  G_CALLBACK (usage_view_zoom_in_cb) },
        { "ZoomToFit", GTK_STOCK_ZOOM_FIT, N_("Zoom to fit"),
	  NULL, N_("Zoom to fit the entire project"),
	  G_CALLBACK (usage_view_zoom_to_fit_cb) }
};

G_DEFINE_TYPE (PlannerUsageView, planner_usage_view, PLANNER_TYPE_VIEW);


static void
planner_usage_view_class_init (PlannerUsageViewClass *klass)
{
	PlannerViewClass *view_class;

	view_class = PLANNER_VIEW_CLASS (klass);

	view_class->setup = usage_view_setup;
	view_class->get_label = usage_view_get_label;
	view_class->get_menu_label = usage_view_get_menu_label;
	view_class->get_icon = usage_view_get_icon;
	view_class->get_name = usage_view_get_name;
	view_class->get_widget = usage_view_get_widget;
	view_class->activate = usage_view_activate;
	view_class->deactivate = usage_view_deactivate;
	view_class->print_init = usage_view_print_init;
	view_class->print_get_n_pages = usage_view_print_get_n_pages;
	view_class->print = usage_view_print;
	view_class->print_cleanup = usage_view_print_cleanup;
}

static void
planner_usage_view_init (PlannerUsageView *view)
{
	view->priv = g_new0 (PlannerUsageViewPriv, 1);
}

static void
usage_view_activate (PlannerView *view)
{
	PlannerUsageViewPriv *priv;

	priv = PLANNER_USAGE_VIEW (view)->priv;

	priv->actions = gtk_action_group_new ("TimeTableView");
	gtk_action_group_set_translation_domain (priv->actions, GETTEXT_PACKAGE);

	gtk_action_group_add_actions (priv->actions, entries, G_N_ELEMENTS (entries), view);

	gtk_ui_manager_insert_action_group (priv->ui_manager, priv->actions, 0);
	priv->merged_id = gtk_ui_manager_add_ui_from_file (priv->ui_manager,
							   DATADIR "/planner/ui/time-table-view.ui",
							   NULL);
	gtk_ui_manager_ensure_update (priv->ui_manager);

        usage_view_update_zoom_sensitivity (view);

	gtk_widget_grab_focus (priv->tree);
}

static void
usage_view_deactivate (PlannerView *view)
{
	PlannerUsageViewPriv *priv;

	priv = PLANNER_USAGE_VIEW (view)->priv;
	gtk_ui_manager_remove_ui (priv->ui_manager, priv->merged_id);
}

static void
usage_view_setup (PlannerView *view, PlannerWindow *main_window)
{
        PlannerUsageViewPriv *priv;

        priv = g_new0 (PlannerUsageViewPriv, 1);

        PLANNER_USAGE_VIEW (view)->priv = priv;
	priv->ui_manager = planner_window_get_ui_manager(main_window);
}

static const gchar *
usage_view_get_label (PlannerView *view)
{
	/* i18n: Label used for the sidebar. Please try to make it short and use
	 * a linebreak if necessary/possible.
	 */
        return _("Resource\nUsage");
}

static const gchar *
usage_view_get_menu_label (PlannerView *view)
{
        return _("Resource _Usage");
}

static const gchar *
usage_view_get_icon (PlannerView *view)
{
        return IMAGEDIR "/resources_usage.png";
}

static const gchar *
usage_view_get_name (PlannerView *view)
{
        return "resource_usage_view";
}

static GtkWidget *
usage_view_get_widget (PlannerView *view)
{
        PlannerUsageViewPriv *priv;

        priv = PLANNER_USAGE_VIEW (view)->priv;
        if (priv->paned == NULL) {
                priv->paned = usage_view_create_widget (view);
                gtk_widget_show_all (priv->paned);
        }

        return PLANNER_USAGE_VIEW (view)->priv->paned;
}

static void
usage_view_print_init (PlannerView *view, PlannerPrintJob *job)
{
        PlannerUsageViewPriv *priv;

        priv = PLANNER_USAGE_VIEW (view)->priv;

        /*priv->print_data = planner_usage_print_data_new (view, job);*/
}

static void
usage_view_print (PlannerView *view)
{
        /*planner_usage_print_do (PLANNER_USAGE_VIEW (view)->priv->print_data);*/
}

static gint
usage_view_print_get_n_pages (PlannerView *view)
{
        return 0; /*planner_usage_print_get_n_pages (PLANNER_USAGE_VIEW (view)->priv->print_data);*/
}

static void
usage_view_print_cleanup (PlannerView *view)
{
        /*planner_usage_print_data_free (PLANNER_USAGE_VIEW (view)->priv->print_data);*/
        /*PLANNER_USAGE_VIEW (view)->priv->print_data = NULL;*/
}

static void
usage_view_zoom_out_cb (GtkAction *action,
			gpointer   data)
{
        PlannerView *view;

        view = PLANNER_VIEW (data);

        planner_usage_chart_zoom_out (PLANNER_USAGE_VIEW (view)->priv->chart);
        usage_view_update_zoom_sensitivity (view);
}

static void
usage_view_zoom_in_cb (GtkAction *action,
		       gpointer   data)
{
        PlannerView *view;

        view = PLANNER_VIEW (data);

        planner_usage_chart_zoom_in (PLANNER_USAGE_VIEW (view)->priv->chart);
        usage_view_update_zoom_sensitivity (view);
}

static void
usage_view_zoom_to_fit_cb (GtkAction *action,
			   gpointer   data)
{
        PlannerView *view;

        view = PLANNER_VIEW (data);

        planner_usage_chart_zoom_to_fit (PLANNER_USAGE_VIEW (view)->priv->chart);
        usage_view_update_zoom_sensitivity (view);
}

static void
usage_view_tree_view_size_request_cb (GtkWidget      *widget,
				      GtkRequisition *req,
				      gpointer        data)
{
        req->height = -1;
}

static gboolean
usage_view_tree_view_scroll_event_cb (GtkWidget      *widget,
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
usage_view_create_widget (PlannerView *view)
{
        PlannerUsageViewPriv *priv;
        MrpProject           *project;
        GtkWidget            *hpaned;
        GtkWidget            *left_frame;
        GtkWidget            *right_frame;
        PlannerUsageModel    *model;
        GtkWidget            *tree;
        GtkWidget            *vbox;
        GtkWidget            *sw;
        GtkWidget            *chart;
	GtkAdjustment        *hadj, *vadj;

        project = planner_window_get_project (view->main_window);
        priv = PLANNER_USAGE_VIEW (view)->priv;
        priv->project = project;

        g_signal_connect (project,
                          "loaded",
                          G_CALLBACK (usage_view_project_loaded_cb),
			  view);

        model = planner_usage_model_new (project);
        tree = planner_usage_tree_new (view->main_window, model);
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

        chart = planner_usage_chart_new_with_model (GTK_TREE_MODEL (model));
        priv->chart = PLANNER_USAGE_CHART (chart);
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

        g_signal_connect (tree,
			  "row_expanded",
                          G_CALLBACK (usage_view_row_expanded), chart);
        g_signal_connect (tree,
			  "row_collapsed",
                          G_CALLBACK (usage_view_row_collapsed), chart);
        g_signal_connect (tree,
			  "expand_all",
                          G_CALLBACK (usage_view_expand_all), chart);
        g_signal_connect (tree,
			  "collapse_all",
                          G_CALLBACK (usage_view_collapse_all), chart);
        g_signal_connect (chart,
			  "status_updated",
                          G_CALLBACK (usage_view_usage_status_updated),
                          view);
        g_signal_connect_after (tree,
				"size_request",
                                G_CALLBACK
                                (usage_view_tree_view_size_request_cb),
                                NULL);
        g_signal_connect_after (tree,
				"scroll_event",
                                G_CALLBACK
                                (usage_view_tree_view_scroll_event_cb),
                                view);
	g_signal_connect (tree,
			  "style_set",
			  G_CALLBACK (usage_view_tree_style_set_cb),
			  view);
	
        gtk_tree_view_expand_all (GTK_TREE_VIEW (tree));

	planner_usage_chart_expand_all (PLANNER_USAGE_CHART (chart));

	g_object_unref (model);

        return hpaned;
}

static void
usage_view_usage_status_updated (PlannerUsageChart *chart,
				 const gchar        *message,
				 PlannerView        *view)
{
	planner_window_set_status (view->main_window, message);
}

static void
usage_view_update_row_and_header_height (PlannerView *view)
{
	GtkTreeView        *tv;
	PlannerUsageChart  *chart;
	gint                row_height;
	gint                header_height;
	gint                height;
	GList              *cols, *l;
	GtkTreeViewColumn  *col;
	GtkRequisition      req;

	tv = GTK_TREE_VIEW (PLANNER_USAGE_VIEW (view)->priv->tree);
	chart = PLANNER_USAGE_VIEW (view)->priv->chart;
	
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

	/* Sync with the chart widget. */
	g_object_set (chart,
		      "header_height", header_height,
		      "row_height", row_height,
		      NULL);
}

static gboolean
idle_update_heights (PlannerView *view)
{
	usage_view_update_row_and_header_height (view);
	return FALSE;
}

static void
usage_view_tree_style_set_cb (GtkWidget   *tree,
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
usage_view_project_loaded_cb (MrpProject *project, PlannerView *view)
{
	PlannerUsageViewPriv *priv;
        GtkTreeModel         *model;

	priv = PLANNER_USAGE_VIEW (view)->priv;
	
	/* FIXME: This is not working so well. Look at how the gantt view
	 * handles this. (The crux is that the root task for example might
	 * change when a project is loaded, so we need to rconnect signals
	 * etc.)
	 */
        if (project == priv->project) {
		/* FIXME: Due to the above, we have this hack. */
		planner_usage_chart_setup_root_task (priv->chart);
		
                gtk_tree_view_expand_all (GTK_TREE_VIEW (priv->tree));
                planner_usage_chart_expand_all (priv->chart);
                return;
        }

	model = GTK_TREE_MODEL (planner_usage_model_new (project));

	planner_usage_tree_set_model (PLANNER_USAGE_TREE (priv->tree),
				      PLANNER_USAGE_MODEL (model));
        planner_usage_chart_set_model (PLANNER_USAGE_CHART (priv->chart), model);
	
	g_object_unref (model);

	gtk_tree_view_expand_all (GTK_TREE_VIEW (priv->tree));
        planner_usage_chart_expand_all (priv->chart);
}

static void
usage_view_row_expanded (GtkTreeView *tree_view,
			 GtkTreeIter *iter,
			 GtkTreePath *path,
			 gpointer     data)
{
        PlannerUsageChart *chart = data;

        planner_usage_chart_expand_row (chart, path);
}

static void
usage_view_row_collapsed (GtkTreeView *tree_view,
			  GtkTreeIter *iter,
			  GtkTreePath *path,
			  gpointer     data)
{
        PlannerUsageChart *chart = data;

	planner_usage_chart_collapse_row (chart, path);
}

static void
usage_view_expand_all (PlannerUsageTree *tree, PlannerUsageChart *chart)
{
        g_signal_handlers_block_by_func (tree,
                                         usage_view_row_expanded, chart);

        planner_usage_tree_expand_all (tree);
        planner_usage_chart_expand_all (chart);

        g_signal_handlers_unblock_by_func (tree,
                                           usage_view_row_expanded, chart);
}

static void
usage_view_collapse_all (PlannerUsageTree  *tree,
			 PlannerUsageChart *chart)
{
        g_signal_handlers_block_by_func (tree,
                                         usage_view_row_collapsed, chart);

        planner_usage_tree_collapse_all (tree);
        planner_usage_chart_collapse_all (chart);

        g_signal_handlers_unblock_by_func (tree,
                                           usage_view_row_collapsed, chart);
}

static void
usage_view_update_zoom_sensitivity (PlannerView *view)
{
	PlannerUsageViewPriv *priv;
        gboolean              in, out;

	priv = PLANNER_USAGE_VIEW (view)->priv;
	
        planner_usage_chart_can_zoom (priv->chart, &in, &out);

	g_object_set (gtk_action_group_get_action (
			      GTK_ACTION_GROUP (priv->actions),
			      "ZoomIn"),
		      "sensitive", in, 
		      NULL);

	g_object_set (gtk_action_group_get_action (
			      GTK_ACTION_GROUP (priv->actions),
			      "ZoomOut"),
		      "sensitive", out,
		      NULL);
}

PlannerView *
planner_usage_view_new (void)
{
	PlannerView *view;

	view = g_object_new (PLANNER_TYPE_USAGE_VIEW, NULL);

	return view;
}


/*
  TODO:
  planner_usage_print_data_new
  planner_usage_print_do
  planner_usage_print_get_n_pages
*/
