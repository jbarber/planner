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

#ifndef __MG_MAIN_WINDOW_H__
#define __MG_MAIN_WINDOW_H__

#include <bonobo/bonobo-window.h>
#include <bonobo/bonobo-ui-component.h>
#include <libplanner/mrp-project.h>
#include "planner-application.h"

#define MG_TYPE_MAIN_WINDOW                (planner_main_window_get_type ())
#define MG_MAIN_WINDOW(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), MG_TYPE_MAIN_WINDOW, MgMainWindow))
#define MG_MAIN_WINDOW_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), MG_TYPE_MAIN_WINDOW, MgMainWindowClass))
#define MG_IS_MAIN_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MG_TYPE_MAIN_WINDOW))
#define MG_IS_MAIN_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), MG_TYPE_MAIN_WINDOW))
#define MG_MAIN_WINDOW_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), MG_TYPE_MAIN_WINDOW, MgMainWindowClass))

typedef struct _MgMainWindow        MgMainWindow;
typedef struct _MgMainWindowClass   MgMainWindowClass;
typedef struct _MgMainWindowPriv    MgMainWindowPriv;

struct _MgMainWindow
{
        BonoboWindow       parent;
        
        MgMainWindowPriv  *priv;
};

struct _MgMainWindowClass
{
        BonoboWindowClass  parent_class;
};

GType                planner_main_window_get_type             (void) G_GNUC_CONST;

GtkWidget         *  planner_main_window_new                  (MgApplication *app);
gboolean             planner_main_window_open                 (MgMainWindow  *window,
							  const gchar   *uri);
BonoboUIContainer *  planner_main_window_get_ui_container     (MgMainWindow  *window);
MrpProject        *  planner_main_window_get_project          (MgMainWindow  *window);
MgApplication     *  planner_main_window_get_application      (MgMainWindow  *window);
void                 planner_main_window_check_version        (MgMainWindow  *window);

void                 planner_main_window_close                (MgMainWindow  *window);

void                 planner_main_window_show_day_type_dialog (MgMainWindow  *window);

void                 planner_main_window_show_calendar_dialog (MgMainWindow  *window);


#endif /* __MG_MAIN_WINDOW_H__ */
