/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003 Imendio AB
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "planner-window.h"
#include "planner-application.h"



#define d(x)

struct _PlannerApplicationPriv {
	GList *windows;

	/* recent file stuff */
	GtkRecentManager *recent_model;
};


static void     application_class_init       (PlannerApplicationClass *klass);
static void     application_init             (PlannerApplication      *page);
static void     application_finalize         (GObject            *object);
static void     application_window_closed_cb (PlannerWindow       *window,
					      PlannerApplication      *application);


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
		"planner-stock-link-tasks",
		N_("Link"),
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
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (PlannerApplicationClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) application_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (PlannerApplication),
			0,              /* n_preallocs */
			(GInstanceInitFunc) application_init
		};

		type = g_type_register_static (MRP_TYPE_APPLICATION,
					       "PlannerApplication", &info, 0);
	}

	return type;
}

static void
application_class_init (PlannerApplicationClass *klass)
{
	GObjectClass *o_class;

	parent_class = g_type_class_peek_parent (klass);

	o_class = (GObjectClass *) klass;

	/* GObject signals */
	o_class->finalize = application_finalize;

	gtk_stock_add_static (stock_items, G_N_ELEMENTS (stock_items));

}

static void
application_init (PlannerApplication *app)
{
	PlannerApplicationPriv *priv;

	priv = g_new0 (PlannerApplicationPriv, 1);

	priv->windows = NULL;

	priv->recent_model = gtk_recent_manager_get_default ();

	app->priv = priv;
}

static void
application_finalize (GObject *object)
{
	PlannerApplication *app = PLANNER_APPLICATION (object);

	g_free (app->priv);

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}

static void
application_window_closed_cb (PlannerWindow  *window,
		      PlannerApplication *application)
{
	PlannerApplicationPriv *priv;

	g_return_if_fail (PLANNER_IS_WINDOW (window));
	g_return_if_fail (PLANNER_IS_APPLICATION (application));

	priv = application->priv;

	priv->windows = g_list_remove (priv->windows, window);

	d(g_print ("Window deleted\n"));

	if (g_list_length (priv->windows) == 0) {
		d(g_print ("Number of windows == 0\n"));
		gtk_main_quit ();
	}
}

PlannerApplication *
planner_application_new (void)
{
	PlannerApplication     *app;

	app  = PLANNER_APPLICATION (g_object_new (PLANNER_TYPE_APPLICATION, NULL));

	return app;
}

GtkWidget *
planner_application_new_window (PlannerApplication *app)
{
	PlannerApplicationPriv *priv;
	GtkWidget         *window;

	g_return_val_if_fail (PLANNER_IS_APPLICATION (app), NULL);

	priv = app->priv;

	window = planner_window_new (app);

	g_signal_connect (window,
			  "closed",
			  G_CALLBACK (application_window_closed_cb),
			  app);

	priv->windows = g_list_prepend (priv->windows, window);

	return window;
}

void
planner_application_exit (PlannerApplication *app)
{
	PlannerApplicationPriv *priv;
	GList             *list_cpy;
	GList             *l;

	g_return_if_fail (PLANNER_IS_APPLICATION (app));

	priv = app->priv;

	list_cpy = g_list_copy (priv->windows);

	for (l = list_cpy; l; l = l->next) {
		planner_window_close (PLANNER_WINDOW (l->data));
	}

	g_list_free (list_cpy);
}

GtkRecentManager *
planner_application_get_recent_model (PlannerApplication *app)
{
	g_return_val_if_fail (PLANNER_IS_APPLICATION (app), NULL);

	return app->priv->recent_model;
}

