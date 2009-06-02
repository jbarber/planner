/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2003 CodeFactory AB
 * Copyright (C) 2001-2003 Richard Hult <richard@imendio.com>
 * Copyright (C) 2001-2003 Mikael Hallendal <micke@imendio.com>
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

#ifndef __PLANNER_PRINT_JOB_H__
#define __PLANNER_PRINT_JOB_H__

#include <gtk/gtk.h>

#define PLANNER_TYPE_PRINT_JOB                (planner_print_job_get_type ())
#define PLANNER_PRINT_JOB(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLANNER_TYPE_PRINT_JOB, PlannerPrintJob))
#define PLANNER_PRINT_JOB_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_PRINT_JOB, PlannerPrintJobClass))
#define PLANNER_IS_PRINT_JOB(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLANNER_TYPE_PRINT_JOB))
#define PLANNER_IS_PRINT_JOB_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), PLANNER_TYPE_PRINT_JOB))
#define PLANNER_PRINT_JOB_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), PLANNER_TYPE_PRINT_JOB, PlannerPrintJobClass))

/* See http://lists.freedesktop.org/archives/cairo/2006-January/005979.html */
#define THIN_LINE_WIDTH	(72.0 / 300.0)

typedef struct _PlannerPrintJob        PlannerPrintJob;
typedef struct _PlannerPrintJobClass   PlannerPrintJobClass;
typedef struct _PlannerPrintJobPriv    PlannerPrintJobPriv;

struct _PlannerPrintJob {
	GObject            parent;
	GtkPrintOperation *operation;
	GtkPrintContext   *pc;
	cairo_t           *cr;
	PangoLayout       *layout;

	/* Printable area */
	gdouble            width;
	gdouble            height;

	/* Text padding to use when printing text next to a line. */
	gdouble            x_pad;

	PlannerPrintJobPriv    *priv;
};

struct _PlannerPrintJobClass {
	GObjectClass parent_class;
};

GType        planner_print_job_get_type         (void) G_GNUC_CONST;
PlannerPrintJob * planner_print_job_new         (GtkPrintOperation *gpo,
					    GList *views);
void         planner_print_job_set_header       (PlannerPrintJob    *job,
					    const gchar   *header);
void         planner_print_job_set_footer       (PlannerPrintJob    *job,
					    const gchar   *footer);
void         planner_print_job_set_total_pages  (PlannerPrintJob    *job,
					    gint           total_pages);
PangoFontDescription *  planner_print_job_get_font (PlannerPrintJob    *job);
gdouble      planner_print_job_get_font_height  (PlannerPrintJob    *job);

/* These functions use 0,0 as the top left of printable area. Use these
 * functions when drawing, as they recalculates the coordinates and calls
 * gnome_print_lineto/moveto with absolute coordinates.
 */
void         planner_print_job_moveto           (PlannerPrintJob    *job,
					    gdouble        x,
					    gdouble        y);
void         planner_print_job_lineto           (PlannerPrintJob    *job,
					    gdouble        x,
					    gdouble        y);
void         planner_print_job_text             (PlannerPrintJob *job,
						 gint x,
						 gint y,
						 const char      *str);

void         planner_print_job_show_clipped     (PlannerPrintJob    *job,
					    gdouble        x,
					    gdouble        y,
					    const gchar   *str,
					    gdouble        x1,
					    gdouble        y1,
					    gdouble        x2,
					    gdouble        y2);
gboolean     planner_print_job_begin_next_page  (PlannerPrintJob    *job);
void         planner_print_job_finish_page      (PlannerPrintJob    *job,
					    gboolean       draw_border);
void         planner_print_job_set_font_regular (PlannerPrintJob    *job);
void         planner_print_job_set_font_bold    (PlannerPrintJob    *job);
void         planner_print_job_set_font_italic  (PlannerPrintJob    *job);
gdouble      planner_print_job_get_extents      (PlannerPrintJob    *job,
					    char *text);




#endif /* __PLANNER_PRINT_JOB_H__ */

