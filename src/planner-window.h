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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __PLANNER_WINDOW_H__
#define __PLANNER_WINDOW_H__

#include <libplanner/mrp-project.h>
#include <gtk/gtk.h>
#include "planner-application.h"
#include "planner-cmd-manager.h"

#define PLANNER_TYPE_WINDOW            (planner_window_get_type ())
#define PLANNER_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLANNER_TYPE_WINDOW, PlannerWindow))
#define PLANNER_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_WINDOW, PlannerWindowClass))
#define PLANNER_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLANNER_TYPE_WINDOW))
#define PLANNER_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PLANNER_TYPE_WINDOW))
#define PLANNER_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PLANNER_TYPE_WINDOW, PlannerWindowClass))

typedef struct _PlannerWindow        PlannerWindow;
typedef struct _PlannerWindowClass   PlannerWindowClass;
typedef struct _PlannerWindowPriv    PlannerWindowPriv;

struct _PlannerWindow {
        GtkWindow           parent;

        PlannerWindowPriv  *priv;
};

struct _PlannerWindowClass {
        GtkWindowClass      parent_class;
};

GType               planner_window_get_type                (void) G_GNUC_CONST;
GtkWidget *         planner_window_new                     (PlannerApplication *app);
gboolean            planner_window_open                    (PlannerWindow      *window,
							    const gchar        *uri,
							    gboolean            internal);
gboolean            planner_window_open_in_existing_or_new (PlannerWindow      *window,
							    const gchar        *uri,
							    gboolean            internal);
GtkUIManager *      planner_window_get_ui_manager          (PlannerWindow      *window);
MrpProject *        planner_window_get_project             (PlannerWindow      *window);
PlannerApplication *planner_window_get_application         (PlannerWindow      *window);
void                planner_window_check_version           (PlannerWindow      *window);
void                planner_window_close                   (PlannerWindow      *window);
void                planner_window_show_day_type_dialog    (PlannerWindow      *window);
void                planner_window_show_calendar_dialog    (PlannerWindow      *window);
PlannerCmdManager * planner_window_get_cmd_manager         (PlannerWindow      *window);
void                planner_window_set_status              (PlannerWindow      *window,
							    const gchar        *message);

#endif /* __PLANNER_WINDOW_H__ */
