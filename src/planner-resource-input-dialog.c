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
#include <libgnome/gnome-i18n.h>
#include "planner-marshal.h"
#include "planner-resource-input-dialog.h"
#include "planner-resource-cmd.h"

typedef struct {
	MrpProject *project;

	PlannerWindow *main_window;
	GtkWidget     *name_entry;
	GtkWidget     *short_name_entry;
	GtkWidget     *email_entry;
	GtkWidget     *group_option_menu;	
} DialogData;

static void resource_input_dialog_setup_groups (DialogData *data);


static void
resource_input_dialog_group_changed (MrpGroup   *group,
				     GParamSpec *spec,
				     DialogData *data)
{
	/* Seems like the easiest way is to rebuild the option menu. */
	resource_input_dialog_setup_groups (data);
}

static MrpGroup *
resource_input_dialog_get_selected (GtkWidget *option_menu)
{
	GtkWidget *menu;
	GtkWidget *item;
	MrpGroup  *ret;
	
	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (option_menu));
	if (!menu) {
		return NULL;
	}
		
	item = gtk_menu_get_active (GTK_MENU (menu));

	ret = g_object_get_data (G_OBJECT (item), "data");

	return ret;
}

static void
resource_input_dialog_setup_groups (DialogData *data)
{
	MrpGroup  *selected_group;
	GList     *groups;
	GtkWidget *option_menu;
	GtkWidget *menu;
	GtkWidget *menu_item;
	gchar     *name;
	GList     *l;
	gint       index;

	option_menu = data->group_option_menu;
	
	selected_group = resource_input_dialog_get_selected (option_menu);

	groups = mrp_project_get_groups (data->project);
	
	menu = gtk_option_menu_get_menu (GTK_OPTION_MENU (option_menu));

	if (menu) {
		gtk_widget_destroy (menu);
	}

	menu = gtk_menu_new ();

	/* Put "no group" at the top. */
	menu_item = gtk_menu_item_new_with_label (_("(None)"));
	gtk_widget_show (menu_item);
	gtk_menu_append (GTK_MENU (menu), menu_item);

	for (l = groups; l; l = l->next) {
		g_object_get (l->data,
			      "name", &name,
			      NULL);

		if (name == NULL) {
			name = g_strdup (_("(No name)"));
		}

		menu_item = gtk_menu_item_new_with_label (name);
		gtk_widget_show (menu_item);
		gtk_menu_append (GTK_MENU (menu), menu_item);

		g_object_set_data (G_OBJECT (menu_item),
				   "data",
				   l->data);

		g_signal_connect (l->data,
				  "notify::name",
				  G_CALLBACK (resource_input_dialog_group_changed),
				  data);
	}

	gtk_widget_show (menu);
	gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);

	/* Select the right group. +1 is for the empty group at the top. */
	if (groups != NULL && selected_group != NULL) {
		index = g_list_index (groups, selected_group) + 1;
	} else {
		index = 0;
	}

	gtk_option_menu_set_history (GTK_OPTION_MENU (data->group_option_menu), index);
}

static void
resource_input_dialog_groups_updated (MrpProject *project,
				      MrpGroup   *dont_care,
				      GtkWidget  *dialog)
{
	DialogData *data;

	data = g_object_get_data (G_OBJECT (dialog), "data");

	resource_input_dialog_setup_groups (data);
}

static void
resource_input_dialog_free (gpointer user_data)
{
	DialogData *data = user_data;

	g_object_unref (data->project);
	g_object_unref (data->main_window);

	g_free (data);
}

static void
resource_input_dialog_response_cb (GtkWidget *button,
				   gint       response,
				   GtkWidget *dialog)
{
	DialogData  *data;
	MrpResource *resource;
	const gchar *name;
	const gchar *short_name;
	const gchar *email;
	MrpGroup    *group;
	
	switch (response) {
	case GTK_RESPONSE_OK:
		data = g_object_get_data (G_OBJECT (dialog), "data");
		
		name = gtk_entry_get_text (GTK_ENTRY (data->name_entry));
		short_name = gtk_entry_get_text (GTK_ENTRY (data->short_name_entry));
		email = gtk_entry_get_text (GTK_ENTRY (data->email_entry));

		group = resource_input_dialog_get_selected (data->group_option_menu);
			
		resource = g_object_new (MRP_TYPE_RESOURCE,
					 "name", name,
					 "short_name", short_name,
					 "email", email,
					 "group", group,
					 NULL);
		
		/* mrp_project_add_resource (data->project, resource); */
		planner_resource_cmd_insert (data->main_window, resource);
		
		gtk_entry_set_text (GTK_ENTRY (data->name_entry), "");
		gtk_entry_set_text (GTK_ENTRY (data->short_name_entry), "");
		gtk_entry_set_text (GTK_ENTRY (data->email_entry), "");
		
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
resource_input_dialog_activate_cb (GtkWidget *widget, GtkDialog *dialog)
{
	gtk_dialog_response (dialog, GTK_RESPONSE_OK);
}

GtkWidget *
planner_resource_input_dialog_new (PlannerWindow *main_window)
{
	GtkWidget  *dialog;
	DialogData *data;
	GladeXML   *gui;
	MrpProject *project;

	project = planner_window_get_project (main_window);
	
	data = g_new0 (DialogData, 1);

	data->project = g_object_ref (project);
	data->main_window = g_object_ref (main_window);
	
	gui = glade_xml_new (GLADEDIR "/resource-input-dialog.glade",
			     NULL , NULL);
	
	dialog = glade_xml_get_widget (gui, "resource_input_dialog"); 
	g_signal_connect (dialog,
			  "response",
			  G_CALLBACK (resource_input_dialog_response_cb),
			  dialog);
	
	data->name_entry = glade_xml_get_widget (gui, "name_entry");
	g_signal_connect (data->name_entry,
			  "activate",
			  G_CALLBACK (resource_input_dialog_activate_cb),
			  dialog);
	
	data->short_name_entry = glade_xml_get_widget (gui, "short_name_entry");
	g_signal_connect (data->short_name_entry,
			  "activate",
			  G_CALLBACK (resource_input_dialog_activate_cb),
			  dialog);
			  
	data->email_entry = glade_xml_get_widget (gui, "email_entry");
	g_signal_connect (data->email_entry,
			  "activate",
			  G_CALLBACK (resource_input_dialog_activate_cb),
			  dialog);
	
	data->group_option_menu = glade_xml_get_widget (gui, "group_optionmenu");

	resource_input_dialog_setup_groups (data);

	g_signal_connect_object (project,
				 "group_added",
				 G_CALLBACK (resource_input_dialog_groups_updated),
				 dialog,
				 0);
	
	g_signal_connect_object (project,
				 "group_removed",
				 G_CALLBACK (resource_input_dialog_groups_updated),
				 dialog,
				 0);
	
	g_object_set_data_full (G_OBJECT (dialog),
				"data",
				data,
				resource_input_dialog_free);
	
        return dialog;
}
