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
#include <glib/gi18n.h>
#include "planner-print-job.h"
#include "planner-print-dialog.h"
#include "planner-view.h"

#define MARGIN 0

struct _PlannerPrintJobPriv {
        gchar         *header;
        gchar         *footer;

	gdouble        paper_width;
	gdouble        paper_height;

	gdouble        header_height;
	gdouble        footer_height;

	PangoFontDescription     *font;
	gdouble        font_height;

	PangoFontDescription     *bold_font;

	/* The font in use, one of the above. */
	PangoFontDescription     *current_font;

	gint           active_page;
	gint           total_pages;

	GList         *views;
};

static void     print_job_class_init          (PlannerPrintJobClass *klass);
static void     print_job_init                (PlannerPrintJob      *job);
static void     print_job_finalize            (GObject         *object);
static void     print_job_transform           (PlannerPrintJob      *job,
					       gdouble         *x,
					       gdouble         *y);
static void     print_job_update_size         (PlannerPrintJob      *job);


static GObjectClass *parent_class = NULL;

GType
planner_print_job_get_type (void)
{
        static GType type = 0;

        if (!type) {
                static const GTypeInfo info =  {
                        sizeof (PlannerPrintJobClass),
                        NULL,           /* base_init */
                        NULL,           /* base_finalize */
                        (GClassInitFunc) print_job_class_init,
                        NULL,           /* class_finalize */
                        NULL,           /* class_data */
                        sizeof (PlannerPrintJob),
                        0,              /* n_preallocs */
                        (GInstanceInitFunc) print_job_init
                };

                type = g_type_register_static (G_TYPE_OBJECT,
					       "PlannerPrintJob", &info, 0);
        }

        return type;
}

static void
print_job_class_init (PlannerPrintJobClass *klass)
{
        GObjectClass *o_class;

	parent_class = g_type_class_peek_parent (klass);

        o_class = G_OBJECT_CLASS (klass);
        o_class->finalize = print_job_finalize;
}

static void
print_job_init (PlannerPrintJob *job)
{
        PlannerPrintJobPriv   *priv;

        priv = g_new0 (PlannerPrintJobPriv, 1);
	job->priv = priv;
}

static void
print_job_finalize (GObject *object)
{
        PlannerPrintJob     *job = PLANNER_PRINT_JOB (object);
	PlannerPrintJobPriv *priv = job->priv;

	g_object_unref (job->operation);
	pango_font_description_free (priv->font);
	pango_font_description_free (priv->bold_font);

        g_free (job->priv);

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}

static void
print_job_transform (PlannerPrintJob *job, gdouble *x, gdouble *y)
{
	if (x) {
		*x += MARGIN;
	}

	if (y) {
		*y += MARGIN;
	}
}

static void
print_job_update_size (PlannerPrintJob *job)
{
	PlannerPrintJobPriv *priv;

	priv = job->priv;

	job->height = (priv->paper_height -
		       priv->header_height -
		       priv->footer_height -
		       2 * MARGIN);

	job->width = (priv->paper_width -
		      2 * MARGIN);
}

static GObject *
print_job_create_custom_widget(GtkPrintOperation *operation,
			       gpointer           user_data)
{
	PlannerPrintJob *job;
	PlannerPrintJobPriv *priv;

	job = (PlannerPrintJob *)user_data;
	priv = job->priv;

	gtk_print_operation_set_custom_tab_label(operation, _("Select views"));

	return G_OBJECT(print_dialog_create_page (NULL, NULL, priv->views));
}

static void
print_job_custom_widget_apply(GtkPrintOperation *operation,
			      GtkWidget         *widget,
			      gpointer           user_data)
{
	gboolean summary;
	PlannerPrintJob *job;
	PlannerPrintJobPriv *priv;

	job = (PlannerPrintJob *)user_data;
	priv = job->priv;

	priv->views = planner_print_dialog_get_print_selection(widget, &summary);
}

static void
print_job_begin_print(GtkPrintOperation *operation,
		      GtkPrintContext   *context,
		      gpointer           user_data)
{
	PlannerPrintJob *job;
	PlannerPrintJobPriv *priv;
	GList *l;
	PlannerView *view;
	int n_pages = 0;

	job = (PlannerPrintJob *)user_data;
	priv = job->priv;

	job->cr = gtk_print_context_get_cairo_context (context);
	job->pc = context;

	priv->paper_height = gtk_print_context_get_height (context);
	priv->paper_width = gtk_print_context_get_width (context);

	print_job_update_size (job);

	planner_print_job_set_font_regular (job);
	job->x_pad = planner_print_job_get_extents (job, "#") / 2;

	for (l = priv->views; l; l = l->next) {
		view = l->data;
		planner_view_print_init (view, job);

		n_pages += planner_view_print_get_n_pages (view);
	}
	gtk_print_operation_set_n_pages (operation, (n_pages > 0) ? n_pages : 1);
}

static void
print_job_draw_page (GtkPrintOperation *operation,
		     GtkPrintContext   *context,
		     gint               page_nr,
		     gpointer           user_data)
{
	PlannerPrintJob     *job;
	PlannerPrintJobPriv *priv;
	PlannerView         *v;
	GList               *l;
	gboolean             page_found = FALSE;
	gint                 pages_in_view;

	job = (PlannerPrintJob *)user_data;
	priv = job->priv;

	l = priv->views;

	/* Abort if there is nothing to print.
	 *
	 * A better solution would be to set the number of pages to print to 0
	 * in print_job_begin_print, but 0 is not a valid value for
	 * gtk_print_operation_set_n_pages.
	 */
	if(!l) {
		return;
	}

	while (!page_found) {
		v = PLANNER_VIEW (l->data);

		pages_in_view = planner_view_print_get_n_pages (v);

		if (page_nr < pages_in_view) {
			planner_view_print (v, page_nr);
			page_found = TRUE;
		} else {
			page_nr -= pages_in_view;
			l = l->next;

			g_assert (l != NULL);
		}
	}
}

static void
print_job_end_print (GtkPrintOperation *operation,
		     GtkPrintContext   *context,
		     gpointer           user_data)
{
	PlannerPrintJob *job;
	PlannerPrintJobPriv *priv;
	GList *l;

	job = (PlannerPrintJob *)user_data;
	priv = job->priv;

	for (l = priv->views; l; l = l->next) {
		planner_view_print_cleanup (PLANNER_VIEW(l->data));
	}
}


PlannerPrintJob *
planner_print_job_new (GtkPrintOperation *gpo, GList *views)
{
        PlannerPrintJob     *job;
	PlannerPrintJobPriv *priv;

        job = g_object_new (PLANNER_TYPE_PRINT_JOB, NULL);

	priv = job->priv;

	job->operation = g_object_ref (gpo);
	gtk_print_operation_set_unit (gpo, GTK_UNIT_POINTS);

	priv->header = NULL;
	priv->footer = NULL;

	priv->font = pango_font_description_from_string ("Sans Regular 6");
	priv->font_height = pango_font_description_get_size (priv->font) / PANGO_SCALE;

	priv->bold_font = pango_font_description_from_string ("Sans Bold 6");

	priv->header_height = 0;
	priv->footer_height = 0;
	priv->views = views;

	g_signal_connect (G_OBJECT (gpo), "create-custom-widget",
			  G_CALLBACK (print_job_create_custom_widget), job);
	g_signal_connect (G_OBJECT (gpo), "custom-widget-apply",
			  G_CALLBACK (print_job_custom_widget_apply), job);
	g_signal_connect (G_OBJECT (gpo), "begin-print",
			  G_CALLBACK (print_job_begin_print), job);
	g_signal_connect (G_OBJECT (gpo), "draw-page",
			  G_CALLBACK (print_job_draw_page), job);
	g_signal_connect (G_OBJECT (gpo), "end-print",
			  G_CALLBACK (print_job_end_print), job);

        return job;
}

void
planner_print_job_set_header (PlannerPrintJob *job, const gchar *header)
{
        PlannerPrintJobPriv *priv;

        g_return_if_fail (PLANNER_IS_PRINT_JOB (job));

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
planner_print_job_set_footer (PlannerPrintJob *job, const gchar *footer)
{
        PlannerPrintJobPriv *priv;

        g_return_if_fail (PLANNER_IS_PRINT_JOB (job));

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
planner_print_job_set_total_pages (PlannerPrintJob *job, gint total_pages)
{
        g_return_if_fail (PLANNER_IS_PRINT_JOB (job));

        job->priv->total_pages = total_pages;
}

void
planner_print_job_moveto (PlannerPrintJob *job, gdouble x, gdouble y)
{
	g_return_if_fail (PLANNER_IS_PRINT_JOB (job));

	print_job_transform (job, &x, &y);

	cairo_move_to (job->cr, x, y);
}

void
planner_print_job_lineto (PlannerPrintJob *job, gdouble x, gdouble y)
{
	g_return_if_fail (PLANNER_IS_PRINT_JOB (job));

	print_job_transform (job, &x, &y);

	cairo_line_to (job->cr, x, y);
}

void
planner_print_job_text (PlannerPrintJob *job,
			gint x,
			gint y,
			const char      *str)
{
	g_return_if_fail (job != NULL);
	g_return_if_fail (str != NULL);

	PangoLayout *layout = gtk_print_context_create_pango_layout (job->pc);
	PlannerPrintJobPriv *priv;

	priv = job->priv;

	pango_layout_set_font_description (layout, priv->current_font);
	pango_layout_set_text (layout, str, -1);

	pango_layout_context_changed (layout);

	planner_print_job_moveto (job, x, y - priv->font_height);

	pango_cairo_show_layout (job->cr, layout);

	g_object_unref (layout);
}

void
planner_print_job_show_clipped (PlannerPrintJob *job,
				gdouble          x,
				gdouble          y,
				const gchar     *str,
				gdouble          x1,
				gdouble          y1,
				gdouble          x2,
				gdouble          y2)
{
	PlannerPrintJobPriv *priv;
	gint height;

	priv = job->priv;

	x1 = MAX (x1, 0);
	x2 = MIN (x2, job->width);
	y1 = MAX (y1, 0);
	y2 = MIN (y2, job->height);

	/* Don't try to print anything if the text starts outside the clip rect. */
	if (x < x1 || x > x2) {
		return;
	}

	cairo_save (job->cr);

	cairo_set_line_width (job->cr, 1);

	cairo_new_path (job->cr);
	planner_print_job_moveto (job, x1, y1);
	planner_print_job_lineto (job, x1, y2);
	planner_print_job_lineto (job, x2, y2);
	planner_print_job_lineto (job, x2, y1);

	cairo_close_path (job->cr);

	cairo_clip (job->cr);

	PangoLayout *layout = gtk_print_context_create_pango_layout (job->pc);

	pango_layout_set_text (layout, str, strlen(str));
	pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
	pango_layout_set_width (layout, (x2 - x1) * PANGO_SCALE);

	pango_layout_set_font_description (layout, priv->current_font);
	pango_layout_context_changed (layout);

	pango_layout_get_pixel_size (layout, NULL, &height);

	planner_print_job_moveto (job, x, y - priv->font_height);

	pango_cairo_show_layout (job->cr, layout);

	g_object_unref (layout);

	cairo_restore (job->cr);
}

gboolean
planner_print_job_begin_next_page (PlannerPrintJob *job)
{
	PlannerPrintJobPriv *priv;

	g_return_val_if_fail (PLANNER_IS_PRINT_JOB (job), FALSE);

	priv = job->priv;

	if ((priv->active_page + 1) > priv->total_pages) {
		return FALSE;
	}

	priv->active_page++;

	/* Draw header and footer, FIXME: implement. */
/*
	cairo_new_path (job->cr);

	planner_print_job_set_font_regular (job);

	cairo_set_line_width (job->cr, THIN_LINE_WIDTH);

	planner_print_job_moveto (job, 0, 0);
	planner_print_job_lineto (job, job->width, 0);
	planner_print_job_lineto (job, job->width, job->height);
	planner_print_job_lineto (job, 0, job->height);
	cairo_close_path (job->cr);

#if 0
	cairo_save (job->cr);
	cairo_set_line_width (job->cr, 3);
	cairo_stroke (job->cr);
	cairo_restore (job->cr);
#endif
	cairo_clip (job->cr);
	*/
	return TRUE;
}

void
planner_print_job_finish_page (PlannerPrintJob *job, gboolean draw_border)
{
	g_return_if_fail (PLANNER_IS_PRINT_JOB (job));

	if (draw_border) {
		cairo_new_path (job->cr);

		cairo_set_line_width (job->cr, THIN_LINE_WIDTH);

		planner_print_job_moveto (job, 0, 0);
		planner_print_job_lineto (job, job->width, 0);
		planner_print_job_lineto (job, job->width, job->height);
		planner_print_job_lineto (job, 0, job->height);
		planner_print_job_lineto (job, 0, 0);
		cairo_close_path (job->cr);

		cairo_stroke (job->cr);
	}
}

PangoFontDescription *
planner_print_job_get_font (PlannerPrintJob *job)
{
	g_return_val_if_fail (PLANNER_IS_PRINT_JOB (job), NULL);

	return job->priv->font;
}

gdouble
planner_print_job_get_font_height (PlannerPrintJob *job)
{
	g_return_val_if_fail (PLANNER_IS_PRINT_JOB (job), 0);

	return job->priv->font_height;
}

void
planner_print_job_set_font_regular (PlannerPrintJob *job)
{
	PlannerPrintJobPriv *priv;

	g_return_if_fail (PLANNER_IS_PRINT_JOB (job));

	priv = job->priv;

	priv->current_font = priv->font;
}

void
planner_print_job_set_font_bold (PlannerPrintJob *job)
{
	PlannerPrintJobPriv *priv;

	g_return_if_fail (PLANNER_IS_PRINT_JOB (job));

	priv = job->priv;

	priv->current_font = priv->bold_font;
}

void
planner_print_job_set_font_italic (PlannerPrintJob *job)
{
	PlannerPrintJobPriv *priv;

	g_return_if_fail (PLANNER_IS_PRINT_JOB (job));

	priv = job->priv;

	/* FIXME: use italic. */

	priv->current_font = priv->bold_font;
}

gdouble
planner_print_job_get_extents (PlannerPrintJob *job, char *text)
{
	PangoLayout *layout = gtk_print_context_create_pango_layout (job->pc);
	PlannerPrintJobPriv *priv;
	PangoRectangle ink;

	priv = job->priv;

	pango_layout_set_font_description (layout, priv->current_font);
	pango_layout_set_text (layout, text, -1);

        pango_cairo_update_layout (gtk_print_context_get_cairo_context(job->pc), layout);
	pango_layout_context_changed (layout);

	pango_layout_get_extents (layout, &ink, NULL);

	g_object_unref (layout);

	return ((gdouble)ink.width / PANGO_SCALE);
}

