/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
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

/* FIXME: This code needs a SERIOUS clean-up. */

#include <config.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkmain.h>
#include <gtk/gtkitemfactory.h>
#include <gtk/gtkiconfactory.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkmessagedialog.h>
#include <libgnome/gnome-i18n.h>

#include "planner-format.h"
#include "planner-marshal.h"
#include "planner-cell-renderer-date.h"
#include "planner-task-dialog.h"
#include "planner-resource-dialog.h"
#include "planner-ttable-tree.h"
#include "planner-ttable-model.h"

enum {
	SELECTION_CHANGED,
	EXPAND_ALL,
	COLLAPSE_ALL,
	LAST_SIGNAL
};

struct _PlannerTtableTreePriv {
	MrpProject	*project;
	PlannerWindow	*main_window;
	GHashTable	*task_dialogs;
	GHashTable	*resource_dialogs;

	GtkItemFactory	*popup_factory;
};

static void     ttable_tree_class_init                   (PlannerTtableTreeClass *klass);
static void     ttable_tree_init                         (PlannerTtableTree      *tree);
static void     ttable_tree_finalize                     (GObject                *object);
static void     ttable_tree_popup_edit_resource_cb       (gpointer                callback_data,
							  guint                   action,
							  GtkWidget              *widget);
static void     ttable_tree_popup_edit_task_cb           (gpointer                callback_data,
							  guint                   action,
							  GtkWidget              *widget);
static void     ttable_tree_popup_expand_all_cb          (gpointer                callback_data,
							  guint                   action,
							  GtkWidget              *widget);
static void     ttable_tree_popup_collapse_all_cb        (gpointer                callback_data,
							  guint                   action,
							  GtkWidget              *widget);
static char *   ttable_tree_item_factory_trans           (const char             *path,
							  gpointer                data);
static void     ttable_tree_tree_view_popup_menu         (GtkWidget              *widget,
							  PlannerTtableTree      *tree);
static gboolean ttable_tree_tree_view_button_press_event (GtkTreeView            *tree_view,
							  GdkEventButton         *event,
							  PlannerTtableTree      *tree);
static void     ttable_tree_row_inserted                 (GtkTreeModel           *model,
							  GtkTreePath            *path,
							  GtkTreeIter            *iter,
							  GtkTreeView            *tree);


static GtkTreeViewClass *parent_class = NULL;
static guint signals[LAST_SIGNAL];

#define GIF_CB(x) ((GtkItemFactoryCallback)(x))

enum {
	POPUP_NONE,
	POPUP_REDIT,
	POPUP_TEDIT,
	POPUP_EXPAND,
	POPUP_COLLAPSE
};

static GtkItemFactoryEntry popup_menu_items[] = {
	{ N_("/_Edit resource..."),	NULL,	GIF_CB (ttable_tree_popup_edit_resource_cb),	POPUP_REDIT,	"<Item>",	NULL	},
	{ N_("/_Edit task..."),		NULL,	GIF_CB (ttable_tree_popup_edit_task_cb),	POPUP_TEDIT,	"<Item>",	NULL	},
	{ "/sep1",			NULL,	0,						POPUP_NONE,	"<Separator>" },
	{ N_("/_Expand all resources"),	NULL,	GIF_CB (ttable_tree_popup_expand_all_cb),	POPUP_EXPAND,	"<Item>",	NULL	},
	{ N_("/_Collapse all resources"), NULL,	GIF_CB (ttable_tree_popup_collapse_all_cb),	POPUP_COLLAPSE,	"<Item>",	NULL	},
};

GType
planner_ttable_tree_get_type(void) {
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (PlannerTtableTreeClass),
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) ttable_tree_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof (PlannerTtableTree),
			0,              /* n_preallocs */
			(GInstanceInitFunc) ttable_tree_init
		};
		type = g_type_register_static (GTK_TYPE_TREE_VIEW, "PlannerTtableTree",
					       &info, 0);
	}
	return type;
}

static void
ttable_tree_class_init (PlannerTtableTreeClass *klass)
{
	GObjectClass *o_class;

	parent_class = g_type_class_peek_parent (klass);

	o_class = (GObjectClass *) klass;

	o_class->finalize = ttable_tree_finalize;

	signals[SELECTION_CHANGED] =
		g_signal_new ("selection-changed",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				0,
				NULL, NULL,
				planner_marshal_VOID__VOID,
				G_TYPE_NONE, 0);
	signals[EXPAND_ALL] =
		g_signal_new ("expand-all",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				0,
				NULL, NULL,
				planner_marshal_VOID__VOID,
				G_TYPE_NONE, 0);
	
	signals[COLLAPSE_ALL] =
		g_signal_new ("collapse-all",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				0,
				NULL, NULL,
				planner_marshal_VOID__VOID,
				G_TYPE_NONE, 0);
	
}

static void
ttable_tree_init (PlannerTtableTree *tree)
{
	PlannerTtableTreePriv *priv;

	priv = g_new0(PlannerTtableTreePriv,1);
	tree->priv = priv;

	priv->popup_factory = gtk_item_factory_new (
			GTK_TYPE_MENU,
			"<main>",
			NULL);
	gtk_item_factory_set_translate_func (
			priv->popup_factory,
			ttable_tree_item_factory_trans,
			NULL,
			NULL);
	gtk_item_factory_create_items (
			priv->popup_factory,
			G_N_ELEMENTS (popup_menu_items),
			popup_menu_items,
			tree);
}

static void
ttable_tree_finalize (GObject *object)
{
	PlannerTtableTree		*tree;
	PlannerTtableTreePriv	*priv;
	tree = PLANNER_TTABLE_TREE(object);
	priv = tree->priv;
	g_free(priv);
	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}

void
planner_ttable_tree_set_model (PlannerTtableTree  *tree,
			       PlannerTtableModel *model)
{
	gtk_tree_view_set_model (GTK_TREE_VIEW (tree),
				 GTK_TREE_MODEL (model));
	gtk_tree_view_expand_all (GTK_TREE_VIEW (tree));

	g_signal_connect (model,
			  "row-inserted",
			  G_CALLBACK (ttable_tree_row_inserted),
			  tree);
}

static void
ttable_tree_setup_tree_view (GtkTreeView   *gtk_tree,
			     MrpProject    *project,
			     PlannerTtableModel *model)
{
	PlannerTtableTree     *tree;

	tree = PLANNER_TTABLE_TREE(gtk_tree);

	planner_ttable_tree_set_model(tree,model);

	gtk_tree_view_set_rules_hint (gtk_tree, TRUE);
	gtk_tree_view_set_reorderable (gtk_tree, TRUE);

	g_signal_connect (gtk_tree,
			  "popup_menu",
			  G_CALLBACK (ttable_tree_tree_view_popup_menu),
			  gtk_tree);
	g_signal_connect (gtk_tree,
			  "button_press_event",
			  G_CALLBACK (ttable_tree_tree_view_button_press_event),
			  gtk_tree);
	
}

static void
ttable_tree_resname_data_func (GtkTreeViewColumn *tree_column,
			       GtkCellRenderer   *cell,
			       GtkTreeModel      *tree_model,
			       GtkTreeIter       *iter,
			       gpointer           data)
{
	gchar *name;
	gtk_tree_model_get (tree_model,
			    iter,
			    COL_RESNAME,&name,
			    -1);
	g_object_set (cell,
		      "text", name,
		      NULL);
	g_free(name);
}

static void
ttable_tree_taskname_data_func (GtkTreeViewColumn *tree_column,
			        GtkCellRenderer   *cell,
			        GtkTreeModel      *tree_model,
			        GtkTreeIter       *iter,
			        gpointer           data)
{
	gchar *name;
	gtk_tree_model_get (tree_model,
			    iter,
			    COL_TASKNAME,&name,
			    -1);
	g_object_set (cell,
		      "text", name,
		      NULL);
	g_free(name);
}

static void
ttable_tree_add_column(GtkTreeView *tree,
		       gint         column,
		       const gchar *title)
{
	GtkTreeViewColumn *col;
	GtkCellRenderer   *cell;

	switch (column) {
		case COL_RESNAME:
			cell = gtk_cell_renderer_text_new ();
			g_object_set (cell, "editable", FALSE, NULL);
			col = gtk_tree_view_column_new_with_attributes (title,cell,NULL);
			gtk_tree_view_column_set_cell_data_func (col,cell,
					ttable_tree_resname_data_func,NULL,NULL);
			g_object_set_data (G_OBJECT (col),"data-func",
					ttable_tree_resname_data_func);
			gtk_tree_view_column_set_resizable (col, TRUE);
			gtk_tree_view_column_set_min_width (col, 100);
			gtk_tree_view_append_column (tree, col);
			break;
		case COL_TASKNAME:
			cell = gtk_cell_renderer_text_new ();
			g_object_set (cell, "editable", FALSE, NULL);
			col = gtk_tree_view_column_new_with_attributes (title,cell,NULL);
			gtk_tree_view_column_set_cell_data_func (col,cell,
					ttable_tree_taskname_data_func,NULL,NULL);
			g_object_set_data (G_OBJECT (col),"data-func",
					ttable_tree_taskname_data_func);
			gtk_tree_view_column_set_resizable (col, TRUE);
			gtk_tree_view_column_set_min_width (col, 100);
			gtk_tree_view_append_column (tree, col);
			break;
		case COL_RESOURCE:
		case COL_ASSIGNMENT:
		default:
			break;
	}
}

GtkWidget *
planner_ttable_tree_new (PlannerWindow  *main_window,
			 PlannerTtableModel *model)
{
	MrpProject		*project;
	PlannerTtableTree		*tree;
	PlannerTtableTreePriv	*priv;

	tree = g_object_new (PLANNER_TYPE_TTABLE_TREE, NULL);

	project = planner_window_get_project (main_window);

	priv = tree->priv;

	priv->main_window = main_window;
	priv->project = project;

	ttable_tree_setup_tree_view (GTK_TREE_VIEW (tree), project, model);

	ttable_tree_add_column (GTK_TREE_VIEW (tree), COL_RESNAME, _("\nName"));
	ttable_tree_add_column (GTK_TREE_VIEW (tree), COL_TASKNAME, _("\nTask"));

	return GTK_WIDGET(tree);
}






/* For the popup menu, and associated actions */
static void
ttable_tree_popup_edit_resource_cb	(gpointer	 callback_data,
					 guint		 action,
					 GtkWidget	*widget)
{
	planner_ttable_tree_edit_resource(callback_data);
}

static void
ttable_tree_popup_edit_task_cb		(gpointer	 callback_data,
					 guint		 action,
					 GtkWidget	*widget)
{
	planner_ttable_tree_edit_task(callback_data);
}

static void
ttable_tree_popup_expand_all_cb		(gpointer	 callback_data,
					 guint		 action,
					 GtkWidget	*widget)
{
	g_signal_emit(callback_data,signals[EXPAND_ALL],0);
}

void
planner_ttable_tree_expand_all	(PlannerTtableTree	*tree)
{
	g_return_if_fail(PLANNER_IS_TTABLE_TREE(tree));
	gtk_tree_view_expand_all(GTK_TREE_VIEW(tree));
}

void
planner_ttable_tree_collapse_all	(PlannerTtableTree	*tree)
{
	g_return_if_fail(PLANNER_IS_TTABLE_TREE(tree));
	gtk_tree_view_collapse_all(GTK_TREE_VIEW(tree));
}

static void
ttable_tree_popup_collapse_all_cb	(gpointer	 callback_data,
					 guint		 action,
					 GtkWidget	*widget)
{
	g_signal_emit(callback_data,signals[COLLAPSE_ALL],0);
}

void
planner_ttable_tree_edit_resource	(PlannerTtableTree	*tree)
{
	PlannerTtableTreePriv	*priv;
	MrpResource		*resource;
	MrpAssignment		*assignment;
	GtkWidget		*dialog;
	GList			*list;

	g_return_if_fail(PLANNER_IS_TTABLE_TREE(tree));

	priv = tree->priv;

	list = planner_ttable_tree_get_selected_items(tree);
	if (list==NULL)
		return;

	if (MRP_IS_RESOURCE(list->data)) {
		resource = MRP_RESOURCE(list->data);
	} else {
		assignment = MRP_ASSIGNMENT(list->data);
		resource = mrp_assignment_get_resource(assignment);
	}

	dialog = planner_resource_dialog_new (priv->main_window, resource);
	gtk_widget_show (dialog);
	g_list_free(list);
}

void
planner_ttable_tree_edit_task	(PlannerTtableTree	*tree)
{
	PlannerTtableTreePriv	*priv;
	MrpAssignment		*assignment;
	MrpTask			*task;
	GtkWidget		*dialog;
	GList			*list, *l;

	g_return_if_fail(PLANNER_IS_TTABLE_TREE(tree));

	priv = tree->priv;

	list = planner_ttable_tree_get_selected_items(tree);
	if (list==NULL)
		return;

	assignment = NULL;
	l = list;
	while (l != NULL && assignment == NULL) {
		if (MRP_IS_ASSIGNMENT(l->data)) {
			assignment = MRP_ASSIGNMENT(l->data);
		} else {
			l = l->next;
		}
	}
	if (assignment == NULL)
		return;
	task = mrp_assignment_get_task(assignment);

	dialog = planner_task_dialog_new (priv->main_window, task,
					  PLANNER_TASK_DIALOG_PAGE_GENERAL);
	gtk_widget_show (dialog);
	g_list_free (list);
}

static void
ttable_tree_get_selected_func	(GtkTreeModel	*model,
				 GtkTreePath	*path,
				 GtkTreeIter	*iter,
				 gpointer	 data)
{
	GList		**list = data;
	MrpAssignment	 *assignment;
	MrpResource	 *resource;

	gtk_tree_model_get (model, iter,
			COL_ASSIGNMENT, &assignment,
			COL_RESOURCE, &resource,
			-1);
	if (assignment != NULL) {
		*list = g_list_prepend(*list,assignment);
	} else if (resource != NULL) {
		*list = g_list_prepend(*list,resource);
	} else {
		g_warning("PlannerTtableTree: no resource nor assignment !!!");
	}
}

GList*
planner_ttable_tree_get_selected_items	(PlannerTtableTree	*tree)
{
	GtkTreeSelection	*selection;
	GList			*list;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

	list = NULL;

	gtk_tree_selection_selected_foreach (
			selection,
			ttable_tree_get_selected_func,
			&list);
	list = g_list_reverse (list);
	return list;
}

static void
ttable_tree_tree_view_popup_menu	(GtkWidget	*widget,
					 PlannerTtableTree	*tree)
{
	gint	x, y;

	gdk_window_get_pointer (widget->window, &x, &y, NULL);

	gtk_item_factory_popup (tree->priv->popup_factory,
			x, y, 0,
			gtk_get_current_event_time ());
}

static gboolean
ttable_tree_tree_view_button_press_event	(GtkTreeView	*tree_view,
						 GdkEventButton	*event,
						 PlannerTtableTree	*tree)
{
	GtkTreePath		*path;
	GtkTreeView		*tv;
	PlannerTtableTreePriv	*priv;
	PlannerTtableModel		*model;
	GtkItemFactory		*factory;

	tv = GTK_TREE_VIEW(tree);
	priv = tree->priv;
	factory = priv->popup_factory;
	model = PLANNER_TTABLE_MODEL(gtk_tree_view_get_model(tv));

	if (event->button == 3) {
		gtk_widget_grab_focus (GTK_WIDGET (tree));
		gtk_widget_set_sensitive (
				gtk_item_factory_get_widget_by_action (factory, POPUP_EXPAND), TRUE);
		gtk_widget_set_sensitive (
				gtk_item_factory_get_widget_by_action (factory, POPUP_COLLAPSE), TRUE);

		if (gtk_tree_view_get_path_at_pos (tv, event->x, event->y, &path, NULL, NULL, NULL)) {
			gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (tv));
			gtk_tree_selection_select_path (gtk_tree_view_get_selection (tv), path);
			if (planner_ttable_model_path_is_assignment(model,path)) {
				gtk_widget_set_sensitive (
						gtk_item_factory_get_widget_by_action (factory, POPUP_REDIT), TRUE);
				gtk_widget_set_sensitive (
						gtk_item_factory_get_widget_by_action (factory, POPUP_TEDIT), TRUE);
			} else {
				gtk_widget_set_sensitive (
						gtk_item_factory_get_widget_by_action (factory, POPUP_REDIT), TRUE);
				gtk_widget_set_sensitive (
						gtk_item_factory_get_widget_by_action (factory, POPUP_TEDIT), FALSE);
			}
			gtk_tree_path_free (path);
		}
		gtk_item_factory_popup (factory, event->x_root, event->y_root,
				event->button, event->time);
		return TRUE;
	}
	return FALSE;
}

static char *
ttable_tree_item_factory_trans	(const char *path, gpointer data)
{
	return _((gchar*)path);
}

static void
ttable_tree_row_inserted (GtkTreeModel *model,
			  GtkTreePath  *path,
			  GtkTreeIter  *iter,
			  GtkTreeView  *tree)
{
	GtkTreePath *parent;

	parent = gtk_tree_path_copy (path);
	
	gtk_tree_path_up (parent);

	gtk_tree_view_expand_row (tree,
				  parent,
				  FALSE);

	gtk_tree_path_free (parent);
}
