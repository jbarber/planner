/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
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
#include <glade/glade.h>
#include <gtk/gtk.h>
#include "planner-marshal.h"
#include "planner-task-input-dialog.h"
#include "planner-task-cmd.h"

typedef struct {
	MrpProject    *project;
	PlannerWindow *main_window;

	GtkWidget     *name_entry;
	GtkWidget     *work_entry;
} DialogData;

static void
task_input_dialog_free (gpointer user_data)
{
	DialogData *data = user_data;

	g_object_unref (data->project);
	g_object_unref (data->main_window);

	g_free (data);
}

static void
task_input_dialog_response_cb (GtkWidget *button,
			       gint       response,
			       GtkWidget *dialog)
{
	DialogData  *data;
	MrpTask     *task;
	const gchar *name;
	const gchar *workstr;
	gint         work;
	
	switch (response) {
	case GTK_RESPONSE_OK:
		data = g_object_get_data (G_OBJECT (dialog), "data");
		
		name = gtk_entry_get_text (GTK_ENTRY (data->name_entry));
		workstr = gtk_entry_get_text (GTK_ENTRY (data->work_entry));
		
		work = 60*60*8 * g_strtod (workstr, NULL);
		
		task = g_object_new (MRP_TYPE_TASK,
				     "work", work,
				     "name", name,
				     NULL);
		
		/*mrp_project_insert_task (data->project, NULL, -1, task);*/
		planner_task_cmd_insert (data->main_window, 
					 NULL, -1, 0, 0, task);
		
		gtk_entry_set_text (GTK_ENTRY (data->name_entry), "");
		gtk_entry_set_text (GTK_ENTRY (data->work_entry), "");
		
		gtk_widget_grab_focus (data->name_entry);
		break;

	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		gtk_widget_destroy (dialog);
		break;
		
	default:
		g_assert_not_reached ();
		break;
	}
}

static void
task_input_dialog_activate_cb (GtkWidget *widget, GtkDialog *dialog)
{
	gtk_dialog_response (dialog, GTK_RESPONSE_OK);
}

GtkWidget *
planner_task_input_dialog_new (PlannerWindow *main_window)
{
	GtkWidget  *dialog;
	DialogData *data;
	GladeXML   *gui;
	MrpProject *project;

	data = g_new0 (DialogData, 1);

	project = planner_window_get_project (main_window);

	data->project = g_object_ref (project);
	data->main_window = g_object_ref (main_window);
	
	gui = glade_xml_new (GLADEDIR "/task-input-dialog.glade",
			     NULL , NULL);
	
	dialog = glade_xml_get_widget (gui, "task_input_dialog"); 
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (task_input_dialog_response_cb),
			  dialog);
	
	data->name_entry = glade_xml_get_widget (gui, "name_entry");
	g_signal_connect (data->name_entry,
			  "activate",
			  G_CALLBACK (task_input_dialog_activate_cb),
			  dialog);
	
	data->work_entry = glade_xml_get_widget (gui, "work_entry");
	g_signal_connect (data->work_entry,
			  "activate",
			  G_CALLBACK (task_input_dialog_activate_cb),
			  dialog);
	
	g_object_set_data_full (G_OBJECT (dialog),
				"data",
				data,
				task_input_dialog_free);
	
        return dialog;
}
