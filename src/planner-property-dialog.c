/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <richard@imendio.com>
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
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <libgnome/gnome-i18n.h>
#include <glade/glade.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtkoptionmenu.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkcellrenderertoggle.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkmain.h>
#include <libplanner/mrp-object.h>
#include <libplanner/mrp-property.h>
#include "planner-cell-renderer-list.h"
#include "planner-property-dialog.h"
#include "planner-property-model.h"

#define GET_PRIV(d) (g_object_get_data (G_OBJECT (d), "priv"))


typedef struct {
	GtkWidget    *tree;
	GtkTreeModel *model;
	MrpProject   *project;
	GType         type;
} PlannerPropertyDialogPriv;

static void
property_dialog_setup_option_menu (GtkWidget     *option_menu,
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
property_dialog_get_selected (GtkWidget *option_menu)
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
property_dialog_close_cb (GtkWidget *button,
			  GtkWidget *dialog)
{
	gtk_widget_destroy (dialog);
}

static void
property_dialog_type_selected_cb (GtkWidget *widget,
				  GtkWidget *dialog)
{
	gint type;
	
	type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "data"));

	g_object_set_data (G_OBJECT (dialog), "type", GINT_TO_POINTER (type));
}

static gboolean
property_dialog_label_changed_cb (GtkWidget *label_entry,
				  GdkEvent  *event,
				  GtkWidget *name_entry)
{
	const gchar *name;
	const gchar *label;

	name = gtk_entry_get_text (GTK_ENTRY (name_entry));

	if (name == NULL || name[0] == 0) {
		label = gtk_entry_get_text (GTK_ENTRY (label_entry));
		gtk_entry_set_text (GTK_ENTRY (name_entry), label);
	}

	return FALSE;
}

static void
property_dialog_add_cb (GtkWidget *button,
			GtkWidget *dialog)
{
	PlannerPropertyDialogPriv *priv;
	MrpProperty          *property;
	MrpPropertyType       type;
	const gchar          *label;
	const gchar          *name;
	const gchar          *description;
	GladeXML             *glade;
	GtkWidget            *label_entry;
	GtkWidget            *name_entry;
	GtkWidget            *add_dialog;
	GtkWidget            *w;
	gint                  response;
	gboolean              finished = FALSE;
	
	priv = GET_PRIV (dialog);

	glade = glade_xml_new (GLADEDIR "/new-property.glade",
			       NULL,
			       NULL);
		
	add_dialog = glade_xml_get_widget (glade, "add_dialog");

	label_entry = glade_xml_get_widget (glade, "label_entry");
	name_entry = glade_xml_get_widget (glade, "name_entry");
	
	g_signal_connect (label_entry,
			  "focus_out_event",
			  G_CALLBACK (property_dialog_label_changed_cb),
			  name_entry);

	property_dialog_setup_option_menu (
		glade_xml_get_widget (glade, "type_menu"),
		G_CALLBACK (property_dialog_type_selected_cb),
		add_dialog,
		mrp_property_type_as_string (MRP_PROPERTY_TYPE_STRING),
		MRP_PROPERTY_TYPE_STRING,
		mrp_property_type_as_string (MRP_PROPERTY_TYPE_INT),
		MRP_PROPERTY_TYPE_INT,
		mrp_property_type_as_string (MRP_PROPERTY_TYPE_FLOAT),
		MRP_PROPERTY_TYPE_FLOAT,
/*  		mrp_property_type_as_string (MRP_PROPERTY_TYPE_DATE), */
/*  		MRP_PROPERTY_TYPE_DATE, */
/*  		mrp_property_type_as_string (MRP_PROPERTY_TYPE_DURATION), */
/*  		MRP_PROPERTY_TYPE_DURATION, */
/* 		mrp_property_type_as_string (MRP_PROPERTY_TYPE_COST), */
/*  		MRP_PROPERTY_TYPE_COST, */
		NULL);

	while (!finished) {
		response = gtk_dialog_run (GTK_DIALOG (add_dialog));

		switch (response) {
		case GTK_RESPONSE_OK:
			label = gtk_entry_get_text (GTK_ENTRY (label_entry));
			if (label == NULL || label[0] == 0) {
				finished = FALSE;
				break;
			}
			
			name = gtk_entry_get_text (GTK_ENTRY (name_entry));
			if (name == NULL || name[0] == 0) {
				finished = FALSE;
				break;
			}
			
			if (!isalpha(name[0])) {
				GtkWidget *msg_dialog;
				
				msg_dialog = gtk_message_dialog_new (GTK_WINDOW (add_dialog),
								     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
								     GTK_MESSAGE_WARNING,
								     GTK_BUTTONS_OK,
								     _("The name of the custom property needs to start with a letter."));
				gtk_dialog_run (GTK_DIALOG (msg_dialog));
				gtk_widget_destroy (msg_dialog);
				
				finished = FALSE;
				break;
			}
		
			w = glade_xml_get_widget (glade, "description_entry");
			description = gtk_entry_get_text (GTK_ENTRY (w));
			
			w = glade_xml_get_widget (glade, "type_menu");
			
			type = property_dialog_get_selected (w);

			if (type != MRP_PROPERTY_TYPE_NONE) {
				property = mrp_property_new (name, 
							     type,
							     label,
							     description,
							     TRUE);

				mrp_project_add_property (priv->project, 
							  priv->type,
							  property,
							  TRUE);	
			}

			finished = TRUE;
			break;
			
		case GTK_RESPONSE_DELETE_EVENT:
		case GTK_RESPONSE_CANCEL:
			finished = TRUE;
			break;
			
		default:
			break;
		}
	}
	
 	gtk_widget_destroy (add_dialog);
}

static void
property_dialog_remove_cb (GtkWidget *button, GtkWidget *dialog)
{
	PlannerPropertyDialogPriv *priv;
	GtkTreeSelection     *selection;
	GtkTreeIter           iter;
	gchar                *name;
	GtkWidget            *remove_dialog;
	gint                  response;

	priv = GET_PRIV (dialog);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree));

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter)) {
		return;
	}

	gtk_tree_model_get (priv->model, &iter,
			    COL_NAME, &name,
			    -1);

	remove_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
						GTK_DIALOG_MODAL |
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_QUESTION,
						GTK_BUTTONS_YES_NO,
						_("Do you really want to remove the property '%s' from "
						  "the project?"),
						name);
	
	response = gtk_dialog_run (GTK_DIALOG (remove_dialog));

	switch (response) {
	case GTK_RESPONSE_YES:
		mrp_project_remove_property (priv->project,
					     priv->type,
					     name);
		break;

	case GTK_RESPONSE_DELETE_EVENT:
	case GTK_RESPONSE_CANCEL:
		break;

	default:
		break;
	}

	gtk_widget_destroy (remove_dialog);
	
	g_free (name);
}

static void  
property_dialog_label_edited (GtkCellRendererText *cell, 
			      gchar               *path_str,
			      gchar               *new_text, 
			      GtkWidget           *dialog)
{
	PlannerPropertyDialogPriv *priv;
	GtkTreePath          *path;
	GtkTreeIter           iter;
	GtkTreeModel         *model;
	MrpProperty          *property;
	
	priv = GET_PRIV (dialog);

	model = priv->model;
	path = gtk_tree_path_new_from_string (path_str);

	gtk_tree_model_get_iter (model, &iter, path);

	gtk_tree_model_get (priv->model, &iter,
			    COL_PROPERTY, &property,
			    -1);

	mrp_property_set_label (property, new_text);

	gtk_tree_path_free (path);
}

static void
property_dialog_setup_list (GtkWidget *dialog,
			    guint      cols)
{
	PlannerPropertyDialogPriv *priv;
	GtkTreeView          *tree;
	GtkTreeViewColumn    *col;
	GtkCellRenderer      *cell;
	GtkTreeModel         *model;

	priv = g_object_get_data (G_OBJECT (dialog), "priv");

	tree = GTK_TREE_VIEW (priv->tree);

	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (tree),
				     GTK_SELECTION_SINGLE);
 
	gtk_tree_view_set_headers_visible (tree, TRUE);

	if (cols | COL_NAME) {
		/* Name */
		cell = gtk_cell_renderer_text_new ();
		g_object_set (G_OBJECT (cell), "editable", FALSE, NULL);
		
		col = gtk_tree_view_column_new_with_attributes (_("Name"),
								cell,
								"text", COL_NAME,
								NULL);
		gtk_tree_view_column_set_resizable (col, TRUE);
		gtk_tree_view_column_set_min_width (col, 100);
		gtk_tree_view_append_column (tree, col);
	}

	if (cols | COL_LABEL) {
		/* Label */
		cell = gtk_cell_renderer_text_new ();
		g_object_set (G_OBJECT (cell), "editable", TRUE, NULL);
		
		g_signal_connect (G_OBJECT (cell),
				  "edited",
				  G_CALLBACK (property_dialog_label_edited),
				  dialog);
		
		col = gtk_tree_view_column_new_with_attributes (_("Label"),
								cell,
								"text", COL_LABEL,
								NULL);
		gtk_tree_view_column_set_resizable (col, TRUE);
		gtk_tree_view_column_set_min_width (col, 200);
		gtk_tree_view_append_column (tree, col);
	}

	if (cols | COL_TYPE) {
		/* Type */
		cell = gtk_cell_renderer_text_new ();
		g_object_set (G_OBJECT (cell), "editable", FALSE, NULL);
		
		col = gtk_tree_view_column_new_with_attributes (_("Type"),
								cell,
								"text", COL_TYPE,
								NULL);
		gtk_tree_view_column_set_resizable (col, TRUE);
		/* gtk_tree_view_column_set_min_width (col, 200); */
		gtk_tree_view_append_column (tree, col);
	}

	if (cols | COL_VALUE) {
#if 0
		/* Value */
		cell = gtk_cell_renderer_text_new ();
		/*g_object_set (G_OBJECT (cell), "editable", TRUE, NULL);
		g_signal_connect (G_OBJECT (cell),
				  "edited",
				  G_CALLBACK (property_dialog_value_edited),
				  dialog);*/
		
		col = gtk_tree_view_column_new ();
		gtk_tree_view_column_set_title (col, _("Value"));
		gtk_tree_view_column_pack_start (col, cell,TRUE);
		gtk_tree_view_column_set_cell_data_func (col,
							 cell,
							 property_dialog_value_data_func,
							 dialog,
							 NULL);

		gtk_tree_view_column_set_resizable (col, TRUE);
		/* gtk_tree_view_column_set_min_width (col, 200); */
		gtk_tree_view_append_column (tree, col);
#endif
	}

	
	model = planner_property_model_new (priv->project, priv->type);
	priv->model = model;
	
	gtk_tree_view_set_model (tree, model);
}

static void
property_dialog_setup_widgets (GtkWidget *dialog,
			       GladeXML  *glade)
{
	PlannerPropertyDialogPriv *priv;
	GtkWidget            *w;

	priv = GET_PRIV (dialog);

	w = glade_xml_get_widget (glade, "close_button");
	g_signal_connect (w,
			  "clicked",
			  G_CALLBACK (property_dialog_close_cb),
			  dialog);

	w = glade_xml_get_widget (glade, "add_button");
	g_signal_connect (w,
			  "clicked",
			  G_CALLBACK (property_dialog_add_cb),
			  dialog);

	w = glade_xml_get_widget (glade, "remove_button");
	g_signal_connect (w,
			  "clicked",
			  G_CALLBACK (property_dialog_remove_cb),
			  dialog);

	w = glade_xml_get_widget (glade, "treeview");
	priv->tree = w;

	property_dialog_setup_list (dialog, COL_NAME | COL_LABEL | COL_TYPE | COL_VALUE);
}

GtkWidget *
planner_property_dialog_new (MrpProject  *project,
			GType        owner_type,
			const gchar *title)
{
	GladeXML             *glade;
	GtkWidget            *dialog;
	PlannerPropertyDialogPriv *priv;

	g_return_val_if_fail (MRP_IS_PROJECT (project), NULL);

	priv = g_new0 (PlannerPropertyDialogPriv, 1);
	
	glade = glade_xml_new (GLADEDIR"/property-dialog.glade",
			       NULL, NULL);
		
	dialog = glade_xml_get_widget (glade, "dialog");

	gtk_window_set_title (GTK_WINDOW (dialog), title);
	
	g_object_set_data (G_OBJECT (dialog), "priv", priv);

	priv->type = owner_type;
	priv->project = project;
	
	property_dialog_setup_widgets (dialog, glade);

	return dialog;
}

void  
planner_property_dialog_value_edited (GtkCellRendererText *cell, 
				 gchar               *path_str,
				 gchar               *new_text, 
				 gpointer             data)
{
	/*PlannerPropertyDialogPriv *priv;*/
	GtkTreePath          *path;
	GtkTreeIter           iter;
	GtkTreeModel         *model;
	MrpProperty          *property;
	
	/*priv = GET_PRIV (dialog);*/

	/*model = priv->model;*/
	model = data;
	path = gtk_tree_path_new_from_string (path_str);

	gtk_tree_model_get_iter (model, &iter, path);

	gtk_tree_model_get (model, 
			    &iter,
			    COL_PROPERTY, &property,
			    -1);

	/*	mrp_object_set (priv->object,
			mrp_property_get_name (property), new_text,
			NULL);
	*/
	g_print ("edited: %s\n", new_text);

	gtk_tree_path_free (path);
}
