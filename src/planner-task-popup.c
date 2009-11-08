/*
 * Copyright (C) 2005 Imendio AB
 * Copyright (C) 2004 Chris Ladd <caladd@particlestorm.net>
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
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "libplanner/mrp-paths.h"
#include "planner-task-tree.h"
#include "planner-task-popup.h"


static void task_popup_insert_task_cb         (gpointer   callback_data,
					       guint      action,
					       GtkWidget *widget);
static void task_popup_insert_subtask_cb      (gpointer   callback_data,
					       guint      action,
					       GtkWidget *widget);
static void task_popup_remove_task_cb         (gpointer   callback_data,
					       guint      action,
					       GtkWidget *widget);
static void task_popup_edit_task_cb           (gpointer   callback_data,
					       guint      action,
					       GtkWidget *widget);
static void task_popup_edit_task_resources_cb (gpointer   callback_data,
					       guint      action,
					       GtkWidget *widget);
static void task_popup_unlink_task_cb         (gpointer   callback_data,
					       guint      action,
					       GtkWidget *widget);


#define GIF_CB(x) ((GtkItemFactoryCallback)(x))

static GtkItemFactoryEntry popup_menu_items[] = {
	{ N_("/_Insert task"), NULL, GIF_CB (task_popup_insert_task_cb),
	  PLANNER_TASK_POPUP_INSERT, "<Item>", NULL
	},
	{ N_("/Insert _subtask"), NULL, GIF_CB (task_popup_insert_subtask_cb),
	  PLANNER_TASK_POPUP_SUBTASK, "<Item>", NULL
	},
	{ N_("/_Remove task"), NULL, GIF_CB (task_popup_remove_task_cb),
	  PLANNER_TASK_POPUP_REMOVE, "<StockItem>", GTK_STOCK_DELETE
	},
	{ "/sep1", NULL, 0, PLANNER_TASK_POPUP_NONE, "<Separator>"
	},
	{ N_("/_Unlink task"), NULL, GIF_CB (task_popup_unlink_task_cb),
	  PLANNER_TASK_POPUP_UNLINK, "<Item>", NULL
	},
	{ "/sep2", NULL, 0, PLANNER_TASK_POPUP_NONE, "<Separator>"
	},
	{ N_("/Assign _resources..."), NULL, GIF_CB (task_popup_edit_task_resources_cb),
	  PLANNER_TASK_POPUP_EDIT_RESOURCES,  "<Item>",   NULL
	},
	{ N_("/_Edit task..."), NULL, GIF_CB (task_popup_edit_task_cb),
	  PLANNER_TASK_POPUP_EDIT_TASK, "<Item>", NULL
	}
};

static char *
task_tree_item_factory_trans (const char *path, gpointer data)
{
	return _((gchar *)path);
}

static void
task_popup_insert_task_cb (gpointer   callback_data,
			   guint      action,
			   GtkWidget *widget)
{
	planner_task_tree_insert_task (callback_data);
}

static void
task_popup_insert_subtask_cb (gpointer   callback_data,
			      guint      action,
			      GtkWidget *widget)
{
	planner_task_tree_insert_subtask (callback_data);
}

static void
task_popup_remove_task_cb (gpointer   callback_data,
			   guint      action,
			   GtkWidget *widget)
{
	planner_task_tree_remove_task (callback_data);
}

static void
task_popup_edit_task_cb (gpointer   callback_data,
			 guint      action,
			 GtkWidget *widget)
{
	planner_task_tree_edit_task (callback_data,
				     PLANNER_TASK_DIALOG_PAGE_GENERAL);
}

static void
task_popup_edit_task_resources_cb (gpointer   callback_data,
				   guint      action,
				   GtkWidget *widget)
{
	planner_task_tree_edit_task (callback_data,
				     PLANNER_TASK_DIALOG_PAGE_RESOURCES);
}

static void
task_popup_unlink_task_cb (gpointer callback_data, guint action,
			   GtkWidget *widget)
{
	planner_task_tree_unlink_task (callback_data);
}

GtkItemFactory *
planner_task_popup_new (PlannerTaskTree *tree)
{
	GtkItemFactory *item_factory;

	item_factory = gtk_item_factory_new (GTK_TYPE_MENU, "<main>", NULL);
	gtk_item_factory_set_translate_func (item_factory,
					     task_tree_item_factory_trans,
					     NULL, NULL);

	gtk_item_factory_create_items (item_factory,
				       G_N_ELEMENTS (popup_menu_items),
				       popup_menu_items, tree);

	return item_factory;
}

static void
task_popup_set_sensitive (GtkItemFactory *factory, gint id, gboolean sensitive)
{
	GtkWidget *widget;

	widget = gtk_item_factory_get_widget_by_action (factory, id);
	gtk_widget_set_sensitive (widget, sensitive);
}

void
planner_task_popup_update_sensitivity (GtkItemFactory *factory,
				       GList          *tasks)
{
	gint          length;
	MrpTask      *task;
	MrpTaskType   type;
	gboolean      milestone;

	length = g_list_length (tasks);

	/* Can always insert task. */
	task_popup_set_sensitive (factory, PLANNER_TASK_POPUP_INSERT, TRUE);

	/* Nothing else when nothing is selected. */
	if (length == 0) {
		task_popup_set_sensitive (factory, PLANNER_TASK_POPUP_SUBTASK, FALSE);
		task_popup_set_sensitive (factory, PLANNER_TASK_POPUP_REMOVE, FALSE);
		task_popup_set_sensitive (factory, PLANNER_TASK_POPUP_UNLINK, FALSE);
		task_popup_set_sensitive (factory, PLANNER_TASK_POPUP_EDIT_TASK, FALSE);
		task_popup_set_sensitive (factory, PLANNER_TASK_POPUP_EDIT_RESOURCES, FALSE);
		return;
	}

	/* Can only insert subtask when one !milestone is selected. */
	if (length == 1) {
		task = tasks->data;

		type = mrp_task_get_task_type (task);
		milestone = (type == MRP_TASK_TYPE_MILESTONE);
		task_popup_set_sensitive (factory, PLANNER_TASK_POPUP_SUBTASK, !milestone);
	} else {
		task_popup_set_sensitive (factory, PLANNER_TASK_POPUP_SUBTASK, FALSE);
	}

	/* The rest are always sensitive when one more more tasks are selected. */
	task_popup_set_sensitive (factory, PLANNER_TASK_POPUP_REMOVE, TRUE);
	task_popup_set_sensitive (factory, PLANNER_TASK_POPUP_UNLINK, TRUE);
	task_popup_set_sensitive (factory, PLANNER_TASK_POPUP_EDIT_TASK, TRUE);
	task_popup_set_sensitive (factory, PLANNER_TASK_POPUP_EDIT_RESOURCES, TRUE);
}
