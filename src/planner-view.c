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
#include <string.h>
#include <time.h>
#include <glib.h>
#include <bonobo/bonobo-ui-util.h>
#include <bonobo/bonobo-ui-util.h>
#include "planner-view.h"

static void mv_init       (MgView      *view);
static void mv_class_init (MgViewClass *class);


static GObjectClass *parent_class;


GType
planner_view_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (MgViewClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) mv_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (MgView),
			0,
			(GInstanceInitFunc) mv_init
		};

		type = g_type_register_static (G_TYPE_OBJECT,
					       "MgView", &info, 0);
	}

	return type;
}

static void
mv_finalize (GObject *object)
{
	(* parent_class->finalize) (object);
}

static void
mv_class_init (MgViewClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass *) klass;

	parent_class = g_type_class_peek_parent (klass);
	
	object_class->finalize = mv_finalize;
}

static void
mv_init (MgView *view)
{

}

const gchar *
planner_view_get_label (MgView *view)
{
	g_return_val_if_fail (MG_IS_VIEW (view), NULL);

	if (view->get_label) {
		return view->get_label (view);
	}

	return NULL;
}

const gchar *
planner_view_get_menu_label (MgView *view)
{
	g_return_val_if_fail (MG_IS_VIEW (view), NULL);

	if (view->get_menu_label) {
		return view->get_menu_label (view);
	}

	return NULL;
}

const gchar *
planner_view_get_icon (MgView *view)
{
	g_return_val_if_fail (MG_IS_VIEW (view), NULL);

	if (view->get_icon) {
		return view->get_icon (view);
	}

	return NULL;
}

GtkWidget *
planner_view_get_widget (MgView *view)
{
	g_return_val_if_fail (MG_IS_VIEW (view), NULL);

	if (view->get_widget) {
		return view->get_widget (view);
	}

	return NULL;
}

void
planner_view_init (MgView       *view,
	      MgMainWindow *main_window)
{
	g_return_if_fail (MG_IS_VIEW (view));

	view->main_window = main_window;
	view->ui_component = bonobo_ui_component_new_default ();

	if (view->init) {
		view->init (view, main_window);
	}
}

void
planner_view_activate (MgView *view)
{
	g_return_if_fail (MG_IS_VIEW (view));

	view->activated = TRUE;
	
	if (view->activate) {
		view->activate (view);
	}
}

void
planner_view_deactivate (MgView *view)
{
	g_return_if_fail (MG_IS_VIEW (view));

	view->activated = FALSE;

	if (view->deactivate) {
		view->deactivate (view);
	}
}

void
planner_view_print_init (MgView     *view,
		    MgPrintJob *job)
{
	g_return_if_fail (MG_IS_VIEW (view));
	g_return_if_fail (MG_IS_PRINT_JOB (job));

	if (view->print_init) {
		view->print_init (view, job);
	}
}

gint
planner_view_print_get_n_pages (MgView *view)
{
	g_return_val_if_fail (MG_IS_VIEW (view), 0);
	
	if (view->print_get_n_pages) {
		return view->print_get_n_pages (view);
	}
	
	return 0;
}

void
planner_view_print (MgView *view)
{
	g_return_if_fail (MG_IS_VIEW (view));

	if (view->print) {
		view->print (view);
	}
}

void
planner_view_print_cleanup (MgView *view)
{
	g_return_if_fail (MG_IS_VIEW (view));

	if (view->print_cleanup) {
		view->print_cleanup (view);
	}
}

void
planner_view_activate_helper (MgView       *view,
			 const gchar  *ui_filename,
			 const gchar  *name,
			 BonoboUIVerb *verbs)
{
	BonoboUIContainer *ui_container;

	g_return_if_fail (MG_IS_VIEW (view));
	
	ui_container = planner_main_window_get_ui_container (view->main_window);
	
	bonobo_ui_component_set_container (view->ui_component,
					   BONOBO_OBJREF (ui_container),
					   NULL);
	
	bonobo_ui_component_freeze (view->ui_component, NULL);

	bonobo_ui_component_add_verb_list_with_data (view->ui_component,
						     verbs,
						     view);
	
	bonobo_ui_util_set_ui (view->ui_component,
 			       DATADIR,
 			       ui_filename,
 			       name,
 			       NULL);

	bonobo_ui_component_thaw (view->ui_component, NULL);
}

void
planner_view_deactivate_helper (MgView *view)
{
	g_return_if_fail (MG_IS_VIEW (view));

	bonobo_ui_component_rm (view->ui_component, "/", NULL);
 	bonobo_ui_component_unset_container (view->ui_component, NULL);
}

