 /* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002-2003 CodeFactory AB
 * Copyright (C) 2002-2003 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2002-2004 Alvaro del Castillo <acs@barrapunto.com>
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
#include <time.h>
#include <string.h>
#include <stdlib.h> /* for atoi */
#include <libgnome/gnome-i18n.h>
#include <glade/glade.h>
#include <gtk/gtk.h>
#include <libplanner/mrp-object.h>
#include <libplanner/mrp-project.h>
#include "planner-cell-renderer-list.h"
#include "planner-assignment-model.h"
#include "planner-predecessor-model.h"
#include "planner-task-dialog.h"

/* FIXME: This should be read from the option menu when that works */
#define WORK_MULTIPLIER (60*60*8.0)

typedef struct {
	PlannerWindow  *main_window;
	MrpTask       *task;
	GtkWidget     *dialog;
	GtkWidget     *predecessor_list;
	GtkWidget     *resource_list;
	GtkWidget     *name_entry;
	GtkWidget     *milestone_checkbutton;
	GtkWidget     *fixed_checkbutton;
	GtkWidget     *work_spinbutton;
	GtkWidget     *duration_spinbutton;
	GtkWidget     *complete_spinbutton;
	GtkWidget     *priority_spinbutton;
	GtkWidget     *note_textview;
	GtkTextBuffer *note_buffer;
} DialogData;


static void  task_dialog_close_clicked_cb           (GtkWidget           *w, 
						     DialogData          *data);
static void  task_dialog_task_removed_cb            (MrpObject           *object,
						     GtkWidget           *dialog);
static void  task_dialog_task_name_changed_cb       (MrpTask             *task, 
						     GParamSpec          *pspec, 
						     GtkWidget           *dialog);
static void  task_dialog_name_changed_cb            (GtkWidget           *w, 
						     DialogData          *data);
static gboolean task_dialog_name_focus_in_cb        (GtkWidget           *w,
						     GdkEventFocus       *event,
						     DialogData          *data);
static gboolean task_dialog_name_focus_out_cb       (GtkWidget           *w,
						     GdkEventFocus       *event,
						     DialogData          *data);
static void  task_dialog_task_type_changed_cb       (MrpTask             *task,
						     GParamSpec          *pspec,
						     GtkWidget           *dialog);
static void  task_dialog_type_toggled_cb            (GtkWidget           *w,
						     DialogData          *data);
static void  task_dialog_task_sched_changed_cb      (MrpTask             *task,
						     GParamSpec          *pspec,
						     GtkWidget           *dialog);
static void  task_dialog_fixed_toggled_cb           (GtkWidget           *w,
						     DialogData          *data);
static void  task_dialog_task_work_changed_cb       (MrpTask             *task,
						     GParamSpec          *pspec,
						     GtkWidget           *dialog);
static void  task_dialog_work_changed_cb            (GtkWidget           *w, 
						     DialogData          *data);
static gboolean task_dialog_work_focus_in_cb        (GtkWidget           *w,
						     GdkEventFocus       *event,
						     DialogData          *data);
static gboolean task_dialog_work_focus_out_cb       (GtkWidget           *w,
						     GdkEventFocus       *event,
						     DialogData          *data);
static void  task_dialog_task_duration_changed_cb   (MrpTask             *task,
						     GParamSpec          *pspec,
						     GtkWidget           *dialog);
static void  task_dialog_duration_changed_cb        (GtkWidget           *w, 
						     DialogData          *data);
static gboolean task_dialog_duration_focus_in_cb    (GtkWidget           *w,
						     GdkEventFocus       *event,
						     DialogData          *data);
static gboolean task_dialog_duration_focus_out_cb   (GtkWidget           *w,
						     GdkEventFocus       *event,
						     DialogData          *data);
static void  task_dialog_task_complete_changed_cb   (MrpTask             *task, 
						     GParamSpec          *pspec,
						     GtkWidget           *dialog);
static void  task_dialog_complete_changed_cb        (GtkWidget           *w, 
						     DialogData          *data);
static gboolean task_dialog_complete_focus_in_cb    (GtkWidget           *w,
						     GdkEventFocus       *event,
						     DialogData          *data);
static gboolean task_dialog_complete_focus_out_cb   (GtkWidget           *w,
						     GdkEventFocus       *event,
						     DialogData          *data);
static void  task_dialog_task_priority_changed_cb   (MrpTask             *task, 
						     GParamSpec          *pspec,
						     GtkWidget           *dialog);
static void  task_dialog_priority_changed_cb        (GtkWidget           *w, 
						     DialogData          *data);
static gboolean task_dialog_priority_focus_in_cb    (GtkWidget           *w,
						     GdkEventFocus       *event,
						     DialogData          *data);
static gboolean task_dialog_priority_focus_out_cb   (GtkWidget           *w,
						     GdkEventFocus       *event,
						     DialogData          *data);
static void  task_dialog_task_note_changed_cb       (MrpTask             *task, 
						     GParamSpec          *pspec, 
						     GtkWidget           *dialog);
static void  task_dialog_note_changed_cb            (GtkWidget           *w,
						     DialogData          *data);
static void  task_dialog_note_stamp_cb              (GtkWidget           *w,
						     DialogData          *data);
static void
task_dialog_task_child_added_or_removed_cb          (MrpTask             *task,
						     GtkWidget           *dialog);
static void  task_dialog_setup_widgets              (DialogData          *data,
						     GladeXML            *glade);
static void  task_dialog_assignment_toggled_cb      (GtkCellRendererText *cell,
						     gchar               *path_str,
						     DialogData          *data);
static void  task_dialog_setup_resource_list        (DialogData          *data); 
static void  task_dialog_connect_to_task            (DialogData          *data);
static void  task_dialog_resource_units_cell_edited (GtkCellRendererText *cell, 
						     gchar               *path_str,
						     gchar               *new_text, 
						     DialogData          *data);
static void  task_dialog_pred_cell_edited           (GtkCellRendererText  *cell,
						     gchar                *path_str,
						     gchar                *new_text,
						     DialogData           *data);
static void  task_dialog_cell_type_show_popup       (PlannerCellRendererList   *cell,
						     const gchar          *path_string,
						     gint                  x1,
						     gint                  y1,
						     gint                  x2,
						     gint                  y2,
						     DialogData           *data);
static void  task_dialog_cell_name_show_popup       (PlannerCellRendererList   *cell,
						     const gchar          *path_string,
						     gint                  x1,
						     gint                  y1,
						     gint                  x2,
						     gint                  y2,
						     DialogData           *data);
static void  task_dialog_cell_hide_popup            (PlannerCellRendererList   *cell,
						     GtkWidget            *view);
static void  task_dialog_add_predecessor_cb         (GtkWidget            *widget,
						     DialogData           *data);
static void  task_dialog_remove_predecessor_cb      (GtkWidget            *widget,
						     DialogData           *data);
static void  task_dialog_new_pred_ok_clicked_cb     (GtkWidget            *w, 
						     GtkWidget            *dialog);
static void  task_dialog_new_pred_cancel_clicked_cb (GtkWidget            *w, 
						     GtkWidget            *dialog); 
static void  task_dialog_update_sensitivity         (DialogData           *data);
static void  task_dialog_update_title               (DialogData           *data);


/* Keep the dialogs here so that we can just raise the dialog if it's
 * opened twice for the same task.
 */
static GHashTable *dialogs = NULL;

#define DIALOG_GET_DATA(d) g_object_get_data ((GObject*)d, "data")

typedef struct {
	PlannerCmd base;

	MrpTask *task;
	gchar   *property;  
	GValue  *value;
	GValue  *old_value;
} TaskCmdEditProperty;

typedef struct {
	PlannerCmd base;

	MrpTask     *task;
	MrpTaskType  type;
	MrpTaskType  old_type;
} TaskCmdEditType;

typedef struct {
	PlannerCmd base;

	MrpTask     *task;
	MrpTaskSched sched;
	MrpTaskSched old_sched;
} TaskCmdEditSchedule;


static void
task_dialog_setup_option_menu (GtkWidget     *option_menu,
			       GCallback      func,
			       gpointer       user_data,
			       gconstpointer  str1, ...)
{
	GtkWidget     *menu;
	GtkWidget     *menu_item;
	gint           i;
	va_list        args;
	gconstpointer  str;
	gint           type;

       	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (option_menu));
	if (menu) {
		gtk_widget_destroy (menu);
	}
	
	menu = gtk_menu_new ();

	va_start (args, str1);
	for (str = str1, i = 0; str != NULL; str = va_arg (args, gpointer), i++) {
		menu_item = gtk_menu_item_new_with_label (str);
		gtk_widget_show (menu_item);
		gtk_menu_append (GTK_MENU (menu), menu_item);

		type = va_arg (args, gint);
		
		g_object_set_data (G_OBJECT (menu_item),
				   "data",
				   GINT_TO_POINTER (type));
		if (func) {
			g_signal_connect (menu_item,
					  "activate",
					  func,
					  user_data);
		}
	}
	va_end (args);

	gtk_widget_show (menu);
	gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
}

static gint
task_dialog_option_menu_get_selected (GtkWidget *option_menu)
{
	GtkWidget *menu;
	GtkWidget *item;
	gint       ret;
	
	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (option_menu));
		
	item = gtk_menu_get_active (GTK_MENU (menu));

	ret = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item), "data"));

	return ret;
}	

static void
task_dialog_task_combo_select_child_cb (GtkList   *list,
					GtkWidget *item,
					GtkCombo  *combo)
{
	MrpTask *task;
	gchar   *name;

	task = g_object_get_data (G_OBJECT (item), "task");
	
	g_object_set_data (G_OBJECT (combo), "selected_task", task);

	g_object_get (task,
		      "name", &name,
		      NULL);
}

static void
task_dialog_setup_task_combo (GtkCombo *combo,
			      GList    *tasks)
{
	GList *strings;
	GList *children;
	GList *l;
	gchar *name;

	if (tasks == NULL) {
		return;
	}
	
	strings = NULL;
	for (l = tasks; l; l = l->next) {
		g_object_get (G_OBJECT (l->data),
			      "name", &name,
			      NULL);

		if (name == NULL || name[0] == 0) {
			strings = g_list_prepend (strings,
						  g_strdup (_("(No name)")));
		} else {
			strings = g_list_prepend (strings, name);
		}
	}

	strings = g_list_reverse (strings);

	gtk_combo_set_popdown_strings (combo, strings);

	for (l = strings; l; l = l->next) {
		g_free (l->data);
	}
	g_list_free (strings);
	
	g_object_set_data (G_OBJECT (combo), "selected_task", tasks->data);
	
	children = GTK_LIST (combo->list)->children;
	for (l = children; l; l = l->next) {
		g_object_set_data (G_OBJECT (l->data), "task", tasks->data);
		tasks = tasks->next;
	}
	
	g_signal_connect (combo->list,
			  "select-child",
			  G_CALLBACK (task_dialog_task_combo_select_child_cb),
			  combo);
}

static gboolean
task_cmd_edit_property_do (PlannerCmd *cmd_base)
{
	TaskCmdEditProperty *cmd;

	cmd = (TaskCmdEditProperty*) cmd_base;

	g_object_set_property (G_OBJECT (cmd->task),
			       cmd->property,
			       cmd->value);

	return TRUE;
}

static void
task_cmd_edit_property_undo (PlannerCmd *cmd_base)
{
	TaskCmdEditProperty *cmd;

	cmd = (TaskCmdEditProperty*) cmd_base;

	g_object_set_property (G_OBJECT (cmd->task),
			       cmd->property,
			       cmd->old_value);
}

static void
task_cmd_edit_property_free (PlannerCmd *cmd_base)
{
	TaskCmdEditProperty *cmd;

	cmd = (TaskCmdEditProperty*) cmd_base;

	g_value_unset (cmd->value);
	g_value_unset (cmd->old_value);

	g_free (cmd->property);
	g_object_unref (cmd->task);
}

static PlannerCmd *
task_cmd_edit_property_focus (PlannerWindow *main_window,
			      MrpTask       *task,
			      const gchar   *property,
			      const GValue  *focus_in_value)
{
	PlannerCmd          *cmd_base;
	TaskCmdEditProperty *cmd;

	cmd = g_new0 (TaskCmdEditProperty, 1);

	cmd_base = (PlannerCmd*) cmd;

	cmd_base->label = g_strdup (_("Edit task property from dialog"));
	cmd_base->do_func = task_cmd_edit_property_do;
	cmd_base->undo_func = task_cmd_edit_property_undo;
	cmd_base->free_func = task_cmd_edit_property_free;

	cmd->property = g_strdup (property);
	cmd->task = g_object_ref (task);

	cmd->value = g_new0 (GValue, 1);
	g_value_init (cmd->value, G_VALUE_TYPE (focus_in_value));
	g_object_get_property (G_OBJECT (cmd->task),
			       cmd->property,
			       cmd->value);

	cmd->old_value = g_new0 (GValue, 1);
	g_value_init (cmd->old_value, G_VALUE_TYPE (focus_in_value));
	g_value_copy (focus_in_value, cmd->old_value);

	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager (main_window),
					   cmd_base);

	return cmd_base;
}

static gboolean
task_cmd_type_do (PlannerCmd *cmd_base)
{
	TaskCmdEditType *cmd;

	cmd = (TaskCmdEditType* ) cmd_base;

	mrp_object_set (cmd->task, "type", cmd->type, NULL);

	return TRUE;
}

static void
task_cmd_type_undo (PlannerCmd *cmd_base)
{
	TaskCmdEditType *cmd;

	cmd = (TaskCmdEditType* ) cmd_base;

	mrp_object_set (cmd->task, "type", cmd->old_type, NULL);
}

static void
task_cmd_type_free (PlannerCmd *cmd_base)
{
	TaskCmdEditType *cmd;

	cmd = (TaskCmdEditType* ) cmd_base;

	g_object_unref (cmd->task);
}

static PlannerCmd *
task_cmd_edit_type (PlannerWindow *main_window,
		    MrpTask       *task,
		    MrpTaskType    type)
{
	PlannerCmd          *cmd_base;
	TaskCmdEditType     *cmd;
	MrpTaskType          old_type;

	mrp_object_get (task, "type", &old_type, NULL);

	if (old_type == type) {
		return NULL;
	}

	cmd = g_new0 (TaskCmdEditType, 1);

	cmd_base = (PlannerCmd*) cmd;

	cmd_base->label = g_strdup (_("Edit type task from dialog"));
	cmd_base->do_func = task_cmd_type_do;
	cmd_base->undo_func = task_cmd_type_undo;
	cmd_base->free_func = task_cmd_type_free;

	cmd->task = g_object_ref (task);

	cmd->old_type = old_type;
	cmd->type = type;

	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager (main_window),
					   cmd_base);

	return cmd_base;
}

static gboolean
task_cmd_sched_do (PlannerCmd *cmd_base)
{
	TaskCmdEditSchedule *cmd;

	cmd = (TaskCmdEditSchedule* ) cmd_base;

	mrp_object_set (cmd->task, "sched", cmd->sched, NULL);

	return TRUE;
}

static void
task_cmd_sched_undo (PlannerCmd *cmd_base)
{
	TaskCmdEditSchedule *cmd;

	cmd = (TaskCmdEditSchedule* ) cmd_base;

	mrp_object_set (cmd->task, "sched", cmd->old_sched, NULL);
}

static void
task_cmd_sched_free (PlannerCmd *cmd_base)
{
	TaskCmdEditSchedule *cmd;

	cmd = (TaskCmdEditSchedule* ) cmd_base;

	g_object_unref (cmd->task);
}

static PlannerCmd *
task_cmd_edit_sched (PlannerWindow *main_window,
		     MrpTask       *task,
		     MrpTaskSched   sched)
{
	PlannerCmd          *cmd_base;
	TaskCmdEditSchedule *cmd;
	MrpTaskSched         old_sched;

	mrp_object_get (task, "sched", &old_sched, NULL);

	if (old_sched == sched) {
		return NULL;
	}

	cmd = g_new0 (TaskCmdEditSchedule, 1);

	cmd_base = (PlannerCmd*) cmd;

	cmd_base->label = g_strdup (_("Edit task schedule from dialog"));
	cmd_base->do_func = task_cmd_sched_do;
	cmd_base->undo_func = task_cmd_sched_undo;
	cmd_base->free_func = task_cmd_sched_free;

	cmd->task = g_object_ref (task);

	cmd->old_sched = old_sched;
	cmd->sched = sched;

	planner_cmd_manager_insert_and_do (planner_window_get_cmd_manager (main_window),
					   cmd_base);

	return cmd_base;
}

static void
task_dialog_close_clicked_cb (GtkWidget *w, DialogData *data)
{
	gtk_widget_destroy (data->dialog);
}

static void
task_dialog_task_removed_cb (MrpObject *object, GtkWidget *dialog)
{
	gtk_widget_destroy (dialog);
}

static void
task_dialog_task_name_changed_cb (MrpTask *task, GParamSpec *pspec, GtkWidget *dialog)
{
	DialogData *data;
	gchar      *name;
		
	g_return_if_fail (MRP_IS_TASK (task));
	g_return_if_fail (GTK_IS_DIALOG (dialog));

	data = DIALOG_GET_DATA (dialog);
	
	g_object_get (task, "name", &name, NULL);

	g_signal_handlers_block_by_func (data->name_entry,
					 task_dialog_name_changed_cb,
					 dialog);
	
	gtk_entry_set_text (GTK_ENTRY (data->name_entry), name);

	task_dialog_update_title (data);

	g_signal_handlers_unblock_by_func (data->name_entry,
					   task_dialog_name_changed_cb,
					   dialog);
	
	g_free (name);
}

static void
task_dialog_name_changed_cb (GtkWidget *w, DialogData *data)
{
	const gchar *name;
	
	name = gtk_entry_get_text (GTK_ENTRY (w));

	g_signal_handlers_block_by_func (data->task, 
					 task_dialog_task_name_changed_cb,
					 data->dialog);
	
	g_object_set (data->task, "name", name, NULL);

	task_dialog_update_title (data);

	g_signal_handlers_unblock_by_func (data->task, 
					   task_dialog_task_name_changed_cb,
					   data->dialog);
}

static gboolean
task_dialog_name_focus_out_cb (GtkWidget     *w,
			       GdkEventFocus *event,
			       DialogData    *data)
{
	gchar        *focus_in_name;
	GValue        value = { 0 };
	PlannerCmd   *cmd;

	g_assert (MRP_IS_TASK (data->task));

	focus_in_name = g_object_get_data (G_OBJECT (data->task),"focus_in_name");
	
	g_value_init (&value, G_TYPE_STRING);
	g_value_set_string (&value, focus_in_name);

	cmd = task_cmd_edit_property_focus (data->main_window, 
					    data->task, "name", &value);

	g_free (focus_in_name);

	return FALSE;
}

static gboolean
task_dialog_name_focus_in_cb (GtkWidget     *w,
			      GdkEventFocus *event,
			      DialogData    *data)
{
	gchar  *name;
	   
	name = g_strdup (gtk_entry_get_text (GTK_ENTRY (w)));
	
	g_object_set_data (G_OBJECT (data->task), 
			   "focus_in_name", (gpointer) name);

	return FALSE;
}

static void
task_dialog_task_type_changed_cb (MrpTask *task, GParamSpec *pspec, GtkWidget *dialog)
{
	DialogData  *data;
	MrpTaskType  type;
		
	g_return_if_fail (MRP_IS_TASK (task));
	g_return_if_fail (GTK_IS_DIALOG (dialog));

	data = DIALOG_GET_DATA (dialog);

	g_object_get (task, "type", &type, NULL);

	g_signal_handlers_block_by_func (data->milestone_checkbutton,
					 task_dialog_type_toggled_cb,
					 dialog);
	
	if (type == MRP_TASK_TYPE_MILESTONE) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->milestone_checkbutton), TRUE); 
	} else {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->milestone_checkbutton), FALSE);
	}

	g_signal_handlers_unblock_by_func (data->milestone_checkbutton,
					   task_dialog_type_toggled_cb,
					   dialog);

	task_dialog_update_sensitivity (data);
}

static void
task_dialog_type_toggled_cb (GtkWidget *w, DialogData *data)
{
	MrpTaskType type;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->milestone_checkbutton))) {
		type = MRP_TASK_TYPE_MILESTONE;
	} else {
		type = MRP_TASK_TYPE_NORMAL;
	}

	g_signal_handlers_block_by_func (data->task, 
					 task_dialog_task_type_changed_cb,
					 data->dialog);
	
	/* g_object_set (data->task, "type", type, NULL); */
	task_cmd_edit_type (data->main_window, data->task, type);

	g_signal_handlers_unblock_by_func (data->task, 
					   task_dialog_task_type_changed_cb,
					   data->dialog);

	task_dialog_update_sensitivity (data);
}

static void
task_dialog_task_sched_changed_cb (MrpTask *task, GParamSpec *pspec, GtkWidget *dialog)
{
	DialogData   *data;
	MrpTaskSched  sched;
		
	g_return_if_fail (MRP_IS_TASK (task));
	g_return_if_fail (GTK_IS_DIALOG (dialog));

	data = DIALOG_GET_DATA (dialog);

	g_object_get (task, "sched", &sched, NULL);

	/* FIXME: this doesn't do anything right now. */
	
	g_signal_handlers_block_by_func (data->fixed_checkbutton,
					 task_dialog_fixed_toggled_cb,
					 dialog);

	if (sched == MRP_TASK_SCHED_FIXED_DURATION) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->fixed_checkbutton), TRUE);
	} else {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->fixed_checkbutton), FALSE);	
	}
	
	/* Set the toggle */

	g_signal_handlers_unblock_by_func (data->fixed_checkbutton,
					   task_dialog_fixed_toggled_cb,
					   dialog);

	task_dialog_update_sensitivity (data);
}

static void
task_dialog_fixed_toggled_cb (GtkWidget *w, DialogData *data)
{
	MrpTaskSched sched;

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (data->fixed_checkbutton))) {
		sched = MRP_TASK_SCHED_FIXED_DURATION;
	} else {
		sched = MRP_TASK_SCHED_FIXED_WORK;
	}

	g_signal_handlers_block_by_func (data->task, 
					 task_dialog_task_sched_changed_cb,
					 data->dialog);

	/* g_object_set (data->task, "sched", sched, NULL); */
	task_cmd_edit_sched (data->main_window, data->task, sched);

	g_signal_handlers_unblock_by_func (data->task, 
					   task_dialog_task_sched_changed_cb,
					   data->dialog);

	task_dialog_update_sensitivity (data);
}

static void
task_dialog_task_work_changed_cb (MrpTask    *task, 
				  GParamSpec *pspec,
				  GtkWidget  *dialog)
{
	DialogData  *data;
	MrpProject  *project;
	MrpCalendar *calendar;
	gint         work;
	gint         work_per_day;	

	data = DIALOG_GET_DATA (dialog);

	g_object_get (task, "project", &project, NULL);
	calendar = mrp_project_get_calendar (project);
	work_per_day = mrp_calendar_day_get_total_work (
		calendar, mrp_day_get_work ());

	/* Prevent div by zero. */
	if (work_per_day == 0) {
		work_per_day = 8;
	}
	
	g_object_get (task, "work", &work, NULL);

	g_signal_handlers_block_by_func (data->work_spinbutton,
					 task_dialog_work_changed_cb,
					 data);
	
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->work_spinbutton),
				   (double) work / (double) work_per_day);
	
	g_signal_handlers_unblock_by_func (data->work_spinbutton,
					   task_dialog_work_changed_cb,
					   data);
}

static void
task_dialog_work_changed_cb (GtkWidget  *w,
			     DialogData *data)
{
	MrpProject  *project;
	MrpCalendar *calendar;
	gint         work;
	gint         work_per_day;	

	g_object_get (data->task, "project", &project, NULL);
	calendar = mrp_project_get_calendar (project);
	work_per_day = mrp_calendar_day_get_total_work (calendar,
							mrp_day_get_work ());

	work = gtk_spin_button_get_value (GTK_SPIN_BUTTON (w)) * work_per_day;
	
	g_signal_handlers_block_by_func (data->task,
					 task_dialog_task_work_changed_cb,
					 data->dialog);
	
	g_object_set (data->task, "work", work, NULL);

	g_signal_handlers_unblock_by_func (data->task,
					   task_dialog_task_work_changed_cb, 
					   data->dialog);
}

static gboolean
task_dialog_work_focus_out_cb (GtkWidget     *w,
			       GdkEventFocus *event,
			       DialogData    *data)
{
	MrpProject   *project;
	MrpCalendar  *calendar;
	gint          work;
	gint          current_work;
	gint          work_per_day;
	gdouble      *focus_in_work;
	GValue        value = { 0 };
	PlannerCmd   *cmd;

	g_assert (MRP_IS_TASK (data->task));

	g_object_get (data->task, "project", &project, NULL);
	calendar = mrp_project_get_calendar (project);
	work_per_day = mrp_calendar_day_get_total_work (calendar,
							mrp_day_get_work ());

	focus_in_work = g_object_get_data (G_OBJECT (data->task), "focus_in_work");

	gtk_spin_button_update (GTK_SPIN_BUTTON (w));
	current_work = gtk_spin_button_get_value (GTK_SPIN_BUTTON (w)) * work_per_day;
	work = (*focus_in_work) * work_per_day;

	if (work == current_work) {
		g_free (focus_in_work);
		return FALSE;
	}

	g_object_set (data->task, "work", current_work, NULL);

	g_value_init (&value, G_TYPE_INT);
	g_value_set_int (&value, work);

	cmd = task_cmd_edit_property_focus (data->main_window, 
					    data->task, "work", &value);

	g_free (focus_in_work);

	return FALSE;
}

static gboolean
task_dialog_work_focus_in_cb (GtkWidget     *w,
			      GdkEventFocus *event,
			      DialogData    *data)
{
	gdouble *work;

	work = g_new0 (gdouble, 1);
	   
	*work = gtk_spin_button_get_value (GTK_SPIN_BUTTON (w));
	
	g_object_set_data (G_OBJECT (data->task), 
			   "focus_in_work", (gpointer) work);

	return FALSE;
}

static void
task_dialog_task_duration_changed_cb (MrpTask    *task, 
				      GParamSpec *pspec,
				      GtkWidget  *dialog)
{
	DialogData  *data;
	MrpProject  *project;
	MrpCalendar *calendar;
	gint         work;
	gint         work_per_day;	

	data = DIALOG_GET_DATA (dialog);

	g_object_get (task, "project", &project, NULL);
	calendar = mrp_project_get_calendar (project);
	work_per_day = mrp_calendar_day_get_total_work (calendar,
							mrp_day_get_work ());

	/* Prevent div by zero. */
	if (work_per_day == 0) {
		work_per_day = 8;
	}
	
	g_object_get (task, "duration", &work, NULL);

	g_signal_handlers_block_by_func (data->duration_spinbutton,
					 task_dialog_duration_changed_cb,
					 data);
	
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->duration_spinbutton),
				   (double) work / (double) work_per_day);
	
	g_signal_handlers_unblock_by_func (data->duration_spinbutton,
					   task_dialog_duration_changed_cb,
					   data);
}

static void
task_dialog_duration_changed_cb (GtkWidget  *w,
				 DialogData *data)
{
	MrpProject  *project;
	MrpCalendar *calendar;
	gint         duration;
	gint         work_per_day;	

	g_object_get (data->task, "project", &project, NULL);
	calendar = mrp_project_get_calendar (project);
	work_per_day = mrp_calendar_day_get_total_work (calendar,
							mrp_day_get_work ());

	duration = gtk_spin_button_get_value (GTK_SPIN_BUTTON (w)) * work_per_day;
	
	g_signal_handlers_block_by_func (data->task,
					 task_dialog_task_duration_changed_cb,
					 data->dialog);
	
	g_object_set (data->task, "duration", duration, NULL);

	g_signal_handlers_unblock_by_func (data->task,
					   task_dialog_task_duration_changed_cb, 
					   data->dialog);
}


/* FIXME: try to unify duration and work focus functions? */

static gboolean
task_dialog_duration_focus_out_cb (GtkWidget     *w,
				   GdkEventFocus *event,
				   DialogData    *data)
{
	MrpProject   *project;
	MrpCalendar  *calendar;
	gint          duration;
	gint          current_duration;
	gint          work_per_day;
	gdouble      *focus_in_duration;
	GValue        value = { 0 };
	PlannerCmd   *cmd;

	g_assert (MRP_IS_TASK (data->task));

	g_object_get (data->task, "project", &project, NULL);
	calendar = mrp_project_get_calendar (project);
	work_per_day = mrp_calendar_day_get_total_work (calendar,
							mrp_day_get_work ());

	focus_in_duration = g_object_get_data (G_OBJECT (data->task), "focus_in_duration");

	/* FIXME: Why spin button isn't updated when is modified with direct entry
	 from keyboard with correct values? */
	gtk_spin_button_update (GTK_SPIN_BUTTON (w));
	current_duration = gtk_spin_button_get_value (GTK_SPIN_BUTTON (w)) * work_per_day;
	duration = (*focus_in_duration) * work_per_day;

	if (duration == current_duration) {
		g_free (focus_in_duration);
		return FALSE;
	}

	g_object_set (data->task, "duration", current_duration, NULL);
	g_message ("Old duration %d, New duration %d", duration, current_duration);

	g_value_init (&value, G_TYPE_INT);
	g_value_set_int (&value, duration);

	cmd = task_cmd_edit_property_focus (data->main_window, 
					    data->task, "duration", &value);

	g_free (focus_in_duration);

	return FALSE;
}

static gboolean
task_dialog_duration_focus_in_cb (GtkWidget     *w,
				  GdkEventFocus *event,
				  DialogData    *data)
{
	gdouble *duration;

	duration = g_new0 (gdouble, 1);
	   
	*duration = gtk_spin_button_get_value (GTK_SPIN_BUTTON (w));
	
	g_object_set_data (G_OBJECT (data->task), 
			   "focus_in_duration", (gpointer) duration);

	return FALSE;
}


static void
task_dialog_task_complete_changed_cb (MrpTask    *task, 
				      GParamSpec *pspec,
				      GtkWidget  *dialog)
{
	DialogData *data;
	gint       complete;
	
	g_return_if_fail (MRP_IS_TASK (task));
	g_return_if_fail (GTK_IS_WIDGET (dialog));

	data = DIALOG_GET_DATA (dialog);
	
	g_object_get (task, "percent_complete", &complete, NULL);
	
	g_signal_handlers_block_by_func (data->complete_spinbutton,
					 task_dialog_complete_changed_cb,
					 data);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->complete_spinbutton),
				   complete);
	
	g_signal_handlers_unblock_by_func (data->complete_spinbutton,
					   task_dialog_complete_changed_cb,
					   data);
}

static void
task_dialog_complete_changed_cb (GtkWidget  *w,
				 DialogData *data)
{
	gint complete;

	complete = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (w));
	
	g_signal_handlers_block_by_func (data->task,
					 task_dialog_task_complete_changed_cb,
					 data->dialog);
	
	g_object_set (data->task, "percent_complete", complete, NULL);

	g_signal_handlers_unblock_by_func (data->task, 
					   task_dialog_task_complete_changed_cb,
					   data->dialog);
}

static gboolean
task_dialog_complete_focus_out_cb (GtkWidget     *w,
				   GdkEventFocus *event,
				   DialogData    *data)
{
	guint         current_complete;
	guint         focus_in_complete;
	GValue        value = { 0 };
	PlannerCmd   *cmd;

	g_assert (MRP_IS_TASK (data->task));

	focus_in_complete = GPOINTER_TO_INT (g_object_get_data 
					     (G_OBJECT (data->task), "focus_in_complete"));

	gtk_spin_button_update (GTK_SPIN_BUTTON (w));
	current_complete = gtk_spin_button_get_value (GTK_SPIN_BUTTON (w));

	if (focus_in_complete == current_complete) {
		return FALSE;
	}

	g_object_set (data->task, "percent_complete", current_complete, NULL);

	g_value_init (&value, G_TYPE_UINT);
	g_value_set_uint (&value, focus_in_complete);

	cmd = task_cmd_edit_property_focus (data->main_window, 
					    data->task, "percent_complete", &value);

	return FALSE;
}

static gboolean
task_dialog_complete_focus_in_cb (GtkWidget     *w,
				  GdkEventFocus *event,
				  DialogData    *data)
{
	guint complete;

	complete = gtk_spin_button_get_value (GTK_SPIN_BUTTON (w));
	
	g_object_set_data (G_OBJECT (data->task), 
			   "focus_in_complete", GINT_TO_POINTER (complete));

	return FALSE;
}

static void
task_dialog_task_priority_changed_cb (MrpTask    *task, 
				      GParamSpec *pspec,
				      GtkWidget  *dialog)
{
	DialogData *data;
	gint       priority;
	
	g_return_if_fail (MRP_IS_TASK (task));
	g_return_if_fail (GTK_IS_WIDGET (dialog));

	data = DIALOG_GET_DATA (dialog);
	
	g_object_get (task, "priority", &priority, NULL);
	
	g_signal_handlers_block_by_func (data->priority_spinbutton,
					 task_dialog_priority_changed_cb,
					 data);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->priority_spinbutton),
				   priority);
	
	g_signal_handlers_unblock_by_func (data->priority_spinbutton,
					   task_dialog_priority_changed_cb,
					   data);
}

static void
task_dialog_priority_changed_cb (GtkWidget  *w,
				 DialogData *data)
{
	gint priority;

	priority = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (w));
	
	g_signal_handlers_block_by_func (data->task,
					 task_dialog_task_priority_changed_cb,
					 data->dialog);
	
	g_object_set (data->task, "priority", priority, NULL);

	g_signal_handlers_unblock_by_func (data->task, 
					   task_dialog_task_priority_changed_cb,
					   data->dialog);
}

static gboolean
task_dialog_priority_focus_out_cb (GtkWidget     *w,
				   GdkEventFocus *event,
				   DialogData    *data)
{
	gint          current_priority;
	gint          focus_in_priority;
	GValue        value = { 0 };
	PlannerCmd   *cmd;

	g_assert (MRP_IS_TASK (data->task));

	focus_in_priority = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (data->task), 
								"focus_in_priority"));

	gtk_spin_button_update (GTK_SPIN_BUTTON (w));
	current_priority = gtk_spin_button_get_value (GTK_SPIN_BUTTON (w));

	if (focus_in_priority == current_priority) {
		return FALSE;
	}

	g_object_set (data->task, "priority", current_priority, NULL);

	g_value_init (&value, G_TYPE_INT);
	g_value_set_int (&value, focus_in_priority);

	cmd = task_cmd_edit_property_focus (data->main_window, 
					    data->task, "priority", &value);

	return FALSE;
}

static gboolean
task_dialog_priority_focus_in_cb (GtkWidget     *w,
				  GdkEventFocus *event,
				  DialogData    *data)
{
	gint priority;

	priority = gtk_spin_button_get_value (GTK_SPIN_BUTTON (w));
	
	g_object_set_data (G_OBJECT (data->task), 
			   "focus_in_priority", GINT_TO_POINTER (priority));

	return FALSE;
}

static void
task_dialog_task_note_changed_cb (MrpTask    *task,
				  GParamSpec *pspec,
				  GtkWidget  *dialog)
{
	DialogData *data;
	gchar      *note;
	
	g_return_if_fail (MRP_IS_TASK (task));
	g_return_if_fail (GTK_IS_DIALOG (dialog));

	data = DIALOG_GET_DATA (dialog);
	
	g_object_get (task, "note", &note, NULL);

	g_signal_handlers_block_by_func (data->note_buffer,
					 task_dialog_note_changed_cb,
					 dialog);
	
	gtk_text_buffer_set_text (data->note_buffer, note, -1);

	g_signal_handlers_unblock_by_func (data->note_buffer,
					   task_dialog_note_changed_cb,
					   dialog);
	
	g_free (note);
}

static void  
task_dialog_note_changed_cb (GtkWidget  *w,
			     DialogData *data)
{
	const gchar   *note;
	GtkTextIter    start, end;
	GtkTextBuffer *buffer;
	
	buffer = GTK_TEXT_BUFFER (w);
	
	gtk_text_buffer_get_bounds (buffer, &start, &end);
	
	note = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

	g_signal_handlers_block_by_func (data->task,
					 task_dialog_task_note_changed_cb,
					 data->dialog);

	g_object_set (data->task, "note", note, NULL);

	g_signal_handlers_unblock_by_func (data->task,
					   task_dialog_task_note_changed_cb,
					   data->dialog);
}

static void
task_dialog_note_stamp_cb (GtkWidget  *w,
			   DialogData *data)
{
	GtkTextIter  end;
	time_t       t;
	struct tm   *tm;
	gchar        stamp[128];
	gchar       *utf8;
	GtkTextMark *mark;
		
	t = time (NULL);
	tm = localtime (&t);

	/* i18n: time stamp format for notes in task dialog, see strftime(3) for
	 * a detailed description. */
	strftime (stamp, sizeof (stamp), _("%a %d %b %Y, %H:%M\n"), tm);

	utf8 = g_locale_to_utf8 (stamp, -1, NULL, NULL, NULL);
	
	gtk_text_buffer_get_end_iter (data->note_buffer, &end);

	if (!gtk_text_iter_starts_line (&end)) {
		gtk_text_buffer_insert (data->note_buffer, &end, "\n", 1);
		gtk_text_buffer_get_end_iter (data->note_buffer, &end);
	}

	gtk_text_buffer_insert (data->note_buffer, &end, utf8, -11);

	g_free (utf8);

	gtk_text_buffer_get_end_iter (data->note_buffer, &end);

	mark = gtk_text_buffer_create_mark (data->note_buffer,
					    NULL,
					    &end,
					    TRUE);

	gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (data->note_textview), mark);
}

static void
task_dialog_task_child_added_or_removed_cb (MrpTask   *task,
					    GtkWidget *dialog)
{
	DialogData *data;

	data = DIALOG_GET_DATA (dialog);
	
	task_dialog_update_sensitivity (data);
}

static GtkWidget *  
task_dialog_predecessor_dialog_new (MrpTask *task)
{
	MrpProject *project;
	GladeXML   *glade;
	GtkWidget  *dialog;
	GtkWidget  *w;
	GList      *tasks;
	
	mrp_object_get (task, "project", &project, NULL);
	
	glade = glade_xml_new (GLADEDIR "/add-predecessor.glade",
			       NULL,
			       NULL);

	dialog = glade_xml_get_widget (glade, "add_predecessor_dialog");
	g_object_set_data (G_OBJECT (dialog), "task_main", task);
	
	w = glade_xml_get_widget (glade, "predecessor_combo");
	g_object_set_data (G_OBJECT (dialog), "predecessor_combo", w);
	
	tasks = mrp_project_get_all_tasks (project);
	tasks = g_list_remove (tasks, task);
	task_dialog_setup_task_combo (GTK_COMBO (w), tasks);
	
	w = glade_xml_get_widget (glade, "type_optionmenu");
	g_object_set_data (G_OBJECT (dialog), "type_optionmenu", w);
	task_dialog_setup_option_menu (w,
				       NULL,
				       NULL,
				       _("Finish to start (FS)"), MRP_RELATION_FS,
				       _("Finish to finish (FF)"), MRP_RELATION_FF,
				       _("Start to start (SS)"), MRP_RELATION_SS,
				       _("Start to finish (SF)"), MRP_RELATION_SF,
				       NULL);

	w = glade_xml_get_widget (glade, "lag_spinbutton");
	g_object_set_data (G_OBJECT (dialog), "lag_spinbutton", w);

	w = glade_xml_get_widget (glade, "cancel_button");
	g_signal_connect (w,
			  "clicked",
			  G_CALLBACK (task_dialog_new_pred_cancel_clicked_cb),
			  dialog);

	w = glade_xml_get_widget (glade, "ok_button");
	g_signal_connect (w,
			  "clicked",
			  G_CALLBACK (task_dialog_new_pred_ok_clicked_cb),
			  dialog);

	return dialog;
}

static void  
task_dialog_new_pred_ok_clicked_cb (GtkWidget *button, 
				    GtkWidget *dialog)
{
	GtkWidget   *w;
	GError      *error = NULL;
	MrpTask     *task_main;
	MrpTask     *new_task_pred; 
	MrpProject  *project; 
	gint         lag;
	gint         pred_type; 
	gchar       *str;
	
	task_main = g_object_get_data (G_OBJECT (dialog), "task_main");
	mrp_object_get (task_main, "project", &project, NULL);

	/* Predecessor lag. */
	w = g_object_get_data (G_OBJECT (dialog), "lag_spinbutton");
	lag = gtk_spin_button_get_value (GTK_SPIN_BUTTON (w)) * (60*60);

	/* Predecessor type. */
	w = g_object_get_data (G_OBJECT(dialog), "type_optionmenu");
	pred_type = task_dialog_option_menu_get_selected (w);

	/* Predecessor task. */
	w = g_object_get_data (G_OBJECT (dialog), "predecessor_combo");

	new_task_pred = g_object_get_data (G_OBJECT (w), "selected_task");
	if (new_task_pred == NULL) {
		g_warning (_("Can't add new predecessor. No task selected!"));
                return;
        }
	
	mrp_object_get (MRP_OBJECT (new_task_pred), "name", &str, NULL);

	if (!mrp_task_add_predecessor (task_main,
				       new_task_pred,
				       pred_type,
				       lag,
				       &error)) {
		GtkWidget *err_dialog;
		
		err_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
						     GTK_DIALOG_DESTROY_WITH_PARENT,
						     GTK_MESSAGE_ERROR,
						     GTK_BUTTONS_OK,
						     "%s", error->message);
		gtk_dialog_run (GTK_DIALOG (err_dialog));
		gtk_widget_destroy (err_dialog);
		
		g_error_free (error);
	} else {
		gtk_widget_destroy (dialog);	
	}

	g_free (str);
}

static void  
task_dialog_new_pred_cancel_clicked_cb (GtkWidget *w, 
					GtkWidget *dialog)
{
	gtk_widget_destroy (dialog);
} 

static void  
task_dialog_add_predecessor_cb (GtkWidget  *widget,
				DialogData *data)
{
	GtkWidget *dialog;
	
	dialog = task_dialog_predecessor_dialog_new (data->task);
	gtk_widget_show (dialog);
}

static void  
task_dialog_remove_predecessor_cb (GtkWidget  *widget,
				   DialogData *data)
{
	GtkTreeView        *tree;
	MrpTask            *predecessor;
	PlannerPredecessorModel *model;
	GtkTreeSelection   *selection;
	GtkTreeIter         iter;

	tree = GTK_TREE_VIEW (data->predecessor_list);
	model = PLANNER_PREDECESSOR_MODEL (gtk_tree_view_get_model (tree));
	
	selection = gtk_tree_view_get_selection (tree);
	if (!gtk_tree_selection_get_selected (selection, NULL, &iter)) {
                return;
        }
	
	predecessor = MRP_TASK (planner_list_model_get_object (PLANNER_LIST_MODEL (model), &iter));
	mrp_task_remove_predecessor (data->task, predecessor);
}

static void  
task_dialog_resource_units_cell_edited (GtkCellRendererText *cell, 
					gchar               *path_str,
					gchar               *new_text, 
					DialogData          *data)
{
	GtkTreeView   *tree;
	GtkTreeModel  *model;
	GtkTreePath   *path;
	GtkTreeIter    iter;
	MrpResource   *resource;
	MrpAssignment *assignment;

	tree = GTK_TREE_VIEW (data->resource_list);
	
	model = gtk_tree_view_get_model (tree);

	path = gtk_tree_path_new_from_string (path_str);
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_path_free (path);

	resource = ((GList *)iter.user_data)->data;
		
	assignment = mrp_task_get_assignment (data->task, resource);
	if (assignment) {
		g_object_set (assignment,
			      "units", atoi (new_text),
			      NULL);
	}
}

static void  
task_dialog_pred_cell_edited (GtkCellRendererText *cell, 
			      gchar               *path_str,
			      gchar               *new_text, 
			      DialogData          *data)
{
	GtkTreeView        *tree;
	GtkTreePath        *path;
	GtkTreeIter         iter;
	GtkTreeModel       *model;
	MrpProject         *project;
	MrpRelation        *relation;
	MrpTask            *task_main;
	MrpTask            *task_pred;
	MrpTask            *new_task_pred;
	PlannerCellRendererList *planner_cell;
	gint                column;
	GList              *tasks;
	gint                lag;
	MrpRelationType     type;

	tree = GTK_TREE_VIEW (data->predecessor_list);
	
	model = gtk_tree_view_get_model (tree);
	path = gtk_tree_path_new_from_string (path_str);
	column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));

	gtk_tree_model_get_iter (model, &iter, path);

	task_pred = MRP_TASK (planner_list_model_get_object (PLANNER_LIST_MODEL (model),
							&iter)); 
	task_main = data->task;
	
	mrp_object_get (task_main, "project", &project, NULL);

	relation = mrp_task_get_relation (task_main, task_pred);
	lag = mrp_relation_get_lag (relation) / (60*60);
	type = mrp_relation_get_relation_type (relation);

	switch (column) {
	case PREDECESSOR_COL_NAME:
		planner_cell = PLANNER_CELL_RENDERER_LIST (cell);

		tasks = mrp_project_get_all_tasks (project);
		tasks = g_list_remove (tasks, task_main);

		new_task_pred = g_list_nth_data (tasks, planner_cell->selected_index);

		if (new_task_pred != task_pred) {
			GError *error = NULL;

			mrp_task_remove_predecessor (task_main, task_pred);

			if (!mrp_task_add_predecessor (task_main,
						       new_task_pred,
						       type,
						       lag,
						       &error)) {
				GtkWidget *dialog;
				
				dialog = gtk_message_dialog_new (
					NULL,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK,
					"%s", error->message);

				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
				
				g_error_free (error);

				/* Restore the previous state. */
				mrp_task_add_predecessor (task_main,
							  task_pred, 
							  type,
							  lag,
							  NULL);
			}
		}
		break;

	case PREDECESSOR_COL_TYPE:
		planner_cell = PLANNER_CELL_RENDERER_LIST (cell);

		{	
			GError *error = NULL;
			mrp_task_remove_predecessor (task_main, task_pred);

			if (!mrp_task_add_predecessor (task_main,
						       task_pred,
						       planner_cell->selected_index + 1,
						       lag,
						       &error)) {
				GtkWidget *dialog;
				
				dialog = gtk_message_dialog_new (
					NULL,
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK,
					"%s", error->message);

				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
				
				g_error_free (error);

				/* Restore the previous state. */
				mrp_task_add_predecessor (task_main,
							  task_pred, 
							  type,
							  lag,
							  NULL);
			}
		}
		/* The index + 1 happens to be the same as the enum,
		 * we should probably do this some other way.
		 */
		
		relation = mrp_task_get_relation (task_main, task_pred);
		mrp_object_set (relation,
				"type",
				planner_cell->selected_index + 1,
				NULL);
				

		break;

	case PREDECESSOR_COL_LAG:
		mrp_object_set (relation,
				"lag",
				60*60 * atoi (new_text),
				NULL);
		break;

	default:
		g_assert_not_reached ();
		break;
	}

	gtk_tree_path_free (path);
}

static void  
task_dialog_cell_type_show_popup (PlannerCellRendererList *cell,
				  const gchar        *path_string,
				  gint                x1,
				  gint                y1,
				  gint                x2,
				  gint                y2,
				  DialogData         *data) 
{
	GtkTreeView       *tree;
	GtkTreeModel      *model;
	PlannerListModel  *list_model;
	GtkTreeIter        iter;
	GtkTreePath       *path;
	MrpTask           *predecessor;
	MrpRelation       *relation;
	GList             *list;

	g_return_if_fail (PLANNER_IS_CELL_RENDERER_LIST (cell));

	tree = GTK_TREE_VIEW (data->predecessor_list);
	model = gtk_tree_view_get_model (tree);
	list_model = PLANNER_LIST_MODEL (model);
	
	path = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (model, &iter, path);
	
	predecessor = MRP_TASK (planner_list_model_get_object (list_model, &iter));

	relation = mrp_task_get_relation (data->task, predecessor);

	list = NULL;
	list = g_list_append (list, g_strdup (_("FS")));
	list = g_list_append (list, g_strdup (_("FF")));
	list = g_list_append (list, g_strdup (_("SS")));
	list = g_list_append (list, g_strdup (_("SF")));
	
	cell->list = list;

	switch (mrp_relation_get_relation_type (relation)) {
	case MRP_RELATION_FS:
		cell->selected_index = 0;
		break;
	case MRP_RELATION_FF:
		cell->selected_index = 1;
		break;
	case MRP_RELATION_SS:
		cell->selected_index = 2;
		break;
	case MRP_RELATION_SF:
		cell->selected_index = 3;
		break;
	default:
		g_warning ("Unknown relation type %d", 
			   mrp_relation_get_relation_type (relation));
		break;
	}	
}

static void  
task_dialog_cell_name_show_popup (PlannerCellRendererList *cell,
				  const gchar        *path_string,
				  gint                x1,
				  gint                y1,
				  gint                x2,
				  gint                y2,
				  DialogData         *data)
{
	GtkTreeView  *tree;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	GtkTreePath  *path;
	MrpTask      *task_main;
	MrpTask      *task_pred;
	MrpProject   *project;
	GList        *list, *tasks, *l;

	g_return_if_fail (PLANNER_IS_CELL_RENDERER_LIST (cell));

	tree = GTK_TREE_VIEW (data->predecessor_list);
	model = gtk_tree_view_get_model (tree);	
	path  = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (model, &iter, path);
	
	task_main = data->task;
	
	task_pred = MRP_TASK (planner_list_model_get_object (PLANNER_LIST_MODEL (model),
							&iter));

	g_object_get (task_main, "project", &project, NULL);
	tasks = mrp_project_get_all_tasks (project);
	tasks = g_list_remove (tasks, task_main);

	list = NULL;

	for (l = tasks; l; l = l->next) {
		gchar *name;

		g_object_get (l->data, "name", &name, NULL);

		list = g_list_append (list, name);
	}
	
	cell->list = list;
	/* FIXME: Select the actual task being edited */
	cell->selected_index = 1;
}

static void  
task_dialog_cell_hide_popup (PlannerCellRendererList *cell,
			     GtkWidget          *view) 
{
	GList *l;

	for (l = cell->user_data; l; l = l->next) {
		if (l->data) {
			g_object_unref (l->data);
		}
	}
	
	g_list_free (cell->user_data);
	/* cell->user_data = NULL; */
}


static void
task_dialog_setup_widgets (DialogData *data,
			   GladeXML   *glade)
{
	GtkWidget    *w;
	gchar        *name;
	MrpTaskType   type;
	MrpTaskSched  sched;
	gchar        *note;      
	gint          int_value;

	w = glade_xml_get_widget (glade, "close_button");
	g_signal_connect (w,
			  "clicked",
			  G_CALLBACK (task_dialog_close_clicked_cb),
			  data);

	data->name_entry = glade_xml_get_widget (glade, "name_entry");

	g_object_get (data->task, "name", &name, NULL);
	if (name) {
		gtk_entry_set_text (GTK_ENTRY (data->name_entry), name);
		g_free (name);
	}

	g_signal_connect (data->name_entry,
			  "changed",
			  G_CALLBACK (task_dialog_name_changed_cb),
			  data);

	g_signal_connect (data->name_entry,
			  "focus_in_event",
			  G_CALLBACK (task_dialog_name_focus_in_cb),
			  data);

	g_signal_connect (data->name_entry,
			  "focus_out_event",
			  G_CALLBACK (task_dialog_name_focus_out_cb),
			  data);

	data->milestone_checkbutton = glade_xml_get_widget (glade, "milestone_checkbutton");
	g_object_get (data->task, "type", &type, NULL);
	if (type == MRP_TASK_TYPE_MILESTONE) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->milestone_checkbutton), TRUE);
	}
	g_signal_connect (data->milestone_checkbutton,
			  "toggled",
			  G_CALLBACK (task_dialog_type_toggled_cb),
			  data);

	data->fixed_checkbutton = glade_xml_get_widget (glade, "fixed_checkbutton");
	g_object_get (data->task, "sched", &sched, NULL);
	if (sched == MRP_TASK_SCHED_FIXED_DURATION) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (data->fixed_checkbutton), TRUE);
	}
	g_signal_connect (data->fixed_checkbutton,
			  "toggled",
			  G_CALLBACK (task_dialog_fixed_toggled_cb),
			  data);

	data->work_spinbutton = glade_xml_get_widget (glade, "work_spinbutton");
	g_object_get (data->task, "work", &int_value, NULL);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->work_spinbutton),
				   int_value / WORK_MULTIPLIER);
	g_signal_connect (data->work_spinbutton,
			  "value_changed",
			  G_CALLBACK (task_dialog_work_changed_cb),
			  data);
	g_signal_connect (data->work_spinbutton,
			  "focus_in_event",
			  G_CALLBACK (task_dialog_work_focus_in_cb),
			  data);
	g_signal_connect (data->work_spinbutton,
			  "focus_out_event",
			  G_CALLBACK (task_dialog_work_focus_out_cb),
			  data);
	

	data->duration_spinbutton = glade_xml_get_widget (glade, "duration_spinbutton");
	g_object_get (data->task, "duration", &int_value, NULL);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->duration_spinbutton),
				   int_value / WORK_MULTIPLIER);
	g_signal_connect (data->duration_spinbutton,
			  "value_changed",
			  G_CALLBACK (task_dialog_duration_changed_cb),
			  data);
	g_signal_connect (data->duration_spinbutton,
			  "focus_in_event",
			  G_CALLBACK (task_dialog_duration_focus_in_cb),
			  data);
	g_signal_connect (data->duration_spinbutton,
			  "focus_out_event",
			  G_CALLBACK (task_dialog_duration_focus_out_cb),
			  data);

	data->complete_spinbutton = glade_xml_get_widget (glade, "complete_spinbutton");

	g_object_get (data->task, "percent_complete", &int_value, NULL);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->complete_spinbutton), int_value);

	g_signal_connect (data->complete_spinbutton,
			  "value_changed",
			  G_CALLBACK (task_dialog_complete_changed_cb),
			  data);
	
	g_signal_connect (data->complete_spinbutton,
			  "focus_in_event",
			  G_CALLBACK (task_dialog_complete_focus_in_cb),
			  data);
	g_signal_connect (data->complete_spinbutton,
			  "focus_out_event",
			  G_CALLBACK (task_dialog_complete_focus_out_cb),
			  data);
	
	data->priority_spinbutton = glade_xml_get_widget (glade, "priority_spinbutton");
	g_object_get (data->task, "priority", &int_value, NULL);	
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (data->priority_spinbutton), int_value);
	g_signal_connect (data->priority_spinbutton,
			  "value_changed",
			  G_CALLBACK (task_dialog_priority_changed_cb),
			  data);	

	g_signal_connect (data->priority_spinbutton,
			  "focus_in_event",
			  G_CALLBACK (task_dialog_priority_focus_in_cb),
			  data);

	g_signal_connect (data->priority_spinbutton,
			  "focus_out_event",
			  G_CALLBACK (task_dialog_priority_focus_out_cb),
			  data);

	data->note_textview = glade_xml_get_widget (glade, "note_textview");

	data->note_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (data->note_textview));	

	g_object_get (data->task, "note", &note, NULL);
	if (note) {
		gtk_text_buffer_set_text (data->note_buffer, note, -1);
		g_free (note);
	}

	g_signal_connect (data->note_buffer,
			  "changed",
			  G_CALLBACK (task_dialog_note_changed_cb),
			  data);

	w = glade_xml_get_widget (glade, "stamp_button");
	g_signal_connect (w,
			  "clicked",
			  G_CALLBACK (task_dialog_note_stamp_cb),
			  data);
}

static void
task_dialog_assignment_toggled_cb (GtkCellRendererText *cell,
				   gchar               *path_str,
				   DialogData          *data)
{
	GtkTreeView  *tree;
	GtkTreeModel *model;
	GtkTreePath  *path;
	GtkTreeIter   iter;
	MrpResource  *resource;
	gboolean      active;

	tree = GTK_TREE_VIEW (data->resource_list);
	
	g_object_get (cell, "active", &active, NULL);

	model = gtk_tree_view_get_model (tree);

	path = gtk_tree_path_new_from_string (path_str);
	
	gtk_tree_model_get_iter (model, &iter, path);

	gtk_tree_path_free (path);

	resource = ((GList *)iter.user_data)->data;

 	if (!active) {
		mrp_resource_assign (resource, data->task, 100);
	} else {
		MrpAssignment *assignment;
		
		assignment = mrp_task_get_assignment (data->task, resource);

		if (assignment) {
			mrp_object_removed (MRP_OBJECT (assignment));
		} 
	}
}

static void
task_dialog_lag_data_func (GtkTreeViewColumn *tree_column,
			   GtkCellRenderer   *cell,
			   GtkTreeModel      *tree_model,
			   GtkTreeIter       *iter,
			   DialogData        *data)
{
	GValue  value = { 0 };
	gchar  *ret;

	gtk_tree_model_get_value (tree_model,
				  iter,
				  PREDECESSOR_COL_LAG,
				  &value);
	
	ret = g_strdup_printf ("%d", g_value_get_int (&value) / (60*60));
	
	g_object_set (cell, "text", ret, NULL);
	g_value_unset (&value);
	g_free (ret);
}

static void
task_dialog_setup_resource_list (DialogData *data)
{
	GtkTreeView       *tree = GTK_TREE_VIEW (data->resource_list);
	GtkTreeViewColumn *col;
	GtkCellRenderer   *cell;
	GtkTreeModel      *model;

	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (tree),
				     GTK_SELECTION_SINGLE);
	
	/* Name */
	cell = gtk_cell_renderer_text_new ();
	g_object_set (cell, "editable", FALSE, NULL);
	
	col = gtk_tree_view_column_new_with_attributes (
		_("Name"),
		cell,
		"text", RESOURCE_ASSIGNMENT_COL_NAME,
		NULL);

	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_column_set_min_width (col, 200);
	gtk_tree_view_append_column (tree, col);
	
	/* Assigned. */
	cell = gtk_cell_renderer_toggle_new ();
	g_object_set (cell,
		      "activatable", TRUE,
		      NULL);

	g_signal_connect (cell,
			  "toggled",
			  G_CALLBACK (task_dialog_assignment_toggled_cb),
			  data);
	
	col = gtk_tree_view_column_new_with_attributes (
		_("Assigned"),
		cell,
		"active", RESOURCE_ASSIGNMENT_COL_ASSIGNED,
		NULL);
	
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_append_column (tree, col);

	/* Assigned units */
	cell = gtk_cell_renderer_text_new ();
	
	g_signal_connect (cell,
			  "edited",
			  G_CALLBACK (task_dialog_resource_units_cell_edited),
			  data);
	
	col = gtk_tree_view_column_new_with_attributes (
		_("Units"),
		cell,
		"text", RESOURCE_ASSIGNMENT_COL_ASSIGNED_UNITS,
		"editable", RESOURCE_ASSIGNMENT_COL_ASSIGNED_UNITS_EDITABLE,
		NULL);

	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_column_set_min_width (col, 50);
	gtk_tree_view_append_column (tree, col);

	model = GTK_TREE_MODEL (planner_assignment_model_new (data->task));
	
	gtk_tree_view_set_model (tree, model);
}

static void
task_dialog_setup_predecessor_list (DialogData *data)
{
	GtkTreeView       *tree;
	GtkTreeViewColumn *col;
	GtkCellRenderer   *cell;
	GtkTreeModel      *model;

	tree = GTK_TREE_VIEW (data->predecessor_list);
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (tree),
				     GTK_SELECTION_SINGLE);
	
	/* Name */
	cell = planner_cell_renderer_list_new ();

	g_object_set (cell, "editable", TRUE, NULL);

	g_object_set_data (G_OBJECT (cell),
			   "column",
			   GINT_TO_POINTER (PREDECESSOR_COL_NAME));

	g_signal_connect (cell,
			  "edited",
			  G_CALLBACK (task_dialog_pred_cell_edited),
			  data);
	
	g_signal_connect (cell,
			  "show_popup",
			  G_CALLBACK (task_dialog_cell_name_show_popup),
			  data);
	
	g_signal_connect_after (cell,
				"hide_popup",
				G_CALLBACK (task_dialog_cell_hide_popup),
				data);

	col = gtk_tree_view_column_new_with_attributes (_("Name"),
							cell,
							"text", PREDECESSOR_COL_NAME,
							NULL);
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_column_set_min_width (col, 200);
	gtk_tree_view_append_column (tree, col);
	
	/* Type */
	cell = planner_cell_renderer_list_new ();

	g_object_set (cell, "editable", TRUE, NULL);

	g_object_set_data (G_OBJECT (cell),
			   "column",
			   GINT_TO_POINTER (PREDECESSOR_COL_TYPE));
	
	g_signal_connect (cell,
			  "edited",
			  G_CALLBACK (task_dialog_pred_cell_edited),
			  data);

	g_signal_connect (cell,
			  "show_popup",
			  G_CALLBACK (task_dialog_cell_type_show_popup),
			  data); 

	g_signal_connect_after (cell,
				"hide_popup",
				G_CALLBACK (task_dialog_cell_hide_popup),
				data);
	
	col = gtk_tree_view_column_new_with_attributes (_("Type"),
							cell,
							"text", PREDECESSOR_COL_TYPE,
							NULL);
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_append_column (tree, col);
	
	/* Lag */
	cell = gtk_cell_renderer_text_new ();

	g_object_set (cell, "editable", TRUE, NULL);

	g_object_set_data (G_OBJECT (cell),
			   "column",
			   GINT_TO_POINTER (PREDECESSOR_COL_LAG));

	g_signal_connect (cell,
			  "edited",
			  G_CALLBACK (task_dialog_pred_cell_edited),
			  data);
	
	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, _("Lag"));

	gtk_tree_view_column_pack_start (col,
					 cell,
					 TRUE);
	
	gtk_tree_view_column_set_cell_data_func (col,
						 cell,
						 (GtkTreeCellDataFunc) task_dialog_lag_data_func,
						 data,
						 NULL);
	
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_append_column (tree, col);
	
	model = planner_predecessor_model_new (data->task);
	
	gtk_tree_view_set_model (tree, model);
}

static void
task_dialog_connect_to_task (DialogData *data)
{
	g_signal_connect_object (data->task,
				 "notify::name",
				 G_CALLBACK (task_dialog_task_name_changed_cb),
				 data->dialog,
				 0);

	g_signal_connect_object (data->task,
				 "notify::type",
				 G_CALLBACK (task_dialog_task_type_changed_cb),
				 data->dialog,
				 0);

	g_signal_connect_object (data->task,
				 "notify::sched",
				 G_CALLBACK (task_dialog_task_sched_changed_cb),
				 data->dialog,
				 0);
	
	g_signal_connect_object (data->task,
				 "notify::note",
				 G_CALLBACK (task_dialog_task_note_changed_cb),
				 data->dialog,
				 0);

	g_signal_connect_object (data->task,
				 "notify::work",
				 G_CALLBACK (task_dialog_task_work_changed_cb),
				 data->dialog,
				 0);

	g_signal_connect_object (data->task,
				 "notify::duration",
				 G_CALLBACK (task_dialog_task_duration_changed_cb),
				 data->dialog,
				 0);

	g_signal_connect_object (data->task,
				 "notify::percent_complete",
				 G_CALLBACK (task_dialog_task_complete_changed_cb),
				 data->dialog,
				 0);

	g_signal_connect_object (data->task,
				 "notify::priority",
				 G_CALLBACK (task_dialog_task_priority_changed_cb),
				 data->dialog,
				 0);
				 
	g_signal_connect_object (data->task,
				 "child_added",
				 G_CALLBACK (task_dialog_task_child_added_or_removed_cb),
				 data->dialog,
				 0);

	g_signal_connect_object (data->task,
				 "child_removed",
				 G_CALLBACK (task_dialog_task_child_added_or_removed_cb),
				 data->dialog,
				 0);
}

static void
task_dialog_calendar_changed_cb (MrpCalendar *calendar,
				 GtkWidget   *dialog)
{
	DialogData *data;

	data = DIALOG_GET_DATA (dialog);

	/* We want the work field to update according to the potentially new day
	 * length.
	 */
	task_dialog_task_work_changed_cb (data->task,
					  NULL,
					  dialog);

	task_dialog_task_duration_changed_cb (data->task,
					      NULL,
					      dialog);
}

static void
task_dialog_update_sensitivity (DialogData *data)
{
	MrpTaskType  type;
	MrpTaskSched sched;
	gboolean     leaf, milestone, fixed, sensitive;
	
	leaf = (mrp_task_get_n_children (data->task) == 0);

	g_object_get (data->task, "type", &type, NULL);
	milestone = (type == MRP_TASK_TYPE_MILESTONE);

	g_object_get (data->task, "sched", &sched, NULL);
	fixed = (sched == MRP_TASK_SCHED_FIXED_DURATION);

	sensitive = leaf && (type != MRP_TASK_TYPE_MILESTONE);

	gtk_widget_set_sensitive (data->milestone_checkbutton, leaf);
	gtk_widget_set_sensitive (data->fixed_checkbutton, leaf);
	
	gtk_widget_set_sensitive (data->work_spinbutton, sensitive);
	gtk_widget_set_sensitive (data->duration_spinbutton, sensitive && fixed);
	gtk_widget_set_sensitive (data->complete_spinbutton, sensitive);
	gtk_widget_set_sensitive (data->priority_spinbutton, sensitive);
}

static void
task_dialog_update_title (DialogData *data)
{
	gchar *title;
	gchar *name;

	g_object_get (data->task, "name", &name, NULL);

	if (!name || strlen (name) == 0) {
		title = g_strdup (_("Edit task properties"));
	} else {
		title = g_strconcat (name, " - ", _("Edit task properties"), NULL);
	}

	gtk_window_set_title (GTK_WINDOW (data->dialog), title);

	g_free (name);
	g_free (title);
}	

static void
task_dialog_parent_destroy_cb (GtkWidget *parent,
			       GtkWidget *dialog)
{
	gtk_widget_destroy (dialog);
}

static void
task_dialog_destroy_cb (GtkWidget  *parent,
			DialogData *data)
{
	g_hash_table_remove (dialogs, data->task);
}

GtkWidget *
planner_task_dialog_new (PlannerWindow *window,
			 MrpTask       *task,
			 PlannerTaskDialogPage  page)
{
	DialogData   *data;
	GladeXML     *glade;
	GtkWidget    *dialog;
	GtkWidget    *w;
	GtkSizeGroup *size_group;
	MrpProject   *project;
	MrpCalendar  *calendar;
	
	g_return_val_if_fail (MRP_IS_TASK (task), NULL);

	if (!dialogs) {
		dialogs = g_hash_table_new (NULL, NULL);
	}

	dialog = g_hash_table_lookup (dialogs, task);
	if (dialog) {
		gtk_window_present (GTK_WINDOW (dialog));
		return dialog;
	}

	glade = glade_xml_new (GLADEDIR "/task-dialog.glade",
			       NULL,
			       GETTEXT_PACKAGE);
	if (!glade) {
		g_warning ("Could not create task dialog.");
		return NULL;
	}

	dialog = glade_xml_get_widget (glade, "task_dialog");
	
	data = g_new0 (DialogData, 1);

	data->main_window = window;
	data->dialog = dialog;
	data->task = task;

	g_hash_table_insert (dialogs, task, dialog);

	g_signal_connect (dialog,
			  "destroy",
			  G_CALLBACK (task_dialog_destroy_cb),
			  data);
	
	g_signal_connect_object (window,
				 "destroy",
				 G_CALLBACK (task_dialog_parent_destroy_cb),
				 dialog,
				 0);

	g_signal_connect_object (task,
				 "removed",
				 G_CALLBACK (task_dialog_task_removed_cb),
				 dialog,
				 0);

	w = glade_xml_get_widget (glade, "task_notebook");

	gtk_notebook_set_current_page (GTK_NOTEBOOK (w), page);
		
	data->resource_list = glade_xml_get_widget (glade, "resource_list");
	task_dialog_setup_resource_list (data);

	data->predecessor_list = glade_xml_get_widget (glade, "predecessor_list");
	task_dialog_setup_predecessor_list (data);

	w = glade_xml_get_widget (glade, "add_predecessor_button");
	g_signal_connect (w,
			  "clicked",
			  G_CALLBACK (task_dialog_add_predecessor_cb),
			  data);
	
	w = glade_xml_get_widget (glade, "remove_predecessor_button");
	g_signal_connect (w,
			  "clicked",
			  G_CALLBACK (task_dialog_remove_predecessor_cb),
			  data);

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
	
	w = glade_xml_get_widget (glade, "name_pad");
	gtk_size_group_add_widget (size_group, w);

	w = glade_xml_get_widget (glade, "milestone_pad");
	gtk_size_group_add_widget (size_group, w);
	
	w = glade_xml_get_widget (glade, "work_optionmenu");
	gtk_size_group_add_widget (size_group, w);

	w = glade_xml_get_widget (glade, "duration_optionmenu");
	gtk_size_group_add_widget (size_group, w);

	w = glade_xml_get_widget (glade, "complete_pad");
	gtk_size_group_add_widget (size_group, w);

	w = glade_xml_get_widget (glade, "priority_pad");
	gtk_size_group_add_widget (size_group, w);

	g_object_unref (size_group);

	g_object_set_data_full (G_OBJECT (dialog),
				"data", data,
				g_free);
	
	task_dialog_setup_widgets (data, glade);

	task_dialog_update_sensitivity (data);

	task_dialog_update_title (data);
		
	task_dialog_connect_to_task (data);
	
	g_object_get (task, "project", &project, NULL);
	calendar = mrp_project_get_calendar (project);

	g_signal_connect_object (calendar,
				 "calendar-changed",
				 G_CALLBACK (task_dialog_calendar_changed_cb),
				 dialog,
				 0);
	
	return dialog;
}
