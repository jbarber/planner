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

#ifndef __MG_VIEW_H__
#define __MG_VIEW_H__

#include <gmodule.h>
#include <gtk/gtkwidget.h>
#include <bonobo/bonobo-ui-component.h>
#include "planner-main-window.h"
#include "planner-print-job.h"


#define MG_TYPE_VIEW		 (planner_view_get_type ())
#define MG_VIEW(obj)		 (G_TYPE_CHECK_INSTANCE_CAST ((obj), MG_TYPE_VIEW, MgView))
#define MG_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass), MG_TYPE_VIEW, MgViewClass))
#define MG_IS_VIEW(obj)		 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MG_TYPE_VIEW))
#define MG_IS_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MG_TYPE_VIEW))
#define MG_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MG_TYPE_VIEW, MgViewClass))

typedef struct _MgView       MgView;
typedef struct _MgViewClass  MgViewClass;
typedef struct _MgViewPriv   MgViewPriv;

struct _MgView {
	GObject            parent;

	GModule           *handle;
	MgMainWindow      *main_window;
	BonoboUIComponent *ui_component;

	MgViewPriv        *priv;
	gboolean           activated;
	
	/* Methods. */
	const gchar *(*get_label)         (MgView       *view);
	const gchar *(*get_menu_label)    (MgView       *view);
	const gchar *(*get_icon)          (MgView       *view);
	GtkWidget   *(*get_widget)        (MgView       *view);
	void         (*init)              (MgView       *view,
					   MgMainWindow *main_window);
	void         (*activate)          (MgView       *view);
	void         (*deactivate)        (MgView       *view);
	
	void         (*print_init)        (MgView       *view,
					   MgPrintJob   *job);
	gint         (*print_get_n_pages) (MgView       *view);
	void         (*print)             (MgView       *view);
	void         (*print_cleanup)     (MgView       *view);
};

struct _MgViewClass {
	GObjectClass parent_class;
};

GType        planner_view_get_type          (void);

const gchar *planner_view_get_label         (MgView       *view);

const gchar *planner_view_get_menu_label    (MgView       *view);

const gchar *planner_view_get_icon          (MgView       *view);

GtkWidget   *planner_view_get_widget        (MgView       *view);

void         planner_view_init              (MgView       *view,
					MgMainWindow *main_window);

void         planner_view_activate_helper   (MgView       *view,
					const gchar  *ui_filename,
					const gchar  *name,
					BonoboUIVerb *verbs);

void         planner_view_deactivate_helper (MgView       *view);

void         planner_view_activate          (MgView       *view);

void         planner_view_deactivate        (MgView       *view);

void         planner_view_print_init        (MgView       *view,
					MgPrintJob   *job);

gint         planner_view_print_get_n_pages (MgView       *view);

void         planner_view_print             (MgView       *view);

void         planner_view_print_cleanup     (MgView       *view);


#endif /* __MG_VIEW_H__ */

