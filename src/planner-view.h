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

#ifndef __PLANNER_VIEW_H__
#define __PLANNER_VIEW_H__

#include <gmodule.h>
#include <gtk/gtkwidget.h>
#include <bonobo/bonobo-ui-component.h>
#include "planner-window.h"
#include "planner-print-job.h"


#define PLANNER_TYPE_VIEW		 (planner_view_get_type ())
#define PLANNER_VIEW(obj)		 (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLANNER_TYPE_VIEW, PlannerView))
#define PLANNER_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), PLANNER_TYPE_VIEW, PlannerViewClass))
#define PLANNER_IS_VIEW(obj)		 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLANNER_TYPE_VIEW))
#define PLANNER_IS_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), PLANNER_TYPE_VIEW))
#define PLANNER_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), PLANNER_TYPE_VIEW, PlannerViewClass))

typedef struct _PlannerView       PlannerView;
typedef struct _PlannerViewClass  PlannerViewClass;
typedef struct _PlannerViewPriv   PlannerViewPriv;

struct _PlannerView {
	GObject            parent;

	GModule           *handle;
	PlannerWindow     *main_window;
	BonoboUIComponent *ui_component;

	PlannerViewPriv   *priv;
	gboolean           activated;
	
	/* Methods. */
	const gchar *(*get_label)         (PlannerView       *view);
	const gchar *(*get_menu_label)    (PlannerView       *view);
	const gchar *(*get_icon)          (PlannerView       *view);
	const gchar *(*get_name)          (PlannerView       *view);
	GtkWidget   *(*get_widget)        (PlannerView       *view);
	void         (*init)              (PlannerView       *view,
					   PlannerWindow     *window);
	void         (*activate)          (PlannerView       *view);
	void         (*deactivate)        (PlannerView       *view);
	
	void         (*print_init)        (PlannerView       *view,
					   PlannerPrintJob   *job);
	gint         (*print_get_n_pages) (PlannerView       *view);
	void         (*print)             (PlannerView       *view);
	void         (*print_cleanup)     (PlannerView       *view);
};

struct _PlannerViewClass {
	GObjectClass parent_class;
};

GType        planner_view_get_type          (void) G_GNUC_CONST;
const gchar *planner_view_get_label         (PlannerView     *view);
const gchar *planner_view_get_menu_label    (PlannerView     *view);
const gchar *planner_view_get_icon          (PlannerView     *view);
const gchar *planner_view_get_name          (PlannerView     *view);
GtkWidget   *planner_view_get_widget        (PlannerView     *view);
void         planner_view_init              (PlannerView     *view,
					     PlannerWindow   *window);
void         planner_view_activate_helper   (PlannerView     *view,
					     const gchar     *ui_filename,
					     const gchar     *name,
					     BonoboUIVerb    *verbs);
void         planner_view_deactivate_helper (PlannerView     *view);
void         planner_view_activate          (PlannerView     *view);
void         planner_view_deactivate        (PlannerView     *view);
void         planner_view_print_init        (PlannerView     *view,
					     PlannerPrintJob *job);
gint         planner_view_print_get_n_pages (PlannerView     *view);
void         planner_view_print             (PlannerView     *view);
void         planner_view_print_cleanup     (PlannerView     *view);


#endif /* __PLANNER_VIEW_H__ */

