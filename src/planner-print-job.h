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

#ifndef __MG_PRINT_JOB_H__
#define __MG_PRINT_JOB_H__

#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-job.h>

#define MG_TYPE_PRINT_JOB                (planner_print_job_get_type ())
#define MG_PRINT_JOB(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), MG_TYPE_PRINT_JOB, MgPrintJob))
#define MG_PRINT_JOB_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), MG_TYPE_PRINT_JOB, MgPrintJobClass))
#define MG_IS_PRINT_JOB(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MG_TYPE_PRINT_JOB))
#define MG_IS_PRINT_JOB_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), MG_TYPE_PRINT_JOB))
#define MG_PRINT_JOB_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), MG_TYPE_PRINT_JOB, MgPrintJobClass))

typedef struct _MgPrintJob        MgPrintJob;
typedef struct _MgPrintJobClass   MgPrintJobClass;
typedef struct _MgPrintJobPriv    MgPrintJobPriv;

struct _MgPrintJob {
	GObject            parent;
        GnomePrintContext *pc;
	GnomePrintJob     *pj;

	/* Printable area */
	gdouble            width;
	gdouble            height;

	/* Text padding to use when printing text next to a line. */
	gdouble            x_pad;

	MgPrintJobPriv    *priv;
};

struct _MgPrintJobClass {
	GObjectClass parent_class;
};

GType        planner_print_job_get_type         (void) G_GNUC_CONST;
MgPrintJob * planner_print_job_new              (GnomePrintJob *gpj);
void         planner_print_job_set_header       (MgPrintJob    *job,
					    const gchar   *header);
void         planner_print_job_set_footer       (MgPrintJob    *job,
					    const gchar   *footer);
void         planner_print_job_set_total_pages  (MgPrintJob    *job,
					    gint           total_pages);
GnomeFont *  planner_print_job_get_font         (MgPrintJob    *job);
gdouble      planner_print_job_get_font_height  (MgPrintJob    *job);

/* These functions use 0,0 as the top left of printable area. Use these
 * functions when drawing, as they recalculates the coordinates and calls
 * gnome_print_lineto/moveto with absolute coordinates.
 */
void         planner_print_job_moveto           (MgPrintJob    *job,
					    gdouble        x,
					    gdouble        y);
void         planner_print_job_lineto           (MgPrintJob    *job,
					    gdouble        x,
					    gdouble        y);
void         planner_print_job_show_clipped     (MgPrintJob    *job,
					    gdouble        x,
					    gdouble        y,
					    const gchar   *str,
					    gdouble        x1,
					    gdouble        y1,
					    gdouble        x2,
					    gdouble        y2);
gboolean     planner_print_job_begin_next_page  (MgPrintJob    *job);
void         planner_print_job_finish_page      (MgPrintJob    *job,
					    gboolean       draw_border);
void         planner_print_job_set_font_regular (MgPrintJob    *job);
void         planner_print_job_set_font_bold    (MgPrintJob    *job);
void         planner_print_job_set_font_italic  (MgPrintJob    *job);





#endif /* __MG_PRINT_JOB_H__ */

