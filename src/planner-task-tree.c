/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003      Imendio HB
 * Copyright (C) 2002-2003 CodeFactory AB
 * Copyright (C) 2002-2003 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2002 Alvaro del Castillo <acs@barrapunto.com>
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
#include "planner-task-input-dialog.h"
#include "planner-property-dialog.h"
#include "planner-task-tree.h"
#include "planner-gantt-model.h"

enum {
	SELECTION_CHANGED,
	RELATION_ADDED,
	RELATION_REMOVED,
	LAST_SIGNAL
};

struct _PlannerTaskTreePriv {
	GtkItemFactory *popup_factory;
	gboolean        custom_properties;
	MrpProject     *project;
	GHashTable     *property_to_column;

	PlannerWindow   *main_window;
	
	/* Keep the dialogs here so that we can just raise the dialog if it's
	 * opened twice for the same task.
	 */
	GHashTable     *task_dialogs;
};

typedef struct {
	GtkTreeView *tree;
	MrpProperty *property;
} ColPropertyData;

typedef enum {
	UNIT_NONE,
	UNIT_MONTH,
	UNIT_WEEK,
	UNIT_DAY,
	UNIT_HOUR,
	UNIT_MINUTE
} Unit;

typedef struct {
	gchar *name;
	Unit   unit;
} Units;

static void        task_tree_class_init                (PlannerTaskTreeClass *klass);
static void        task_tree_init                      (PlannerTaskTree      *tree);
static void        task_tree_finalize                  (GObject              *object);
static void        task_tree_setup_tree_view           (GtkTreeView          *tree,
							MrpProject           *project,
							PlannerGanttModel    *model);
static void        task_tree_add_column                (GtkTreeView          *tree,
							gint                  column,
							const gchar          *title);
static char    *   task_tree_item_factory_trans        (const char           *path,
							gpointer              data);
static void        task_tree_name_data_func            (GtkTreeViewColumn    *tree_column,
							GtkCellRenderer      *cell,
							GtkTreeModel         *tree_model,
							GtkTreeIter          *iter,
							gpointer              data);
static void        task_tree_start_data_func           (GtkTreeViewColumn    *tree_column,
							GtkCellRenderer      *cell,
							GtkTreeModel         *tree_model,
							GtkTreeIter          *iter,
							gpointer              data);
static void        task_tree_finish_data_func          (GtkTreeViewColumn    *tree_column,
							GtkCellRenderer      *cell,
							GtkTreeModel         *tree_model,
							GtkTreeIter          *iter,
							gpointer              data);
static void        task_tree_duration_data_func        (GtkTreeViewColumn    *tree_column,
							GtkCellRenderer      *cell,
							GtkTreeModel         *tree_model,
							GtkTreeIter          *iter,
							gpointer              data);
static void        task_tree_cost_data_func            (GtkTreeViewColumn    *tree_column,
							GtkCellRenderer      *cell,
							GtkTreeModel         *tree_model,
							GtkTreeIter          *iter,
							gpointer              data);
static void        task_tree_work_data_func            (GtkTreeViewColumn    *tree_column,
							GtkCellRenderer      *cell,
							GtkTreeModel         *tree_model,
							GtkTreeIter          *iter,
							gpointer              data);
static void        task_tree_popup_insert_task_cb      (gpointer              callback_data,
							guint                 action,
							GtkWidget            *widget);
static void        task_tree_popup_insert_subtask_cb   (gpointer              callback_data,
							guint                 action,
							GtkWidget            *widget);
static void        task_tree_popup_remove_task_cb      (gpointer              callback_data,
							guint                 action,
							GtkWidget            *widget);
static void        task_tree_popup_edit_task_cb        (gpointer              callback_data,
							guint                 action,
							GtkWidget            *widget);
static void        task_tree_popup_unlink_task_cb      (gpointer              callback_data,
							guint                 action,
							GtkWidget            *widget);
static void        task_tree_block_selection_changed   (PlannerTaskTree      *tree);
static void        task_tree_unblock_selection_changed (PlannerTaskTree      *tree);
static void        task_tree_selection_changed_cb      (GtkTreeSelection     *selection,
							PlannerTaskTree      *tree);
static void        task_tree_relation_added_cb         (MrpTask              *task,
							MrpRelation          *relation,
							PlannerTaskTree      *tree);
static void        task_tree_relation_removed_cb       (MrpTask              *task,
							MrpRelation          *relation,
							PlannerTaskTree      *tree);
static void        task_tree_row_inserted              (GtkTreeModel         *model,
							GtkTreePath          *path,
							GtkTreeIter          *iter,
							GtkTreeView          *tree);
static void        task_tree_task_added_cb             (PlannerGanttModel    *model,
							MrpTask              *task,
							PlannerTaskTree      *tree);
static void        task_tree_task_removed_cb           (PlannerGanttModel    *model,
							MrpTask              *task,
							PlannerTaskTree      *tree);
static gint        task_tree_parse_time_string         (PlannerTaskTree      *tree,
							const gchar          *str);
static MrpProject *task_tree_get_project               (PlannerTaskTree      *tree);
static MrpTask *   task_tree_get_task_from_path        (PlannerTaskTree      *tree,
							GtkTreePath          *path);


/*
 * Commands
 */

typedef struct {
	PlannerCmd       base;

	PlannerTaskTree *tree;
	MrpProject      *project;
	
	gchar           *name;
	gint             work;
	gint             duration;
	
	GtkTreePath     *path;

	MrpTask         *task; 	/* The inserted task */
} TaskCmdInsert;

static void
task_cmd_insert_do (PlannerCmd *cmd_base)
{
	TaskCmdInsert *cmd;
	GtkTreePath   *path;
	MrpTask       *task, *parent;
	gint           depth;
	gint           position;

	cmd = (TaskCmdInsert*) cmd_base;

	path = gtk_tree_path_copy (cmd->path);

	depth = gtk_tree_path_get_depth (path);
	position = gtk_tree_path_get_indices (path)[depth];

	if (gtk_tree_path_up (path) && gtk_tree_path_get_depth (path) > 0) {
		parent = task_tree_get_task_from_path (cmd->tree, path);
	} else {
		parent = NULL;
	}
	
	gtk_tree_path_free (path);
	
	task = g_object_new (MRP_TYPE_TASK,
			     "name", cmd->name,
			     "work", cmd->work,
			     "duration", cmd->duration,
			     NULL);
	
	mrp_project_insert_task (cmd->project,
				 parent,
				 position,
				 task);

	cmd->task = task;
}

static void
task_cmd_insert_undo (PlannerCmd *cmd_base)
{
	TaskCmdInsert *cmd;
	
	cmd = (TaskCmdInsert*) cmd_base;

	mrp_project_remove_task (cmd->project,
				 cmd->task);
	
	cmd->task = NULL;
}

static PlannerCmd *
task_cmd_insert (PlannerTaskTree *tree,
		 GtkTreePath     *path,
		 gint             work,
		 gint             duration)
{
	PlannerTaskTreePriv *priv = tree->priv;
	PlannerCmd          *cmd_base;
	TaskCmdInsert       *cmd;
	static gint          i = 1;

	cmd = g_new0 (TaskCmdInsert, 1);

	cmd_base = (PlannerCmd*) cmd;

	cmd_base->label = g_strdup ("Insert task");
	cmd_base->do_func = task_cmd_insert_do;
	cmd_base->undo_func = task_cmd_insert_undo;
	cmd_base->free_func = NULL; /* FIXME */

	cmd->tree = tree;
	cmd->project = task_tree_get_project (tree);

	cmd->name = g_strdup_printf ("Foo %d", i++);
	
	cmd->path = path;
	cmd->work = work;
	cmd->duration = duration;
	
	planner_window_cmd_manager_insert_and_do (priv->main_window, cmd_base);

	return cmd_base;
}

typedef struct {
	PlannerCmd         base;

	PlannerTaskTree   *tree;
	MrpProject        *project;
	
	GtkTreePath       *path;
	
	gchar             *property;  
	GValue            *value;
	GValue            *old_value;
} TaskCmdEditProperty;

static void
task_cmd_edit_property_do (PlannerCmd *cmd_base)
{
	TaskCmdEditProperty *cmd;
	MrpTask             *task;
	
	cmd = (TaskCmdEditProperty*) cmd_base;

	task = task_tree_get_task_from_path (cmd->tree, cmd->path);
	
	g_object_set_property (G_OBJECT (task),
			       cmd->property,
			       cmd->value);
}

static void
task_cmd_edit_property_undo (PlannerCmd *cmd_base)
{
	TaskCmdEditProperty *cmd;
	MrpTask             *task;
	
	cmd = (TaskCmdEditProperty*) cmd_base;

	task = task_tree_get_task_from_path (cmd->tree, cmd->path);
	
	g_object_set_property (G_OBJECT (task),
			       cmd->property,
			       cmd->old_value);
}

static PlannerCmd *
task_cmd_edit_property (PlannerTaskTree *tree,
			MrpTask         *task,
			const gchar     *property,
			GValue          *value)
{
	PlannerTaskTreePriv *priv = tree->priv;
	PlannerCmd          *cmd_base;
	TaskCmdEditProperty *cmd;
	PlannerGanttModel   *model;

	cmd = g_new0 (TaskCmdEditProperty, 1);

	cmd_base = (PlannerCmd*) cmd;

	cmd_base->label = g_strdup ("Edit task property");

	cmd_base->do_func = task_cmd_edit_property_do;
	cmd_base->undo_func = task_cmd_edit_property_undo;
	cmd_base->free_func = NULL; /* FIXME: task_cmd_edit_free */

	cmd->tree = tree;
	cmd->project = task_tree_get_project (tree);

	model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));
	
	cmd->path = planner_gantt_model_get_path_from_task (model, task);
	
	cmd->property = g_strdup (property);

	cmd->value = g_new0 (GValue, 1);
	g_value_init (cmd->value, G_VALUE_TYPE (value));
	g_value_copy (value, cmd->value);

	// FIXME
	cmd->old_value = g_new0 (GValue, 1);
	g_value_init (cmd->old_value, G_TYPE_STRING);

	g_object_get_property (G_OBJECT (task),
			       cmd->property,
			       cmd->old_value);

	planner_window_cmd_manager_insert_and_do (priv->main_window, cmd_base);
	
	return cmd_base;
}





static GtkTreeViewClass *parent_class = NULL;
static guint signals[LAST_SIGNAL];

enum {
	POPUP_NONE,
	POPUP_INSERT,
	POPUP_SUBTASK,
	POPUP_REMOVE,
	POPUP_UNLINK,
	POPUP_EDIT
};


#define GIF_CB(x) ((GtkItemFactoryCallback)(x))

static GtkItemFactoryEntry popup_menu_items[] = {
	{ N_("/_Insert task"),       NULL, GIF_CB (task_tree_popup_insert_task_cb),    POPUP_INSERT,  "<Item>",       NULL },
	{ N_("/_Insert subtask"),    NULL, GIF_CB (task_tree_popup_insert_subtask_cb), POPUP_SUBTASK, "<Item>",       NULL },
	{ N_("/_Remove task"),       NULL, GIF_CB (task_tree_popup_remove_task_cb),    POPUP_REMOVE,  "<StockItem>",  GTK_STOCK_DELETE },
	{ "/sep1",                   NULL, 0,                                          POPUP_NONE,    "<Separator>" },
	{ N_("/_Unlink task"),       NULL, GIF_CB (task_tree_popup_unlink_task_cb),    POPUP_UNLINK,  "<Item>",       NULL },
	{ "/sep2",                   NULL, 0,                                          POPUP_NONE,    "<Separator>" },
	{ N_("/_Edit task..."),      NULL, GIF_CB (task_tree_popup_edit_task_cb),      POPUP_EDIT,    "<Item>",       NULL }, 
};


GType
planner_task_tree_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (PlannerTaskTreeClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) task_tree_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (PlannerTaskTree),
			0,              /* n_preallocs */
			(GInstanceInitFunc) task_tree_init
		};

		type = g_type_register_static (GTK_TYPE_TREE_VIEW, "PlannerTaskTree",
					       &info, 0);
	}
	
	return type;
}

static void
task_tree_class_init (PlannerTaskTreeClass *klass)
{
	GObjectClass *o_class;

	parent_class = g_type_class_peek_parent (klass);

	o_class = (GObjectClass *) klass;

	o_class->finalize = task_tree_finalize;

	signals[SELECTION_CHANGED] =
		g_signal_new ("selection-changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      planner_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	signals[RELATION_ADDED] =
		g_signal_new ("relation-added",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      planner_marshal_VOID__OBJECT_OBJECT,
			      G_TYPE_NONE, 
			      2, MRP_TYPE_TASK, MRP_TYPE_RELATION);
	signals[RELATION_REMOVED] = 
		g_signal_new ("relation-removed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      planner_marshal_VOID__OBJECT_OBJECT,
			      G_TYPE_NONE, 
			      2, MRP_TYPE_TASK, MRP_TYPE_RELATION);
}

static void
task_tree_init (PlannerTaskTree *tree)
{
	PlannerTaskTreePriv *priv;
	GtkIconFactory *icon_factory;
	GtkIconSet     *icon_set;
	GdkPixbuf      *pixbuf;
	
	priv = g_new0 (PlannerTaskTreePriv, 1);
	tree->priv = priv;

	priv->property_to_column = g_hash_table_new (NULL, NULL);
	
	priv->popup_factory = gtk_item_factory_new (GTK_TYPE_MENU,
						    "<main>",
						    NULL);
	gtk_item_factory_set_translate_func (priv->popup_factory,
					     task_tree_item_factory_trans,
					     NULL,
					     NULL);
	
	gtk_item_factory_create_items (priv->popup_factory,
				       G_N_ELEMENTS (popup_menu_items),
				       popup_menu_items,
				       tree);

	/* Add stock icons. */
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

	pixbuf = gdk_pixbuf_new_from_file (IMAGEDIR "/24_task_up.png", NULL);
	icon_set = gtk_icon_set_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);
	gtk_icon_factory_add (icon_factory,
			      "planner-stock-move-task-up",
			      icon_set);

	pixbuf = gdk_pixbuf_new_from_file (IMAGEDIR "/24_task_down.png", NULL);
	icon_set = gtk_icon_set_new_from_pixbuf (pixbuf);
	g_object_unref (pixbuf);
	gtk_icon_factory_add (icon_factory,
			      "planner-stock-move-task-down",
			      icon_set);
}

static void
task_tree_finalize (GObject *object)
{
	PlannerTaskTree     *tree;
	PlannerTaskTreePriv *priv;

	tree = PLANNER_TASK_TREE (object);
	priv = tree->priv;
	
	g_hash_table_destroy (priv->property_to_column);

	g_free (priv);

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}

static void
task_tree_popup_insert_task_cb (gpointer   callback_data,
				guint      action,
				GtkWidget *widget)
{
	planner_task_tree_insert_task (callback_data);
}

static void
task_tree_popup_insert_subtask_cb (gpointer   callback_data,
				   guint      action,
				   GtkWidget *widget)
{
	planner_task_tree_insert_subtask (callback_data);
}

static void
task_tree_popup_remove_task_cb (gpointer   callback_data,
				guint      action,
				GtkWidget *widget)
{
	planner_task_tree_remove_task (callback_data);
}

static void
task_tree_popup_edit_task_cb (gpointer   callback_data,
			      guint      action,
			      GtkWidget *widget)
{
	planner_task_tree_edit_task (callback_data);
}

static void
task_tree_popup_unlink_task_cb (gpointer   callback_data,
				guint      action,
				GtkWidget *widget)
{
	planner_task_tree_unlink_task (callback_data);
}

static void
task_tree_block_selection_changed (PlannerTaskTree *tree)
{
	GtkTreeSelection *selection;
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	
	g_signal_handlers_block_by_func (selection,
					 task_tree_selection_changed_cb,
					 tree);
}

static void
task_tree_unblock_selection_changed (PlannerTaskTree *tree)
{
	GtkTreeSelection *selection;
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	
	g_signal_handlers_unblock_by_func (selection,
					   task_tree_selection_changed_cb,
					   tree);
}

static void 
task_tree_selection_changed_cb (GtkTreeSelection *selection,
				PlannerTaskTree       *tree)
{
	g_return_if_fail (GTK_IS_TREE_SELECTION (selection));
	g_return_if_fail (PLANNER_IS_TASK_TREE (tree));

	g_signal_emit (tree, signals[SELECTION_CHANGED], 0, NULL);
}

static void
task_tree_relation_added_cb (MrpTask     *task, 
			     MrpRelation *relation, 
			     PlannerTaskTree  *tree)
{
	g_return_if_fail (MRP_IS_TASK (task));
	g_return_if_fail (MRP_IS_RELATION (relation));
	
	g_signal_emit (tree, signals[RELATION_ADDED], 0, task, relation);
}

static void
task_tree_relation_removed_cb (MrpTask     *task,
			       MrpRelation *relation,
			       PlannerTaskTree  *tree)
{
	g_return_if_fail (MRP_IS_TASK (task));
	g_return_if_fail (MRP_IS_RELATION (relation));
	
	g_signal_emit (tree, signals[RELATION_REMOVED], 0, task, relation);
}

static void
task_tree_row_inserted (GtkTreeModel *model,
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

static void
task_tree_task_added_cb (PlannerGanttModel *model, MrpTask *task, PlannerTaskTree *tree)
{
	g_object_ref (task);

	g_signal_connect (task, "relation_added", 
			  G_CALLBACK (task_tree_relation_added_cb),
			  tree);
	g_signal_connect (task, "relation_removed",
			  G_CALLBACK (task_tree_relation_removed_cb),
			  tree);
}

static void
task_tree_task_removed_cb (PlannerGanttModel *model,
			   MrpTask      *task,
			   PlannerTaskTree   *tree)
{
	g_signal_handlers_disconnect_by_func (task,
					      task_tree_relation_added_cb,
					      tree);
	g_signal_handlers_disconnect_by_func (task,
					      task_tree_relation_removed_cb,
					      tree);
	g_object_unref (task);
}

static void
task_tree_tree_view_popup_menu (GtkWidget  *widget,
				PlannerTaskTree *tree)
{
	gint x, y;

	/* FIXME: We should position the popup at the selected cell. */
	gdk_window_get_pointer (widget->window, &x, &y, NULL);

	gtk_item_factory_popup (tree->priv->popup_factory,
				x, y,
				0,
				gtk_get_current_event_time ());
}

static gboolean
task_tree_tree_view_button_press_event (GtkTreeView    *tree_view,
					GdkEventButton *event,
					PlannerTaskTree     *tree)
{
	GtkTreePath    *path;
	GtkTreeView    *tv;
	PlannerTaskTreePriv *priv;
	GtkItemFactory *factory;

	tv = GTK_TREE_VIEW (tree);
	priv = tree->priv;
	factory = priv->popup_factory;

	if (event->button == 3) {
		gtk_widget_grab_focus (GTK_WIDGET (tree));

		/* Select our row */
		if (gtk_tree_view_get_path_at_pos (tv, event->x, event->y, &path, NULL, NULL, NULL)) {
			gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (tv));

			gtk_tree_selection_select_path (gtk_tree_view_get_selection (tv), path);

			gtk_widget_set_sensitive (
				gtk_item_factory_get_widget_by_action (factory, POPUP_SUBTASK), TRUE);
			gtk_widget_set_sensitive (
				gtk_item_factory_get_widget_by_action (factory, POPUP_REMOVE), TRUE);
			gtk_widget_set_sensitive (
				gtk_item_factory_get_widget_by_action (factory, POPUP_UNLINK), TRUE);
			gtk_widget_set_sensitive (
				gtk_item_factory_get_widget_by_action (factory, POPUP_EDIT), TRUE);
			
			gtk_tree_path_free (path);
		} else {
			gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (tv));

			gtk_widget_set_sensitive (
				gtk_item_factory_get_widget_by_action (factory, POPUP_SUBTASK), FALSE);
			gtk_widget_set_sensitive (
				gtk_item_factory_get_widget_by_action (factory, POPUP_REMOVE), FALSE);
			gtk_widget_set_sensitive (
				gtk_item_factory_get_widget_by_action (factory, POPUP_UNLINK), FALSE);
			gtk_widget_set_sensitive (
				gtk_item_factory_get_widget_by_action (factory, POPUP_EDIT), FALSE);
		}
		
		gtk_item_factory_popup (factory, event->x_root, event->y_root,
					event->button, event->time);
		return TRUE;
	}

	return FALSE;
}

static void
task_tree_name_data_func (GtkTreeViewColumn *tree_column,
			  GtkCellRenderer   *cell,
			  GtkTreeModel      *tree_model,
			  GtkTreeIter       *iter,
			  gpointer           data)
{
	gchar *name;
	gint   weight;
	
	gtk_tree_model_get (tree_model,
			    iter,
			    COL_NAME, &name,
			    COL_WEIGHT, &weight,
			    -1);
	
	g_object_set (cell,
		      "text", name, 
		      "weight", weight,
		      NULL);
	g_free (name);
}

static void
task_tree_start_data_func (GtkTreeViewColumn *tree_column,
			   GtkCellRenderer   *cell,
			   GtkTreeModel      *tree_model,
			   GtkTreeIter       *iter,
			   gpointer           data)
{
	glong     start;
	gint      weight;
	gboolean  editable;
	gchar    *str;
 
	gtk_tree_model_get (tree_model,
			    iter,
			    COL_START, &start,
			    COL_WEIGHT, &weight,
			    COL_EDITABLE, &editable, 
			    -1);

	str = planner_format_date (start);

	g_object_set (cell, 
		      "text", str,
		      "weight", weight,
		      "editable", editable,
		      NULL);
	g_free (str);
}

static void
task_tree_finish_data_func (GtkTreeViewColumn *tree_column,
			    GtkCellRenderer   *cell,
			    GtkTreeModel      *tree_model,
			    GtkTreeIter       *iter,
			    gpointer           data)
{
	glong  start;
	gchar *str;
	gint   weight;

	gtk_tree_model_get (tree_model,
			    iter,
			    COL_FINISH, &start,
			    COL_WEIGHT, &weight,
			    -1);

	str = planner_format_date (start);
	
	g_object_set (cell,
		      "text", str,
		      "weight", weight,
		      NULL);
	g_free (str);
}

static void
task_tree_duration_data_func (GtkTreeViewColumn *tree_column,
			      GtkCellRenderer   *cell,
			      GtkTreeModel      *tree_model,
			      GtkTreeIter       *iter,
			      gpointer           data)
{
	PlannerTaskTree     *task_tree;
	PlannerTaskTreePriv *priv;
	MrpCalendar    *calendar;
	gint            hours_per_day;
	gint            duration;
	gchar          *str;
	gint            weight;
	gboolean        editable;

	task_tree = PLANNER_TASK_TREE (data);
	priv = task_tree->priv;
	
	gtk_tree_model_get (tree_model,
			    iter,
			    COL_DURATION, &duration,
			    COL_WEIGHT, &weight,
			    COL_EDITABLE, &editable,
			    -1);

	calendar = mrp_project_get_calendar (priv->project);
	
	hours_per_day = mrp_calendar_day_get_total_work (
		calendar, mrp_day_get_work ()) / (60*60);
	
	str = planner_format_duration (duration, hours_per_day);

	g_object_set (cell, 
		      "text", str,
		      "weight", weight,
		      "editable", editable,
		      NULL);

	g_free (str);
}

static void
task_tree_cost_data_func (GtkTreeViewColumn *tree_column,
			  GtkCellRenderer   *cell,
			  GtkTreeModel      *tree_model,
			  GtkTreeIter       *iter,
			  gpointer           data)
{
	gfloat  cost;
	gchar  *str;
	gint    weight;

	gtk_tree_model_get (tree_model,
			    iter,
			    COL_COST, &cost,
			    COL_WEIGHT, &weight,
			    -1);

	str = planner_format_float (cost, 2, FALSE);

	g_object_set (cell,
		      "text", str,
		      "weight", weight,
		      NULL);

	g_free (str);
}

static void
task_tree_work_data_func (GtkTreeViewColumn *tree_column,
			  GtkCellRenderer   *cell,
			  GtkTreeModel      *tree_model,
			  GtkTreeIter       *iter,
			  gpointer           data)
{
	PlannerTaskTree *tree;
	gint             work;
	gint             hours_per_day;
	MrpTask         *task;
	MrpTaskType      type;
	gint             weight;
	gboolean         editable;

	g_return_if_fail (PLANNER_IS_TASK_TREE (data));
	tree = PLANNER_TASK_TREE (data);

	hours_per_day = mrp_calendar_day_get_total_work (
		mrp_project_get_calendar (tree->priv->project),
		mrp_day_get_work ()) / (60*60);

	/* FIXME */
	if (hours_per_day == 0) {
		hours_per_day = 8;
	}

	gtk_tree_model_get (tree_model,
			    iter,
			    COL_WORK, &work,
			    COL_TASK, &task,
			    COL_WEIGHT, &weight,
			    COL_EDITABLE, &editable,
			    -1);

	g_object_get (task, "type", &type, NULL);

	g_object_set (cell, 
		      "weight", weight,
		      "editable", editable,
		      NULL);

	if (type == MRP_TASK_TYPE_MILESTONE) {
		g_object_set (cell, "text", "", NULL);
	} else {
		gchar *str = planner_format_duration (work, hours_per_day);
		g_object_set (cell, "text", str, NULL);
		g_free (str);
	}
}

static void
task_tree_slack_data_func (GtkTreeViewColumn *tree_column,
			   GtkCellRenderer   *cell,
			   GtkTreeModel      *tree_model,
			   GtkTreeIter       *iter,
			   gpointer           data)
{
	PlannerTaskTree *tree = data;
	gint        slack;
	gint        hours_per_day;
	gchar      *str;
	gint        weight;

	hours_per_day = mrp_calendar_day_get_total_work (
		mrp_project_get_calendar (tree->priv->project),
		mrp_day_get_work ()) / (60*60);

	/* FIXME */
	if (hours_per_day == 0) {
		hours_per_day = 8;
	}
	
	gtk_tree_model_get (tree_model, iter,
			    COL_SLACK, &slack,
			    COL_WEIGHT, &weight,
			    -1);
	
	str = planner_format_duration (slack, hours_per_day);

	g_object_set (cell, 
		      "text", str,
		      "weight", weight, 
		      NULL);
	g_free (str);
}

static void
task_tree_name_edited (GtkCellRendererText *cell,
		       gchar               *path_string,
		       gchar               *new_text,
		       gpointer             data)
{
	GtkTreeView  *view;
	GtkTreeModel *model;
	GtkTreePath  *path;
	GtkTreeIter   iter;
	MrpTask      *task;
	GValue        value = { 0 };

	view = GTK_TREE_VIEW (data);
	model = gtk_tree_view_get_model (view);

	path = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (model, &iter, path);

	gtk_tree_model_get (model, &iter, 
			    COL_TASK, &task,
			    -1);
	
	g_value_init (&value, G_TYPE_STRING);
	g_value_set_string (&value, new_text);
	
	task_cmd_edit_property (PLANNER_TASK_TREE (view),
				task,
				"name",
				&value);
	
	//g_object_set (task, "name", new_text, NULL);
		
	gtk_tree_path_free (path);
}

static void
task_tree_start_edited (GtkCellRendererText *cell,
			gchar               *path_string,
			gchar               *new_text,
			gpointer             data)
{
	PlannerCellRendererDate *date;
	GtkTreeView        *view;
	GtkTreeModel       *model;
	GtkTreePath        *path;
	GtkTreeIter         iter;
	MrpTask            *task;
	MrpConstraint       constraint;

	view = GTK_TREE_VIEW (data);
	model = gtk_tree_view_get_model (view);
	date = PLANNER_CELL_RENDERER_DATE (cell);

	path = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (model, &iter, path);

	gtk_tree_model_get (model, &iter, 
			    COL_TASK, &task,
			    -1);

	constraint.time = date->time;
	constraint.type = date->type;
	
	g_object_set (task, "constraint", &constraint, NULL);

	gtk_tree_path_free (path);
}

static void
task_tree_start_show_popup (PlannerCellRendererDate *cell,
			    const gchar        *path_string,
			    gint                x1,
			    gint                y1,
			    gint                x2,
			    gint                y2,
			    GtkTreeView        *tree_view)
{
	GtkTreeModel  *model;
	GtkTreeIter    iter;
	GtkTreePath   *path;
	MrpTask       *task;
	mrptime        start;
	MrpConstraint *constraint;

	model = gtk_tree_view_get_model (tree_view);
	
	path = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, 
			    COL_TASK, &task,
			    -1);

	g_object_get (G_OBJECT (task),
		      "constraint", &constraint,
		      NULL);

	cell->type = constraint->type;
	
	if (cell->type == MRP_CONSTRAINT_ASAP) {
		g_object_get (G_OBJECT (task),
			      "start", &start,
			      NULL);
		
		cell->time = start;
	} else {
		cell->time = constraint->time;
	}

	g_free (constraint);
	gtk_tree_path_free (path);
}

static void
task_tree_property_date_show_popup (PlannerCellRendererDate *cell,
				    const gchar        *path_string,
				    gint                x1,
				    gint                y1,
				    gint                x2,
				    gint                y2,
				    GtkTreeView        *tree_view)
{
	
	if (cell->time == MRP_TIME_INVALID) {
		cell->time = mrp_time_current_time ();
	}
}

static void
task_tree_duration_edited (GtkCellRendererText *cell,
			   gchar               *path_string,
			   gchar               *new_text,
			   gpointer             data)
{
	PlannerTaskTree   *tree = data;
	GtkTreeView  *view = data;
	GtkTreeModel *model;
	GtkTreePath  *path;
	GtkTreeIter   iter;
	gfloat        flt;
	gint          duration;
	gint          seconds_per_day;
	gchar        *ptr;
	MrpTask      *task;
	
	model = gtk_tree_view_get_model (view);
	path = gtk_tree_path_new_from_string (path_string);	
	gtk_tree_model_get_iter (model, &iter, path);

	seconds_per_day = mrp_calendar_day_get_total_work (
		mrp_project_get_calendar (tree->priv->project),
		mrp_day_get_work ());
		
	flt = g_ascii_strtod (new_text, &ptr);
	if (ptr != NULL) {
		duration = flt * seconds_per_day;
		gtk_tree_model_get (model, &iter, 
				    COL_TASK, &task,
				    -1);
		g_object_set (task, "duration", duration, NULL);
	}
	
	gtk_tree_path_free (path);
}

static void
task_tree_work_edited (GtkCellRendererText *cell,
		       gchar               *path_string,
		       gchar               *new_text,
		       gpointer             data)
{
	GtkTreeView  *view;
	GtkTreeModel *model;
	GtkTreePath  *path;
	GtkTreeIter   iter;
	gint          work;
	MrpTask      *task;
	
	view = GTK_TREE_VIEW (data);
	
	model = gtk_tree_view_get_model (view);
	path = gtk_tree_path_new_from_string (path_string);	
	gtk_tree_model_get_iter (model, &iter, path);

	work = task_tree_parse_time_string (PLANNER_TASK_TREE (view), new_text);
	
	gtk_tree_model_get (model, &iter, 
			    COL_TASK, &task,
			    -1);
	g_object_set (task, "work", work, NULL);
	
	gtk_tree_path_free (path);
}

static void
task_tree_property_data_func (GtkTreeViewColumn *tree_column,
			      GtkCellRenderer   *cell,
			      GtkTreeModel      *tree_model,
			      GtkTreeIter       *iter,
			      gpointer           data)
{
	MrpObject       *object;
	MrpProperty     *property = data;
	MrpPropertyType  type;
	gchar           *svalue;
	gint             ivalue;
	gfloat           fvalue;
	mrptime          tvalue;
	gint             work;

	gtk_tree_model_get (tree_model,
			    iter,
			    COL_TASK,
			    &object,
			    -1);

	/* FIXME: implement mrp_object_get_property like
	 * g_object_get_property that takes a GValue. 
	 */
	type = mrp_property_get_property_type (property);

	switch (type) {
	case MRP_PROPERTY_TYPE_STRING:
		mrp_object_get (object,
				mrp_property_get_name (property), &svalue,
				NULL);
		
		if (svalue == NULL) {
			svalue = g_strdup ("");
		}		

		break;
	case MRP_PROPERTY_TYPE_INT:
		mrp_object_get (object,
				mrp_property_get_name (property), &ivalue,
				NULL);
		svalue = g_strdup_printf ("%d", ivalue);
		break;

	case MRP_PROPERTY_TYPE_FLOAT:
		mrp_object_get (object,
				mrp_property_get_name (property), &fvalue,
				NULL);

		svalue = planner_format_float (fvalue, 4, FALSE);
		break;

	case MRP_PROPERTY_TYPE_DATE:
		mrp_object_get (object,
				mrp_property_get_name (property), &tvalue,
				NULL); 
		svalue = planner_format_date (tvalue);
		break;
		
	case MRP_PROPERTY_TYPE_DURATION:
		mrp_object_get (object,
				mrp_property_get_name (property), &ivalue,
				NULL); 

/*		work = mrp_calendar_day_get_total_work (
		mrp_project_get_calendar (tree->priv->project),
		mrp_day_get_work ());
*/
		work = 8*60*60;

		svalue = planner_format_duration (ivalue, work / (60*60));
		break;
		
	case MRP_PROPERTY_TYPE_COST:
		mrp_object_get (object,
				mrp_property_get_name (property), &fvalue,
				NULL); 

		svalue = planner_format_float (fvalue, 2, FALSE);
		break;
				
	default:
		g_warning ("Type not implemented.");
		break;
	}

	g_object_set (cell, "text", svalue, NULL);
	g_free (svalue);
}

static void  
task_tree_property_value_edited (GtkCellRendererText *cell, 
				 gchar               *path_str,
				 gchar               *new_text, 
				 ColPropertyData     *data)
{
	GtkTreePath        *path;
	GtkTreeIter         iter;
	GtkTreeModel       *model;
	MrpProperty        *property;
	MrpPropertyType     type;
	MrpTask            *task;
	PlannerCellRendererDate *date;	
	gfloat              fvalue;
	
	model = gtk_tree_view_get_model (data->tree);
	property = data->property;

	path = gtk_tree_path_new_from_string (path_str);
	gtk_tree_model_get_iter (model, &iter, path);

	task = planner_gantt_model_get_task (PLANNER_GANTT_MODEL (model), &iter);

	/* FIXME: implement mrp_object_set_property like
	 * g_object_set_property that takes a GValue. 
	 */
	type = mrp_property_get_property_type (property);

	switch (type) {
	case MRP_PROPERTY_TYPE_STRING:
		mrp_object_set (MRP_OBJECT (task),
				mrp_property_get_name (property), 
				new_text,
				NULL);
		break;
	case MRP_PROPERTY_TYPE_INT:
		mrp_object_set (MRP_OBJECT (task),
				mrp_property_get_name (property), 
				atoi (new_text),
				NULL);
		break;
	case MRP_PROPERTY_TYPE_FLOAT:
		fvalue = g_ascii_strtod (new_text, NULL);
		mrp_object_set (MRP_OBJECT (task),
				mrp_property_get_name (property), 
				fvalue,
				NULL);
		break;

	case MRP_PROPERTY_TYPE_DURATION:
		/* FIXME: support reading units etc... */
		mrp_object_set (MRP_OBJECT (task),
				mrp_property_get_name (property), 
				atoi (new_text) *8*60*60,
				NULL);
		break;
		

	case MRP_PROPERTY_TYPE_DATE:
		date = PLANNER_CELL_RENDERER_DATE (cell);
		mrp_object_set (MRP_OBJECT (task),
				mrp_property_get_name (property), 
				&(date->time),
				NULL);
		break;
	case MRP_PROPERTY_TYPE_COST:
		fvalue = g_ascii_strtod (new_text, NULL);
		mrp_object_set (MRP_OBJECT (task),
				mrp_property_get_name (property), 
				fvalue,
				NULL);
		break;	
				
	default:
		g_assert_not_reached ();
		break;
	}

	gtk_tree_path_free (path);
}

static void
task_tree_property_added (MrpProject  *project,
			  GType        object_type,
			  MrpProperty *property,
			  PlannerTaskTree  *task_tree)
{
	GtkTreeView       *tree;
	PlannerTaskTreePriv    *priv;
	MrpPropertyType    type;
	GtkTreeViewColumn *col;	
	GtkCellRenderer   *cell;
	ColPropertyData   *data;

	priv = task_tree->priv;
	
	tree = GTK_TREE_VIEW (task_tree);

	data = g_new0 (ColPropertyData, 1);

	type = mrp_property_get_property_type (property);

	/* The costs are edited in resources view 
	   if (type == MRP_PROPERTY_TYPE_COST) {
	   return;
	   } */

	if (object_type != MRP_TYPE_TASK) {
		return;
	}
	
	if (type == MRP_PROPERTY_TYPE_DATE) {
		cell = planner_cell_renderer_date_new (FALSE);
		g_signal_connect (cell,
				  "show_popup",
				  G_CALLBACK (task_tree_property_date_show_popup),
				  tree);
	} else {
		cell = gtk_cell_renderer_text_new ();			
	}

	g_object_set (cell, "editable", TRUE, NULL);

	g_signal_connect_data (cell,
			       "edited",
			       G_CALLBACK (task_tree_property_value_edited),
			       data,
			       (GClosureNotify) g_free,
			       0);

	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_column_set_title (col, 
					mrp_property_get_label (property));

	g_hash_table_insert (priv->property_to_column, property, col);
	
	data->property = property;
	data->tree = tree;

	gtk_tree_view_column_pack_start (col, cell, TRUE);

	gtk_tree_view_column_set_cell_data_func (col,
						 cell,
						 task_tree_property_data_func,
						 property,
						 NULL);
	g_object_set_data (G_OBJECT (col),
			   "data-func", task_tree_property_data_func);
	g_object_set_data (G_OBJECT (col),
			   "user-data", property);

	gtk_tree_view_append_column (tree, col);
}

static void
task_tree_property_removed (MrpProject  *project,
			    MrpProperty *property,
			    PlannerTaskTree  *task_tree)
{
	PlannerTaskTreePriv    *priv;
	GtkTreeViewColumn *col;

	priv = task_tree->priv;
	
	col = g_hash_table_lookup (priv->property_to_column, property);
	if (col) {
		g_hash_table_remove (priv->property_to_column, property);

		gtk_tree_view_remove_column (GTK_TREE_VIEW (task_tree), col);
	}
}

void
planner_task_tree_set_model (PlannerTaskTree *tree,
			     PlannerGanttModel *model)
{
	gtk_tree_view_set_model (GTK_TREE_VIEW (tree),
				 GTK_TREE_MODEL (model));

	g_signal_connect (model,
			  "row-inserted",
			  G_CALLBACK (task_tree_row_inserted),
			  tree);

	g_signal_connect (model,
			  "task-added",
			  G_CALLBACK (task_tree_task_added_cb),
			  tree);
	
	g_signal_connect (model,
			  "task-removed",
			  G_CALLBACK (task_tree_task_removed_cb),
			  tree);

	gtk_tree_view_expand_all (GTK_TREE_VIEW (tree));
}

static void
task_tree_setup_tree_view (GtkTreeView  *tree,
			   MrpProject   *project,
			   PlannerGanttModel *model)
{
	PlannerTaskTree       *task_tree;
	GtkTreeSelection *selection;

	task_tree = PLANNER_TASK_TREE (tree);
	
	planner_task_tree_set_model (task_tree, model);

	gtk_tree_view_set_rules_hint (tree, TRUE);
	gtk_tree_view_set_reorderable (tree, TRUE);
	
	g_signal_connect (tree,
			  "popup_menu",
			  G_CALLBACK (task_tree_tree_view_popup_menu),
			  tree);
	
	g_signal_connect (tree,
			  "button_press_event",
			  G_CALLBACK (task_tree_tree_view_button_press_event),
			  tree);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
	g_signal_connect (selection, "changed",
			  G_CALLBACK (task_tree_selection_changed_cb),
			  tree);
	
	if (task_tree->priv->custom_properties) {
		g_signal_connect (project,
				  "property_added",
				  G_CALLBACK (task_tree_property_added),
				  tree);
		
		g_signal_connect (project,
				  "property_removed",
				  G_CALLBACK (task_tree_property_removed),
				  tree);
	}
}

static void
task_tree_add_column (GtkTreeView *tree,
		      gint         column,
		      const gchar *title)
{
	GtkTreeViewColumn *col;
	GtkCellRenderer   *cell;

	switch (column) {
	case COL_NAME:
		cell = gtk_cell_renderer_text_new ();
		g_object_set (cell, "editable", TRUE, NULL);
		g_signal_connect (cell,
				  "edited",
				  G_CALLBACK (task_tree_name_edited),
				  tree);

		col = gtk_tree_view_column_new_with_attributes (title,
								cell,
								NULL);
		gtk_tree_view_column_set_cell_data_func (col,
							 cell,
							 task_tree_name_data_func,
							 NULL, NULL);
		g_object_set_data (G_OBJECT (col),
				   "data-func", task_tree_name_data_func);
		
		gtk_tree_view_column_set_resizable (col, TRUE);
		gtk_tree_view_column_set_min_width (col, 100);
		gtk_tree_view_append_column (tree, col);
		break;

	case COL_START:
		cell = planner_cell_renderer_date_new (TRUE);
		g_signal_connect (cell,
				  "edited",
				  G_CALLBACK (task_tree_start_edited),
				  tree);
		g_signal_connect (cell,
				  "show-popup",
				  G_CALLBACK (task_tree_start_show_popup),
				  tree);
		
		col = gtk_tree_view_column_new_with_attributes (title,
								cell,
								NULL);
		gtk_tree_view_column_set_resizable (col, TRUE);
		gtk_tree_view_column_set_min_width (col, 70);
		gtk_tree_view_column_set_cell_data_func (col,
							 cell,
							 task_tree_start_data_func,
							 NULL, NULL);
		g_object_set_data (G_OBJECT (col),
				   "data-func", task_tree_start_data_func);
		
		gtk_tree_view_append_column (tree, col);
		break;
		
	case COL_DURATION:
		cell = gtk_cell_renderer_text_new ();
		col = gtk_tree_view_column_new_with_attributes (title,
								cell,
								NULL);
		gtk_tree_view_column_set_resizable (col, TRUE);
		gtk_tree_view_column_set_cell_data_func (col,
							 cell,
							 task_tree_duration_data_func,
							 NULL,
							 NULL);
		g_object_set_data (G_OBJECT (col),
				   "data-func", task_tree_duration_data_func);
		g_signal_connect (cell,
				  "edited",
				  G_CALLBACK (task_tree_duration_edited),
				  tree);
		
		gtk_tree_view_append_column (tree, col);
		break;

	case COL_WORK:
		cell = gtk_cell_renderer_text_new ();
		col = gtk_tree_view_column_new_with_attributes (title,
								cell,
								NULL);
		gtk_tree_view_column_set_resizable (col, TRUE);
		gtk_tree_view_column_set_cell_data_func (col,
							 cell,
							 task_tree_work_data_func,
							 tree,
							 NULL);
		g_object_set_data (G_OBJECT (col),
				   "data-func", task_tree_work_data_func);
		g_object_set_data (G_OBJECT (col),
				   "user-data", tree);

		g_signal_connect (cell,
				  "edited",
				  G_CALLBACK (task_tree_work_edited),
				  tree);
		
		gtk_tree_view_append_column (tree, col);
		break;
		
	case COL_SLACK:
		cell = gtk_cell_renderer_text_new ();
		col = gtk_tree_view_column_new_with_attributes (title,
								cell,
								NULL);
		gtk_tree_view_column_set_resizable (col, TRUE);
		gtk_tree_view_column_set_cell_data_func (col,
							 cell,
							 task_tree_slack_data_func,
							 tree,
							 NULL);
		g_object_set_data (G_OBJECT (col),
				   "data-func", task_tree_slack_data_func);
		g_object_set_data (G_OBJECT (col),
				   "user-data", tree);
		
		gtk_tree_view_append_column (tree, col);
		break;
		
	case COL_FINISH:
		cell = planner_cell_renderer_date_new (FALSE);
		/*g_signal_connect (cell,
		  "edited",
		  G_CALLBACK (task_tree_start_edited),
		  tree);*/
		g_signal_connect (cell,
				  "show-popup",
				  G_CALLBACK (task_tree_start_show_popup),
				  tree);
		
		col = gtk_tree_view_column_new_with_attributes (title,
								cell,
								NULL);
		gtk_tree_view_column_set_resizable (col, TRUE);
		gtk_tree_view_column_set_min_width (col, 70);
		gtk_tree_view_column_set_cell_data_func (col,
							 cell,
							 task_tree_finish_data_func,
							 NULL, NULL);
		g_object_set_data (G_OBJECT (col),
				   "data-func", task_tree_finish_data_func);
		gtk_tree_view_append_column (tree, col);
		break;

	case COL_COST:
		cell = gtk_cell_renderer_text_new ();
		col = gtk_tree_view_column_new_with_attributes (title,
								cell,
								NULL);
		gtk_tree_view_column_set_resizable (col, TRUE);
		gtk_tree_view_column_set_cell_data_func (col,
							 cell,
							 task_tree_cost_data_func,
							 NULL,
							 NULL);
		g_object_set_data (G_OBJECT (col),
				   "data-func", task_tree_cost_data_func);
		gtk_tree_view_append_column (tree, col);
		break;

	default:
		g_assert_not_reached ();
	}
}

static char *
task_tree_item_factory_trans (const char *path, gpointer data)
{
	return _((gchar*)path);
}

GtkWidget *
planner_task_tree_new (PlannerWindow *main_window,
		       PlannerGanttModel *model, 
		       gboolean      custom_properties,
		       gpointer      first_column,
		       ...)
{
	MrpProject     *project;
	PlannerTaskTree     *tree;
	PlannerTaskTreePriv *priv;
	va_list         args;
	gpointer        str;
	gint            col;
	
	tree = g_object_new (PLANNER_TYPE_TASK_TREE, NULL);

	project = planner_window_get_project (main_window);
	
	priv = tree->priv;

	priv->custom_properties = custom_properties;
	priv->main_window = main_window;
	priv->project = project;

	task_tree_setup_tree_view (GTK_TREE_VIEW (tree), project, model);

	va_start (args, first_column);

	col = GPOINTER_TO_INT (first_column);
	while (col != -1) {
		str = va_arg (args, gpointer);

		task_tree_add_column (GTK_TREE_VIEW (tree), col, str);
		
		col = va_arg (args, gint);
	}

	va_end (args);
		
	return GTK_WIDGET (tree);
}

/*
 * Commands
 */

void
planner_task_tree_insert_subtask (PlannerTaskTree *tree)
{
	GtkTreeView  *tree_view;
	PlannerGanttModel *model;
	GtkTreePath  *path;
	MrpTask      *task, *parent;
	GList        *list;
	gint          work;

	list = planner_task_tree_get_selected_tasks (tree);
	if (list == NULL) {
		return;
	}

	parent = list->data;

	work = mrp_calendar_day_get_total_work (
		mrp_project_get_calendar (tree->priv->project),
		mrp_day_get_work ());
	
	task = g_object_new (MRP_TYPE_TASK,
			     "work", work,
			     "duration", work,
			     NULL);

	if (!GTK_WIDGET_HAS_FOCUS (tree)) {
		gtk_widget_grab_focus (GTK_WIDGET (tree));
	}
	
	mrp_project_insert_task (tree->priv->project,
				 parent,
				 -1,
				 task);

	tree_view = GTK_TREE_VIEW (tree);
	
	model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (tree_view));
	
	path = planner_gantt_model_get_path_from_task (model, task);
	
	gtk_tree_view_set_cursor (tree_view,
				  path,
				  NULL,
				  FALSE);
	
	gtk_tree_path_free (path);

	g_list_free (list);
}

void
planner_task_tree_insert_task (PlannerTaskTree *tree)
{
	GtkTreeView   *tree_view;
	PlannerGanttModel  *model;
	GtkTreePath   *path;
	MrpTask       *parent;
	GList         *list;
	gint           work;
	gint           position;
	TaskCmdInsert *cmd;

	list = planner_task_tree_get_selected_tasks (tree);
	if (list == NULL) {
		parent = NULL;
		position = -1;
	}
	else {
		parent = mrp_task_get_parent (list->data);
		position = mrp_task_get_position (list->data) + 1;
	}

	if (parent) {
		model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));

		path = planner_gantt_model_get_path_from_task (model, parent);
		gtk_tree_path_append_index (path, position);
	} else {
		path = gtk_tree_path_new_first ();
	}

	work = mrp_calendar_day_get_total_work (
		mrp_project_get_calendar (tree->priv->project),
		mrp_day_get_work ());
	
	cmd = (TaskCmdInsert*) task_cmd_insert (tree, path, work, work);
	
	if (!GTK_WIDGET_HAS_FOCUS (tree)) {
		gtk_widget_grab_focus (GTK_WIDGET (tree));
	}

	tree_view = GTK_TREE_VIEW (tree);
	
/*	gtk_tree_view_set_cursor (tree_view,
				  path,
				  NULL,
				  FALSE);
*/
	//gtk_tree_path_free (path);

	g_list_free (list);
}

void
planner_task_tree_remove_task (PlannerTaskTree *tree)
{
	GList *list, *l;

	list = planner_task_tree_get_selected_tasks (tree);
	if (list == NULL) {
		return;
	}

	for (l = list; l; l = l->next) {
		mrp_project_remove_task (tree->priv->project, l->data);
	}
	
	g_list_free (list);
}

void
planner_task_tree_edit_task (PlannerTaskTree *tree)
{
	PlannerTaskTreePriv *priv;
	MrpTask        *task;
	GList          *list;
	GtkWidget      *dialog;

	g_return_if_fail (PLANNER_IS_TASK_TREE (tree));
	
	priv = tree->priv;
	
	list = planner_task_tree_get_selected_tasks (tree);
	if (list == NULL) {
		return;
	}

	task = list->data;

	dialog = planner_task_dialog_new (priv->main_window, task);
	gtk_widget_show (dialog);

	g_list_free (list);
}

static void
task_tree_insert_tasks_dialog_destroy_cb (GtkWidget *dialog,
					  GObject   *window)
{
	g_object_set_data (window, "input-tasks-dialog", NULL);
}

void
planner_task_tree_insert_tasks (PlannerTaskTree   *tree)
{
	PlannerTaskTreePriv *priv;
	GtkWidget      *widget;

	g_return_if_fail (PLANNER_IS_TASK_TREE (tree));
	
	priv = tree->priv;

	/* We only want one of these dialogs per main window. */
	widget = g_object_get_data (G_OBJECT (priv->main_window), "input-tasks-dialog");
	if (widget) {
		gtk_window_present (GTK_WINDOW (widget));
		return;
	}

	widget = planner_task_input_dialog_new (priv->project);
	gtk_window_set_transient_for (GTK_WINDOW (widget),
				      GTK_WINDOW (priv->main_window));
	gtk_widget_show (widget);

	g_object_set_data (G_OBJECT (priv->main_window), "input-tasks-dialog", widget);
	
	g_signal_connect_object (widget,
				 "destroy",
				 G_CALLBACK (task_tree_insert_tasks_dialog_destroy_cb),
				 priv->main_window,
				 0);
}

void
planner_task_tree_select_all (PlannerTaskTree *tree)
{
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));

	gtk_tree_selection_select_all (selection);
}

void
planner_task_tree_unlink_task (PlannerTaskTree *tree)
{
	MrpTask     *task;
	GList       *list, *l;
	GList       *relations, *r;
	MrpRelation *relation;

	list = planner_task_tree_get_selected_tasks (tree);
	if (list == NULL) {
		return;
	}

	for (l = list; l; l = l->next) {
		task = l->data;

		relations = g_list_copy (mrp_task_get_predecessor_relations (task));
		for (r = relations; r; r = r->next) {
			relation = r->data;
			
			mrp_task_remove_predecessor (
				task, mrp_relation_get_predecessor (relation));
		}

		g_list_free (relations);
		
		relations = g_list_copy (mrp_task_get_successor_relations (task));
		for (r = relations; r; r = r->next) {
			relation = r->data;
			
			mrp_task_remove_predecessor (
				mrp_relation_get_successor (relation), task);
		}

		g_list_free (relations);
	}
	
	g_list_free (list);
}

void
planner_task_tree_indent_task (PlannerTaskTree *tree)
{
	PlannerGanttModel     *model;
	MrpTask          *task;
	MrpTask          *new_parent;
	MrpTask          *first_task_parent;
	MrpProject       *project;
	GList            *list, *l;
	GList            *indent_tasks = NULL;
	GError           *error = NULL;
	GtkTreePath      *path;
	GtkWidget        *dialog;
	GtkTreeSelection *selection;
				
	project = tree->priv->project;

	model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));
	
	list = planner_task_tree_get_selected_tasks (tree);
	if (list == NULL) {
		return;
	}
	
	task = list->data;
	
	new_parent = planner_gantt_model_get_indent_task_target (model, task);
	if (new_parent == NULL) {
		g_list_free (list);
		return;
	}

	first_task_parent = mrp_task_get_parent (task);

	/* Get a list of tasks that have the same parent as the first one. */
	for (l = list; l; l = l->next) {
		task = l->data;
		
		if (mrp_task_get_parent (task) == first_task_parent) {
			indent_tasks = g_list_prepend (indent_tasks, task);
		}
	}
	g_list_free (list);

	indent_tasks = g_list_reverse (indent_tasks);

	for (l = indent_tasks; l; l = l->next) {
		gboolean success;
		
		task = l->data;

		success = mrp_project_move_task (project,
						 task,
						 NULL,
						 new_parent,
						 FALSE,
						 &error);
		if (!success) {
			dialog = gtk_message_dialog_new (GTK_WINDOW (tree->priv->main_window),
							 GTK_DIALOG_DESTROY_WITH_PARENT,
							 GTK_MESSAGE_ERROR,
							 GTK_BUTTONS_OK,
							 "%s",
							 error->message);
			
			gtk_dialog_run (GTK_DIALOG (dialog));
			gtk_widget_destroy (dialog);
			
			g_clear_error (&error);
		}
	}

	path = planner_gantt_model_get_path_from_task (PLANNER_GANTT_MODEL (model), 
						       indent_tasks->data);

	task_tree_block_selection_changed (tree);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	gtk_tree_selection_select_path (selection, path);
	task_tree_unblock_selection_changed (tree);

	gtk_tree_path_free (path);

	g_list_free (indent_tasks);
}

void
planner_task_tree_unindent_task (PlannerTaskTree *tree)
{
	PlannerGanttModel     *model;
	MrpTask          *task;
	MrpTask          *new_parent;
	MrpTask          *first_task_parent;
	MrpProject       *project;
	GList            *list, *l;
	GList            *unindent_tasks = NULL;
	GtkTreePath      *path;
	GtkTreeSelection *selection;

	project = tree->priv->project;

	model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));
	
	list = planner_task_tree_get_selected_tasks (tree);
	if (list == NULL) {
		return;
	}

	task = list->data;

	new_parent = mrp_task_get_parent (task);
	if (new_parent != NULL) {
		new_parent = mrp_task_get_parent (new_parent);
	}
	if (new_parent == NULL) {
		/* No grandparent to unindent to. */ 
		g_list_free (list);
		return;
	}
	
	first_task_parent = mrp_task_get_parent (task);

	/* Get a list of tasks that have the same parent as the first one. */
	for (l = list; l; l = l->next) {
		task = l->data;
		
		if (mrp_task_get_parent (task) == first_task_parent) {
			unindent_tasks = g_list_prepend (unindent_tasks, task);
		}
	}
	g_list_free (list);

	unindent_tasks = g_list_reverse (unindent_tasks);

	for (l = unindent_tasks; l; l = l->next) {
		task = l->data;

		mrp_project_move_task (project,
				       task,
				       NULL,
				       new_parent,
				       FALSE,
				       NULL);
	}

	path = planner_gantt_model_get_path_from_task (PLANNER_GANTT_MODEL (model), 
						       unindent_tasks->data);

	task_tree_block_selection_changed (tree);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	gtk_tree_selection_select_path (selection, path);
	task_tree_unblock_selection_changed (tree);

	gtk_tree_path_free (path);

	g_list_free (unindent_tasks);
}

void 
planner_task_tree_move_task_up (PlannerTaskTree *tree)
{
	GtkTreeSelection *selection;
	GtkTreeModel	 *model;
	GtkTreePath	 *path;
	MrpProject  	 *project;
	MrpTask	    	 *task, *parent, *sibling;
	GList	    	 *list;
	guint	    	  position;

	project = tree->priv->project;

	task_tree_block_selection_changed (tree);
	
	list = planner_task_tree_get_selected_tasks (tree);

	if (list == NULL) {
		/* Nothing selected */
		return;
	} 

	task = list->data;
	position = mrp_task_get_position (task);
	parent = mrp_task_get_parent (task);
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
	
	if (position == 0) {
		/* Task on the top of the list */
	} else {
		sibling = mrp_task_get_nth_child (parent, 
						  position - 1);
		
		/* Move task from 'position' to 'position-1' */
		mrp_project_move_task (project, task, sibling, 
				       parent, TRUE, NULL);
		path = planner_gantt_model_get_path_from_task (
			PLANNER_GANTT_MODEL (model), task);
		gtk_tree_selection_select_path (selection, path);
	}

	task_tree_unblock_selection_changed (tree);
}

void 
planner_task_tree_move_task_down (PlannerTaskTree *tree)
{
	GtkTreeSelection *selection;
	GtkTreeModel	 *model;
	GtkTreePath	 *path;
	MrpProject 	 *project;
	MrpTask	   	 *task, *parent, *sibling;
	GList		 *list;
	guint		 position;

	project = tree->priv->project;

	task_tree_block_selection_changed (tree);

	list = planner_task_tree_get_selected_tasks (tree);

	if (list == NULL) {
		/* Nothing selected */
		return;
	} else {
		task = list->data;
		position = mrp_task_get_position (task);
		parent = mrp_task_get_parent (task);
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));

		if (position == (mrp_task_get_n_children (parent) - 1) ) {
			/* The task is in the bottom of the list */
		} else {
			sibling = mrp_task_get_nth_child (parent, position + 1);
			/* Moving task from 'position' to 'position + 1' */
			mrp_project_move_task (project, task, sibling, 
					       parent, FALSE, NULL);

			path = planner_gantt_model_get_path_from_task (PLANNER_GANTT_MODEL (model), task);
			gtk_tree_selection_select_path (selection, path);
		}
	}

	task_tree_unblock_selection_changed (tree);
}

void
planner_task_tree_reset_constraint (PlannerTaskTree *tree)
{
	MrpTask *task;
	GList   *list, *l;

	list = planner_task_tree_get_selected_tasks (tree);

	for (l = list; l; l = l->next) {
		task = l->data;

		mrp_task_reset_constraint (task);
	}
	
	g_list_free (list);
}

void
planner_task_tree_reset_all_constraints (PlannerTaskTree *tree)
{
	MrpProject *project;
	MrpTask    *task;
	GList      *list, *l;

	project = tree->priv->project;
		
	list = mrp_project_get_all_tasks (project);
	for (l = list; l; l = l->next) {
		task = l->data;

		mrp_task_reset_constraint (task);
	}
	
	g_list_free (list);
}

static  void
task_tree_get_selected_func (GtkTreeModel *model,
			     GtkTreePath  *path,
			     GtkTreeIter  *iter,
			     gpointer      data)
{
	GList   **list = data;
	MrpTask  *task;

	gtk_tree_model_get (model,
			    iter,
			    COL_TASK, &task,
			    -1);
	
	*list = g_list_prepend (*list, task);
}

GList *
planner_task_tree_get_selected_tasks (PlannerTaskTree *tree)	
{
	GtkTreeSelection *selection;
	GList            *list;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	
	list = NULL;
	gtk_tree_selection_selected_foreach (selection,
					     task_tree_get_selected_func,
					     &list);

	list = g_list_reverse (list);
	
	return list;
}

/* Returns TRUE if one or more of the tasks in the list have links. */
gboolean
planner_task_tree_has_relation (GList *list)
{
	GList   *l;
	MrpTask *task;

	for (l = list; l; l = l->next) {
		task = l->data;

		if (mrp_task_has_relation (task)) {
			return TRUE;
		}
	}

	return FALSE;
}


/* The comments here are for i18n, they get extracted to the po files. */
static Units units[] = {
	{ N_("mon"),     UNIT_MONTH },  /* month unit variant accepted in input */
	{ N_("month"),   UNIT_MONTH },  /* month unit variant accepted in input */
	{ N_("months"),  UNIT_MONTH },  /* month unit variant accepted in input */
	{ N_("w"),       UNIT_WEEK },   /* week unit variant accepted in input */
	{ N_("week"),    UNIT_WEEK },   /* week unit variant accepted in input */
	{ N_("weeks"),   UNIT_WEEK },   /* week unit variant accepted in input */
	{ N_("d"),       UNIT_DAY },    /* day unit variant accepted in input */
	{ N_("day"),     UNIT_DAY },    /* day unit variant accepted in input */
	{ N_("days"),    UNIT_DAY },    /* day unit variant accepted in input */
	{ N_("h"),       UNIT_HOUR },   /* hour unit variant accepted in input */
	{ N_("hour"),    UNIT_HOUR },   /* hour unit variant accepted in input */
	{ N_("hours"),   UNIT_HOUR },   /* hour unit variant accepted in input */
	{ N_("min"),     UNIT_MINUTE }, /* minute unit variant accepted in input */
	{ N_("minute"),  UNIT_MINUTE }, /* minute unit variant accepted in input */
	{ N_("minutes"), UNIT_MINUTE }  /* minute unit variant accepted in input */
};

static Unit
task_tree_get_unit_from_string (const gchar *str)
{
	static Units    *translated_units;
	static gboolean  inited = FALSE;
	Unit             unit = UNIT_NONE;
	gint             i, len;
	gchar           *tmp, *tmp2;

	if (!inited) {
		len = G_N_ELEMENTS (units);

		translated_units = g_new0 (Units, len);
		
		for (i = 0; i < len; i++) {
			tmp = _(units[i].name);

			tmp2 = g_utf8_casefold (tmp, -1);
			/* Not sure this is necessary... */
			tmp = g_utf8_normalize (tmp2, -1, G_NORMALIZE_DEFAULT);

			translated_units[i].name = tmp;
			translated_units[i].unit = units[i].unit;
		}
		
		inited = TRUE;
	}

	
	len = G_N_ELEMENTS (units);
	for (i = 0; i < len; i++) {
		if (!strncmp (str, translated_units[i].name,
			      strlen (translated_units[i].name))) {
			unit = translated_units[i].unit;
		}
	}

	if (unit != UNIT_NONE) {
		return unit;
	}

	/* Try untranslated names as a fallback. */
	for (i = 0; i < len; i++) {
		if (!strncmp (str, units[i].name, strlen (units[i].name))) {
			unit = units[i].unit;
		}
	}

	return unit;
}

static gint
task_tree_multiply_with_unit (gdouble value,
			      Unit    unit,
			      gint    seconds_per_month,
			      gint    seconds_per_week,
			      gint    seconds_per_day)
{
	switch (unit) {
	case UNIT_MONTH:
		value *= seconds_per_month;
		break;
	case UNIT_WEEK:
		value *= seconds_per_week;
		break;
	case UNIT_DAY:
		value *= seconds_per_day;
		break;
	case UNIT_HOUR:
		value *= 60*60;
		break;
	case UNIT_MINUTE:
		value *= 60;
		break;
	case UNIT_NONE:
		return 0;
	}	
	
	return floor (value + 0.5);
}

static gint
task_tree_parse_time_string (PlannerTaskTree  *tree,
			     const gchar *input)
{
	gchar           *tmp;
	gchar           *str;
	gchar           *freeme;
	gchar           *end_ptr;
	gdouble          dbl;
	Unit             unit;
	gint             total;
	gint             seconds_per_month;
	gint             seconds_per_week;
	gint             seconds_per_day;

	seconds_per_day = mrp_calendar_day_get_total_work (
		mrp_project_get_calendar (tree->priv->project),
		mrp_day_get_work ());

	/* Hardcode these for now. */
	seconds_per_week = seconds_per_day * 5;
	seconds_per_month = seconds_per_day * 30;

	tmp = g_utf8_casefold (input, -1);
	/* Not sure this is necessary... */
	str = g_utf8_normalize (tmp, -1, G_NORMALIZE_DEFAULT);
	g_free (tmp);

	freeme = str;

	if (!str) {
		return 0;
	}
	
	total = 0;
	while (*str) {
		while (*str && g_unichar_isalpha (g_utf8_get_char (str))) {   
			str = g_utf8_next_char (str);
		}

		if (*str == 0) {
			break;
		}

		dbl = g_strtod (str, &end_ptr);

		if (end_ptr == str) {
			break;
		}
		
		if (end_ptr) {
			unit = task_tree_get_unit_from_string (end_ptr);

			/* If no unit was specified and it was the first number
			 * in the input, treat it as Day.
			 */
			if (unit == UNIT_NONE && str == freeme) {
				unit = UNIT_DAY;
			}

			total += task_tree_multiply_with_unit (dbl,
							       unit,
							       seconds_per_month,
							       seconds_per_week,
							       seconds_per_day);
		}

		if (*end_ptr == 0) {
			break;
		}
		
		str = end_ptr + 1;
	}

	g_free (freeme);
	
	return total;
}

static MrpProject *
task_tree_get_project (PlannerTaskTree *tree)
{
	return planner_window_get_project (tree->priv->main_window);
}

static MrpTask *
task_tree_get_task_from_path (PlannerTaskTree *tree,
			      GtkTreePath     *path)
{
	PlannerGanttModel   *model;
	MrpTask             *task;
	
	model = PLANNER_GANTT_MODEL (gtk_tree_view_get_model (GTK_TREE_VIEW (tree)));
	task = planner_gantt_model_get_task_from_path (model, path);

	return task;
}
