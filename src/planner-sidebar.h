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

#ifndef __PLANNER_SIDEBAR_H__
#define __PLANNER_SIDEBAR_H__

#include <gtk/gtkframe.h>

#define PLANNER_TYPE_SIDEBAR		(planner_sidebar_get_type ())
#define PLANNER_SIDEBAR(obj)		(GTK_CHECK_CAST ((obj), PLANNER_TYPE_SIDEBAR, PlannerSidebar))
#define PLANNER_SIDEBAR_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_SIDEBAR, PlannerSidebarClass))
#define PLANNER_IS_SIDEBAR(obj)		(GTK_CHECK_TYPE ((obj), PLANNER_TYPE_SIDEBAR))
#define PLANNER_IS_SIDEBAR_CLASS(klass)	(GTK_CHECK_CLASS_TYPE ((obj), PLANNER_TYPE_SIDEBAR))
#define PLANNER_SIDEBAR_GET_CLASS(obj)	(GTK_CHECK_GET_CLASS ((obj), PLANNER_TYPE_SIDEBAR, PlannerSidebarClass))

typedef struct _PlannerSidebar           PlannerSidebar;
typedef struct _PlannerSidebarClass      PlannerSidebarClass;
typedef struct _PlannerSidebarPriv       PlannerSidebarPriv;

struct _PlannerSidebar
{
	GtkFrame       parent;
	PlannerSidebarPriv *priv;
};

struct _PlannerSidebarClass
{
	GtkFrameClass  parent_class;
};


GtkType    planner_sidebar_get_type     (void) G_GNUC_CONST;

GtkWidget *planner_sidebar_new          (void);

void       planner_sidebar_append       (PlannerSidebar   *sidebar,
				    const gchar *icon_filename,
				    const gchar *text);

void       planner_sidebar_set_active   (PlannerSidebar   *sidebar,
				    gint         index);


#endif /* __PLANNER_SIDEBAR_H__ */

