/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003 Imendio HB
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
#include <gtk/gtkmain.h>
#include <gtk/gtkstock.h>
#include <libgnome/gnome-i18n.h>
#include "planner-main-window.h"
#include "planner-application.h"



#define d(x) 

struct _MgApplicationPriv {
	GList *windows;

	/* recent file stuff */
	EggRecentModel *recent_model;
};


static void     application_class_init       (MgApplicationClass *klass);
static void     application_init             (MgApplication      *page);
static void     application_finalize         (GObject            *object);
static void     application_window_closed_cb (MgMainWindow       *window,
					      MgApplication      *application);


static MrpApplicationClass *parent_class = NULL;

static GtkStockItem stock_items[] = {
	{
		"planner-stock-insert-task",
		N_("Insert"),
		0, /*GdkModifierType*/
		0, /*keyval*/
		GETTEXT_PACKAGE
	},
	{
		"planner-stock-remove-task",
		N_("Remove"),
		0, /*GdkModifierType*/
		0, /*keyval*/
		GETTEXT_PACKAGE
	},
	{
		"planner-stock-unlink-task",
		N_("Unlink"),
		0, /*GdkModifierType*/
		0, /*keyval*/
		GETTEXT_PACKAGE
	},
	{
		"planner-stock-indent-task",
		N_("Indent"),
		0, /*GdkModifierType*/
		0, /*keyval*/
		GETTEXT_PACKAGE
	},
	{
		"planner-stock-unindent-task",
		N_("Unindent"),
		0, /*GdkModifierType*/
		0, /*keyval*/
		GETTEXT_PACKAGE
	},
	{
		"planner-stock-move-task-up",
		N_("Move up"),
		0, /*GdkModifierType*/
		0, /*keyval*/
		GETTEXT_PACKAGE
	},
	{
		"planner-stock-move-task-down",
		N_("Move down"),
		0, /*GdkModifierType*/
		0, /*keyval*/
		GETTEXT_PACKAGE
	},
	{
		"planner-stock-insert-resource",
		N_("Insert"),
		0, /*GdkModifierType*/
		0, /*keyval*/
		GETTEXT_PACKAGE
	},
       	{
		"planner-stock-remove-resource",
		N_("Remove"),
		0, /*GdkModifierType*/
		0, /*keyval*/
		GETTEXT_PACKAGE
	},
	{
		"planner-stock-edit-groups",
		N_("Edit"),
		0, /*GdkModifierType*/
		0, /*keyval*/
		GETTEXT_PACKAGE
	}

};


GType
planner_application_get_type (void)
{
	static GtkType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (MgApplicationClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) application_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (MgApplication),
			0,              /* n_preallocs */
			(GInstanceInitFunc) application_init
		};

		type = g_type_register_static (MRP_TYPE_APPLICATION,
					       "MgApplication", &info, 0);
	}
	
	return type;
}

static void
application_class_init (MgApplicationClass *klass)
{
	GObjectClass *o_class;
	
	parent_class = g_type_class_peek_parent (klass);
	
	o_class = (GObjectClass *) klass;

	/* GObject signals */
	o_class->finalize = application_finalize;

	gtk_stock_add_static (stock_items, G_N_ELEMENTS (stock_items));
	
}

static void
application_init (MgApplication *app)
{
	MgApplicationPriv *priv;

	priv = g_new0 (MgApplicationPriv, 1);

	priv->windows = NULL;
	
	priv->recent_model = egg_recent_model_new (EGG_RECENT_MODEL_SORT_MRU);
	egg_recent_model_set_filter_mime_types (priv->recent_model, "application/x-mrproject", NULL);
	egg_recent_model_set_filter_uri_schemes (priv->recent_model, "file", NULL);

	g_object_set (priv->recent_model, "limit", 5, NULL);
	
	app->priv = priv;
}

static void
application_finalize (GObject *object)
{
	MgApplication *app = MG_APPLICATION (object);

	g_object_unref (app->priv->recent_model);
	
	g_free (app->priv);

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}

static void
application_window_closed_cb (MgMainWindow  *window,
		      MgApplication *application)
{
	MgApplicationPriv *priv;
	
	g_return_if_fail (MG_IS_MAIN_WINDOW (window));
	g_return_if_fail (MG_IS_APPLICATION (application));
	
	priv = application->priv;
	
	priv->windows = g_list_remove (priv->windows, window);

	d(g_print ("Window deleted\n"));
	
	if (g_list_length (priv->windows) == 0) {
		d(g_print ("Number of windows == 0\n"));
		gtk_main_quit ();
	}
}

MgApplication *
planner_application_new (void)
{
	MgApplication     *app;
	MgApplicationPriv *priv;
	
	app  = MG_APPLICATION (g_object_new (MG_TYPE_APPLICATION, NULL));
	priv = app->priv;
	
	return app;
}

GtkWidget *
planner_application_new_window (MgApplication *app)
{
	MgApplicationPriv *priv;
	GtkWidget         *window;

	g_return_val_if_fail (MG_IS_APPLICATION (app), NULL);

	priv = app->priv;

	window = planner_main_window_new (app);

	g_signal_connect (window,
			  "closed",
			  G_CALLBACK (application_window_closed_cb),
			  app);

	priv->windows = g_list_prepend (priv->windows, window);

	return window;
}

void
planner_application_exit (MgApplication *app)
{
	MgApplicationPriv *priv;
	GList             *list_cpy;
	GList             *l;
	
	g_return_if_fail (MG_IS_APPLICATION (app));
	
	priv = app->priv;

	list_cpy = g_list_copy (priv->windows);

	for (l = list_cpy; l; l = l->next) {
		planner_main_window_close (MG_MAIN_WINDOW (l->data));
	}
	
	g_list_free (list_cpy);
}

EggRecentModel *
planner_application_get_recent_model (MgApplication *app)
{
	g_return_val_if_fail (MG_IS_APPLICATION (app), NULL);

	return app->priv->recent_model;
}

GConfClient *
planner_application_get_gconf_client (MgApplication *app)
{
	static GConfClient *client;
	
	g_return_val_if_fail (MG_IS_APPLICATION (app), NULL);

	if (!client) {
		client = gconf_client_get_default ();
	}

	return client;
}

