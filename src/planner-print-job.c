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

#include <config.h>
#include <string.h>
#include <glib-object.h>
#include <libgnomeprint/gnome-print-config.h>
#include <libgnomeprint/gnome-print-job.h>
#include <libart_lgpl/libart.h>
#include "planner-print-job.h"

#define MARGIN 30

struct _MgPrintJobPriv {
        gchar         *header;
        gchar         *footer;

	gdouble        paper_width;
	gdouble        paper_height;

	gdouble        header_height;
	gdouble        footer_height;

	GnomeFont     *font;
	gdouble        font_height;

	GnomeFont     *bold_font;

	/* The font in use, one of the above. */
	GnomeFont     *current_font;

	gint           active_page;
	gint           total_pages;

	gboolean       upside_down;
};

static void     print_job_class_init          (MgPrintJobClass *klass);
static void     print_job_init                (MgPrintJob      *job);
static void     print_job_finalize            (GObject         *object);
static void     print_job_transform           (MgPrintJob      *job,
					       gdouble         *x,
					       gdouble         *y);
static void     print_job_update_size         (MgPrintJob      *job);


static GObjectClass *parent_class = NULL;

GType
planner_print_job_get_type (void)
{
        static GType type = 0;

        if (!type) {
                static const GTypeInfo info =  {
                        sizeof (MgPrintJobClass),
                        NULL,           /* base_init */
                        NULL,           /* base_finalize */
                        (GClassInitFunc) print_job_class_init,
                        NULL,           /* class_finalize */
                        NULL,           /* class_data */
                        sizeof (MgPrintJob),
                        0,              /* n_preallocs */
                        (GInstanceInitFunc) print_job_init
                };
                
                type = g_type_register_static (G_TYPE_OBJECT,
					       "MgPrintJob", &info, 0);
        }

        return type;
}

static void 
print_job_class_init (MgPrintJobClass *klass)
{
        GObjectClass *o_class;

	parent_class = g_type_class_peek_parent (klass);

        o_class = G_OBJECT_CLASS (klass);
        o_class->finalize = print_job_finalize;
}

static void 
print_job_init (MgPrintJob *job)
{
        MgPrintJobPriv   *priv;

        priv = g_new0 (MgPrintJobPriv, 1);
	job->priv = priv;
}

static void
print_job_finalize (GObject *object)
{
        MgPrintJob     *job = MG_PRINT_JOB (object);
	MgPrintJobPriv *priv = job->priv;

	g_object_unref (job->pj);
	gnome_print_context_close (job->pc);
	g_object_unref (job->pc);
	g_object_unref (priv->font);

        g_free (job->priv);

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}

static void
print_job_transform (MgPrintJob *job, gdouble *x, gdouble *y)
{
	MgPrintJobPriv *priv;
	
	priv = job->priv;
	
	if (x) {
		*x += MARGIN;
	}

	if (y) {
		*y = job->height - *y + MARGIN;
	}
}

static void
print_job_update_size (MgPrintJob *job)
{
	MgPrintJobPriv *priv;

	priv = job->priv;
	
	job->height = (priv->paper_height -
		       priv->header_height -
		       priv->footer_height -
		       2 * MARGIN);

	job->width = (priv->paper_width -
		      2 * MARGIN);
}

MgPrintJob *
planner_print_job_new (GnomePrintJob *gpj)
{
        MgPrintJob       *job;
	MgPrintJobPriv   *priv;
	GnomePrintConfig *config;
	gchar            *orientation;
        
        job = g_object_new (MG_TYPE_PRINT_JOB, NULL);
	
	priv = job->priv;

	job->pj = gpj;
	job->pc = gnome_print_job_get_context (job->pj);

        config = gnome_print_job_get_config (job->pj);

	/* Useful for testing, so we leave if here. */
#if 0
	if (!gnome_print_config_set (config, "Settings.Transport.Backend", "file")) {
		g_warning ("Could not set the backend to file.");
	}
#endif
	
	gnome_print_config_get_length (config, 
				       GNOME_PRINT_KEY_PAPER_WIDTH,
				       &priv->paper_width,
				       NULL);
	gnome_print_config_get_length (config,
				       GNOME_PRINT_KEY_PAPER_HEIGHT,
				       &priv->paper_height,
				       NULL);
	
	orientation= gnome_print_config_get (config,
					     GNOME_PRINT_KEY_PAGE_ORIENTATION);

	if (!strcmp (orientation, "R90") || !strcmp (orientation, "R270")) {
		gdouble tmp;

		tmp = priv->paper_width;
		priv->paper_width = priv->paper_height;
		priv->paper_height = tmp;
	}

	if (!strcmp (orientation, "R270") || !strcmp (orientation, "R180")) {
		priv->upside_down = TRUE;
	}
	
	g_free (orientation);
	
	priv->header = NULL;
	priv->footer = NULL;
	
	priv->font = gnome_font_find_closest ("Sans Regular", 6.0);
	priv->font_height = (gnome_font_get_ascender (priv->font) + 
			     gnome_font_get_descender (priv->font));

	priv->bold_font = gnome_font_find_closest ("Sans Bold", 6.0);

	priv->header_height = 0;
	priv->footer_height = 0;

	print_job_update_size (job);

	job->x_pad = gnome_font_get_width_utf8 (priv->font, "#") / 2;
        
        return job;
}

void
planner_print_job_set_header (MgPrintJob *job, const gchar *header)
{
        MgPrintJobPriv *priv;
        
        g_return_if_fail (MG_IS_PRINT_JOB (job));

        priv = job->priv;

	g_free (priv->header);
	priv->header = NULL;
	
        if (header) {
                priv->header = g_strdup (header);
		priv->header_height = priv->font_height * 2;
        } else {
		priv->header_height = 0;
	}

	print_job_update_size (job);
}

void
planner_print_job_set_footer (MgPrintJob *job, const gchar *footer)
{
        MgPrintJobPriv *priv;
        
        g_return_if_fail (MG_IS_PRINT_JOB (job));

        priv = job->priv;

	g_free (priv->footer);
	priv->footer = NULL;
	
        if (footer) {
                priv->footer = g_strdup (footer);
		priv->footer_height = priv->font_height * 2;
        } else {
		priv->footer_height = 0;
	}

	print_job_update_size (job);
}

void
planner_print_job_set_total_pages (MgPrintJob *job, gint total_pages)
{
        g_return_if_fail (MG_IS_PRINT_JOB (job));
        
        job->priv->total_pages = total_pages;
}

void
planner_print_job_moveto (MgPrintJob *job, gdouble x, gdouble y)
{
	g_return_if_fail (MG_IS_PRINT_JOB (job));

	print_job_transform (job, &x, &y);
	
	gnome_print_moveto (job->pc, x, y);
}

void
planner_print_job_lineto (MgPrintJob *job, gdouble x, gdouble y)
{
	g_return_if_fail (MG_IS_PRINT_JOB (job));

	print_job_transform (job, &x, &y);

	gnome_print_lineto (job->pc, x, y);
}

void
planner_print_job_show_clipped (MgPrintJob  *job,
			   gdouble      x,
			   gdouble      y,
			   const gchar *str,
			   gdouble      x1,
			   gdouble      y1,
			   gdouble      x2,
			   gdouble      y2)
{
	MgPrintJobPriv *priv;
	gdouble         width;
	gdouble         ellipsis_width;
	gchar          *tmp, *ellipsized;
	gchar          *p;
	glong           len;

	/* FIXME: The clipping doesn't work in the preview, but works when
	 * printing for real. Bug in libgnomeprint.
	 */

	priv = job->priv;
	
	x1 = MAX (x1, 0);
	x2 = MIN (x2, job->width);
	y1 = MAX (y1, 0);
	y2 = MIN (y2, job->height);

	/* Don't try to print anything if the text starts outside the clip
	 * rect.
	 */
	if (x < x1 || x > x2) {
		return;
	}
	
	width = gnome_font_get_width_utf8 (priv->current_font, str);

	gnome_print_gsave (job->pc);

        gnome_print_newpath (job->pc);
	planner_print_job_moveto (job, x1, y1);
	planner_print_job_lineto (job, x1, y2);
	planner_print_job_lineto (job, x2, y2);
	planner_print_job_lineto (job, x2, y1);
	gnome_print_closepath (job->pc);
	gnome_print_clip (job->pc);

	/* First, see if we can fit the text without ellipsizing. */
	if (x + width <= x2) {
		planner_print_job_moveto (job, x, y);
		gnome_print_show (job->pc, str);
		gnome_print_grestore (job->pc);
		return;
	}
	
	ellipsis_width = gnome_font_get_width_utf8 (priv->current_font, "...");

	tmp = g_strdup (str);
	len = g_utf8_strlen (tmp, -1);

	do {
		p = g_utf8_offset_to_pointer (tmp, len);
		*p = 0;

		width = gnome_font_get_width_utf8 (priv->current_font, tmp) + ellipsis_width;
		
		if (x + width <= x2) {
			ellipsized = g_strconcat (tmp, "...", NULL);
			
			planner_print_job_moveto (job, x, y);
			gnome_print_show (job->pc, ellipsized);
			gnome_print_grestore (job->pc);
			
			g_free (tmp);
			g_free (ellipsized);
			
			return;
		}
		
		len--;
	} while (len);

	g_free (tmp);
	
	gnome_print_grestore (job->pc);
}

gboolean
planner_print_job_begin_next_page (MgPrintJob *job)
{
	MgPrintJobPriv *priv;
	gchar          *job_name;
	
	g_return_val_if_fail (MG_IS_PRINT_JOB (job), FALSE);
	
	priv = job->priv;

	if ((priv->active_page + 1) > priv->total_pages) {
		return FALSE;
	}

	priv->active_page++;

	job_name = g_strdup_printf ("%d", priv->active_page);
	gnome_print_beginpage (job->pc, job_name);
	g_free (job_name);

	if (priv->upside_down) {
		gdouble affine[6];
		
		art_affine_rotate (affine, 180.0);
		gnome_print_concat (job->pc, affine);

		art_affine_translate (affine,
				      -job->width - 2 * MARGIN,
				      -job->height - 2 * MARGIN);
		gnome_print_concat (job->pc, affine);
	}
	
	/* Draw header and footer, FIXME: implement. */

	gnome_print_newpath (job->pc);

	planner_print_job_set_font_regular (job);

	gnome_print_setlinewidth (job->pc, 0);

	planner_print_job_moveto (job, 0, 0);
	planner_print_job_lineto (job, job->width, 0);
	planner_print_job_lineto (job, job->width, job->height);
	planner_print_job_lineto (job, 0, job->height);
	gnome_print_closepath (job->pc);
	gnome_print_clip (job->pc);

	gnome_print_newpath (job->pc);

	return TRUE;
}

void
planner_print_job_finish_page (MgPrintJob *job, gboolean draw_border)
{
	g_return_if_fail (MG_IS_PRINT_JOB (job));

	if (draw_border) {
		gnome_print_setlinewidth (job->pc, 0);
		
		planner_print_job_moveto (job, 0, 0);
		planner_print_job_lineto (job, job->width, 0);
		planner_print_job_lineto (job, job->width, job->height);
		planner_print_job_lineto (job, 0, job->height);
		gnome_print_closepath (job->pc);
		gnome_print_stroke (job->pc);
	}
	
	gnome_print_showpage (job->pc);
}

GnomeFont *
planner_print_job_get_font (MgPrintJob *job)
{
	g_return_val_if_fail (MG_IS_PRINT_JOB (job), NULL);
	
	return job->priv->font;
}

gdouble
planner_print_job_get_font_height (MgPrintJob *job)
{
	g_return_val_if_fail (MG_IS_PRINT_JOB (job), 0);
	
	return job->priv->font_height;
}

void
planner_print_job_set_font_regular (MgPrintJob *job)
{
	MgPrintJobPriv *priv;
	
	g_return_if_fail (MG_IS_PRINT_JOB (job));

	priv = job->priv;
	
	priv->current_font = priv->font;
	gnome_print_setfont (job->pc, priv->font);
}

void
planner_print_job_set_font_bold (MgPrintJob *job)
{
	MgPrintJobPriv *priv;
	
	g_return_if_fail (MG_IS_PRINT_JOB (job));
	
	priv = job->priv;
	
	priv->current_font = priv->bold_font;
	gnome_print_setfont (job->pc, priv->bold_font);
}

void
planner_print_job_set_font_italic (MgPrintJob *job)
{
	MgPrintJobPriv *priv;
	
	g_return_if_fail (MG_IS_PRINT_JOB (job));
	
	priv = job->priv;

	/* FIXME: use italic. */
	
	priv->current_font = priv->bold_font;
	gnome_print_setfont (job->pc, priv->bold_font);
}
