/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003 Benjamin BAYART <benjamin@sitadelle.com>
 * Copyright (C) 2003 Xavier Ordoquy <xordoquy@wanadoo.fr>
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

#ifndef __PLANNER_TTABLE_ROW_H__
#define __PLANNER_TTABLE_ROW_H__

#include <gtk/gtk.h>
#include <libgnomecanvas/gnome-canvas.h>

#define PLANNER_TYPE_TTABLE_ROW            (planner_ttable_row_get_type ())
#define PLANNER_TTABLE_ROW(obj)            (GTK_CHECK_CAST ((obj), PLANNER_TYPE_TTABLE_ROW, PlannerTtableRow))
#define PLANNER_TTABLE_ROW_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_TTABLE_ROW, PlannerTtableRowClass))
#define PLANNER_IS_TTABLE_ROW(obj)         (GTK_CHECK_TYPE ((obj), PLANNER_TYPE_TTABLE_ROW))
#define PLANNER_IS_TTABLE_ROW_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), PLANNER_TYPE_TTABLE_ROW))
#define PLANNER_TTABLE_ROW_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), PLANNER_TYPE_TTABLE_ROW, PlannerTtableRowClass))


typedef struct _PlannerTtableRow PlannerTtableRow;
typedef struct _PlannerTtableRowClass PlannerTtableRowClass;
typedef struct _PlannerTtableRowPriv PlannerTtableRowPriv;

struct _PlannerTtableRow {
        GnomeCanvasItem       parent;
        PlannerTtableRowPriv *priv;
};

struct _PlannerTtableRowClass {
        GnomeCanvasItemClass parent_class;
};

GType planner_ttable_row_get_type     (void) G_GNUC_CONST;
void  planner_ttable_row_get_geometry (PlannerTtableRow *row,
				       gdouble          *x1,
				       gdouble          *y1,
				       gdouble          *x2,
				       gdouble          *y2);
void  planner_ttable_row_set_visible  (PlannerTtableRow *row,
				       gboolean          is_visible);

#endif /* __PLANNER_TTABLE_ROW_H__ */
 
