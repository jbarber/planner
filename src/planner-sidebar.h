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

#ifndef __MG_SIDEBAR_H__
#define __MG_SIDEBAR_H__

#include <gtk/gtkframe.h>

#define MG_TYPE_SIDEBAR		(planner_sidebar_get_type ())
#define MG_SIDEBAR(obj)		(GTK_CHECK_CAST ((obj), MG_TYPE_SIDEBAR, MgSidebar))
#define MG_SIDEBAR_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), MG_TYPE_SIDEBAR, MgSidebarClass))
#define MG_IS_SIDEBAR(obj)		(GTK_CHECK_TYPE ((obj), MG_TYPE_SIDEBAR))
#define MG_IS_SIDEBAR_CLASS(klass)	(GTK_CHECK_CLASS_TYPE ((obj), MG_TYPE_SIDEBAR))
#define MG_SIDEBAR_GET_CLASS(obj)	(GTK_CHECK_GET_CLASS ((obj), MG_TYPE_SIDEBAR, MgSidebarClass))

typedef struct _MgSidebar           MgSidebar;
typedef struct _MgSidebarClass      MgSidebarClass;
typedef struct _MgSidebarPriv       MgSidebarPriv;

struct _MgSidebar
{
	GtkFrame       parent;
	MgSidebarPriv *priv;
};

struct _MgSidebarClass
{
	GtkFrameClass  parent_class;
};


GtkType    planner_sidebar_get_type     (void);

GtkWidget *planner_sidebar_new          (void);

void       planner_sidebar_append       (MgSidebar   *sidebar,
				    const gchar *icon_filename,
				    const gchar *text);

void       planner_sidebar_set_active   (MgSidebar   *sidebar,
				    gint         index);


#endif /* __MG_SIDEBAR_H__ */

