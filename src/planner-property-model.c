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
#include <string.h>
#include <libgnome/gnome-i18n.h>
#include <gtk/gtkliststore.h>
#include <libplanner/mrp-property.h>
#include "planner-property-model.h"


static void
property_model_property_added_cb (MrpProject   *project,
				  GType         owner_type,
				  MrpProperty  *property,
				  MrpPropertyStore  *shop)
{
	GtkTreeIter     iter;
	MrpPropertyType type;
	GtkListStore    *store;

	if (owner_type != shop->owner_type) {  
		return;
	}

	store = shop->store;

	if (store) {
	type = mrp_property_get_property_type (property);
	
	gtk_list_store_append (store, &iter);

	gtk_list_store_set (store, &iter,
			    COL_NAME, mrp_property_get_name (property),
			    COL_LABEL, mrp_property_get_label (property),
			    COL_TYPE, mrp_property_type_as_string (type),
			    COL_PROPERTY, property,
			    -1);
	}
}

static gboolean
property_model_property_removed_helper (GtkTreeModel *model,
					GtkTreePath  *path,
					GtkTreeIter  *iter,
					gpointer      data)
{
	gchar *name;
	
	gtk_tree_model_get (model, iter,
			    COL_NAME, &name,
			    -1);
	
	if (!strcmp (name, data)) {
		gtk_list_store_remove (GTK_LIST_STORE (model), iter);
		
		g_free (name);
		return TRUE;
	}

	g_free (name);

	return FALSE;
}

static void
property_model_property_removed_cb (MrpProject   *project,
				    MrpProperty  *property,
				    GtkTreeModel *model)
{
	gtk_tree_model_foreach (model,
				property_model_property_removed_helper,
				(gchar *)mrp_property_get_name (property));
}

static gboolean 
property_model_property_changed_helper (GtkTreeModel *model,
					GtkTreePath  *path,
					GtkTreeIter  *iter,
					gpointer      data)
{
	MrpProperty *property;
	const gchar *property_name;
	gchar       *name;

	g_return_val_if_fail (data != NULL, FALSE);
	
	property      = MRP_PROPERTY (data);
	property_name = mrp_property_get_name (property);
	
	gtk_tree_model_get (model, iter,
			    COL_NAME, &name,
			    -1);
	
	if (!strcmp (name, property_name)) {
		gtk_list_store_set (GTK_LIST_STORE (model), iter,
				    COL_LABEL, mrp_property_get_label (property),
				    -1);
		return TRUE;
	}

	return FALSE;
}

static void
property_model_property_changed_cb (MrpProject   *project,
				    MrpProperty  *property,
				    GtkTreeModel *model)
{
	/* Find the iter and update it */
	gtk_tree_model_foreach (model, 
				property_model_property_changed_helper,
				property);
}

GtkTreeModel *
planner_property_model_new (MrpProject *project,
		            GType      		owner_type,
			    MrpPropertyStore	*shop)
{
	GtkListStore    *store;
	GList           *properties, *l;
	MrpProperty     *property;
	MrpPropertyType  type;
	GtkTreeIter      iter;
	
	store = gtk_list_store_new (5,
				    G_TYPE_STRING,
				    G_TYPE_STRING,
				    G_TYPE_STRING,
				    G_TYPE_POINTER,
				    G_TYPE_POINTER);
	
	shop->store = store;
	shop->owner_type = owner_type;

	properties = mrp_project_get_properties_from_type (project, 
							   owner_type);

	for (l = properties; l; l = l->next) {
		property = l->data;

		type = mrp_property_get_property_type (property);
		
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store,
				    &iter,
				    COL_NAME, mrp_property_get_name (property),
				    COL_LABEL, mrp_property_get_label (property),
				    COL_TYPE, mrp_property_type_as_string (type),
				    COL_PROPERTY, property,
				    -1);
	}

	/* We need to know which store to add the property so we pass the shop 
	*  reference not the store. The shop is a structure that correlates 
	*  which store currently holds which owner. We don't have to bother with
	*  this when changing or removing - just adding.
	*/ 
	g_signal_connect (project,
			  "property_added",
			  G_CALLBACK (property_model_property_added_cb),
			  shop);
	
	g_signal_connect (project,
			  "property_removed",
			  G_CALLBACK (property_model_property_removed_cb),
			  store);

	g_signal_connect (project,
			  "property_changed",
			  G_CALLBACK (property_model_property_changed_cb),
			  store);

	return GTK_TREE_MODEL (store);
}


