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

#ifndef __MG_GANTT_HEADER_H__
#define __MG_GANTT_HEADER_H__

#include <gtk/gtkwidget.h>

#define MG_TYPE_GANTT_HEADER		(planner_gantt_header_get_type ())
#define MG_GANTT_HEADER(obj)		(GTK_CHECK_CAST ((obj), MG_TYPE_GANTT_HEADER, MgGanttHeader))
#define MG_GANTT_HEADER_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), MG_TYPE_GANTT_HEADER, MgGanttHeaderClass))
#define MG_IS_GANTT_HEADER(obj)		(GTK_CHECK_TYPE ((obj), MG_TYPE_GANTT_HEADER))
#define MG_IS_GANTT_HEADER_CLASS(klass)	(GTK_CHECK_CLASS_TYPE ((obj), MG_TYPE_GANTT_HEADER))
#define MG_GANTT_HEADER_GET_CLASS(obj)	(GTK_CHECK_GET_CLASS ((obj), MG_TYPE_GANTT_HEADER, MgGanttHeaderClass))

typedef struct _MgGanttHeader           MgGanttHeader;
typedef struct _MgGanttHeaderClass      MgGanttHeaderClass;
typedef struct _MgGanttHeaderPriv       MgGanttHeaderPriv;

struct _MgGanttHeader
{
	GtkWidget          parent;
	MgGanttHeaderPriv *priv;
};

struct _MgGanttHeaderClass
{
	GtkWidgetClass parent_class;

	void  (* set_scroll_adjustments) (MgGanttHeader  *header,
					  GtkAdjustment *hadjustment,
					  GtkAdjustment *vadjustment);
};


GtkType                planner_gantt_header_get_type        (void);
GtkWidget             *planner_gantt_header_new             (void);


#endif /* __MG_GANTT_HEADER_H__ */

