/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2002 CodeFactory AB
 * Copyright (C) 2001-2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
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

#ifndef __MG_GANTT_ROW_H__
#define __MG_GANTT_ROW_H__

#include <gtk/gtk.h>
#include <libgnomecanvas/gnome-canvas.h>
#include <libgnomecanvas/gnome-canvas-util.h>

#define MG_TYPE_GANTT_ROW            (planner_gantt_row_get_type ())
#define MG_GANTT_ROW(obj)            (GTK_CHECK_CAST ((obj), MG_TYPE_GANTT_ROW, MgGanttRow))
#define MG_GANTT_ROW_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), MG_TYPE_GANTT_ROW, MgGanttRowClass))
#define MG_IS_GANTT_ROW(obj)         (GTK_CHECK_TYPE ((obj), MG_TYPE_GANTT_ROW))
#define MG_IS_GANTT_ROW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), MG_TYPE_GANTT_ROW))
#define MG_GANTT_ROW_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), MG_TYPE_GANTT_ROW, MgGanttRowClass))


typedef struct _MgGanttRow      MgGanttRow;
typedef struct _MgGanttRowClass MgGanttRowClass;
typedef struct _MgGanttRowPriv  MgGanttRowPriv;

struct _MgGanttRow {
	GnomeCanvasItem  parent;
	MgGanttRowPriv  *priv;	
};

struct _MgGanttRowClass {
	GnomeCanvasItemClass parent_class;
};


GType planner_gantt_row_get_type     (void) G_GNUC_CONST;
void  planner_gantt_row_get_geometry (MgGanttRow *row,
				 gdouble    *x1,
				 gdouble    *y1,
				 gdouble    *x2,
				 gdouble    *y2);
void  planner_gantt_row_set_visible  (MgGanttRow *row,
				 gboolean    is_visible);


#endif /* __MG_GANTT_ROW_H__ */

