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

#include <config.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libplanner/mrp-time.h>
#include "planner-marshal.h"
#include "planner-gantt-header.h"
#include "planner-gantt-model.h"
#include "planner-scale-utils.h"


struct _PlannerGanttHeaderPriv {
	GdkWindow          *bin_window;

	GtkAdjustment      *hadjustment;

	PangoLayout        *layout;

	MrpTimeUnit    major_unit;
	PlannerScaleFormat  major_format;

	MrpTimeUnit    minor_unit;
	PlannerScaleFormat  minor_format;

	gdouble             hscale;

	gint                width;
	gint                height;

	gdouble             x1;
	gdouble             x2;

	gchar              *date_hint;
};

/* Properties */
enum {
	PROP_0,
	PROP_HEIGHT,
	PROP_X1,
	PROP_X2,
	PROP_SCALE,
	PROP_ZOOM
};

enum {
	DATE_HINT_CHANGED,
	LAST_SIGNAL
};

static void     gantt_header_class_init          (PlannerGanttHeaderClass *klass);
static void     gantt_header_init                (PlannerGanttHeader      *header);
static void     gantt_header_finalize            (GObject                 *object);
static void     gantt_header_set_property        (GObject                 *object,
						  guint                    prop_id,
						  const GValue            *value,
						  GParamSpec              *pspec);
static void     gantt_header_get_property        (GObject                 *object,
						  guint                    prop_id,
						  GValue                  *value,
						  GParamSpec              *pspec);
static void     gantt_header_destroy             (GtkObject               *object);
static void     gantt_header_map                 (GtkWidget               *widget);
static void     gantt_header_realize             (GtkWidget               *widget);
static void     gantt_header_unrealize           (GtkWidget               *widget);
static void     gantt_header_size_allocate       (GtkWidget               *widget,
						  GtkAllocation           *allocation);
static gboolean gantt_header_expose_event        (GtkWidget               *widget,
						  GdkEventExpose          *event);
static gboolean gantt_header_motion_notify_event (GtkWidget               *widget,
						  GdkEventMotion          *event);
static gboolean gantt_header_leave_notify_event  (GtkWidget	          *widget,
						  GdkEventCrossing        *event);
static void     gantt_header_set_adjustments     (PlannerGanttHeader      *header,
						  GtkAdjustment           *hadj,
						  GtkAdjustment           *vadj);
static void     gantt_header_adjustment_changed  (GtkAdjustment           *adjustment,
						  PlannerGanttHeader      *header);



static GtkWidgetClass *parent_class = NULL;
static guint           signals[LAST_SIGNAL];


GtkType
planner_gantt_header_get_type (void)
{
	static GtkType planner_gantt_header_type = 0;

	if (!planner_gantt_header_type) {
		static const GTypeInfo planner_gantt_header_info = {
			sizeof (PlannerGanttHeaderClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) gantt_header_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (PlannerGanttHeader),
			0,              /* n_preallocs */
			(GInstanceInitFunc) gantt_header_init
		};

		planner_gantt_header_type = g_type_register_static (
			GTK_TYPE_WIDGET,
			"PlannerGanttHeader",
			&planner_gantt_header_info,
			0);
	}

	return planner_gantt_header_type;
}

static void
gantt_header_class_init (PlannerGanttHeaderClass *class)
{
	GObjectClass      *o_class;
	GtkObjectClass    *object_class;
	GtkWidgetClass    *widget_class;

	parent_class = g_type_class_peek_parent (class);

	o_class = (GObjectClass *) class;
	object_class = (GtkObjectClass *) class;
	widget_class = (GtkWidgetClass *) class;

	/* GObject methods. */
	o_class->set_property = gantt_header_set_property;
	o_class->get_property = gantt_header_get_property;
	o_class->finalize = gantt_header_finalize;

	/* GtkObject methods. */
	object_class->destroy = gantt_header_destroy;

	/* GtkWidget methods. */
	widget_class->map = gantt_header_map;;
	widget_class->realize = gantt_header_realize;
	widget_class->unrealize = gantt_header_unrealize;
	widget_class->size_allocate = gantt_header_size_allocate;
	widget_class->expose_event = gantt_header_expose_event;
	widget_class->leave_notify_event = gantt_header_leave_notify_event;

	widget_class->motion_notify_event = gantt_header_motion_notify_event;

	class->set_scroll_adjustments = gantt_header_set_adjustments;

	widget_class->set_scroll_adjustments_signal =
		g_signal_new ("set_scroll_adjustments",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (PlannerGanttHeaderClass, set_scroll_adjustments),
			      NULL, NULL,
			      planner_marshal_VOID__OBJECT_OBJECT,
			      G_TYPE_NONE, 2,
			      GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);

	/* Properties. */
	g_object_class_install_property (
		o_class,
		PROP_HEIGHT,
		g_param_spec_int ("height",
				  NULL,
				  NULL,
				  0, G_MAXINT, 0,
				  G_PARAM_READWRITE));

	g_object_class_install_property (
		o_class,
		PROP_X1,
		g_param_spec_double ("x1",
				     NULL,
				     NULL,
				     -1, G_MAXDOUBLE, -1,
				     G_PARAM_READWRITE));

	g_object_class_install_property (
		o_class,
		PROP_X2,
		g_param_spec_double ("x2",
				     NULL,
				     NULL,
				     -1, G_MAXDOUBLE, -1,
				     G_PARAM_READWRITE));

	g_object_class_install_property (
		o_class,
		PROP_SCALE,
		g_param_spec_double ("scale",
				     NULL,
				     NULL,
				     0.000001, G_MAXDOUBLE, 1.0,
				     G_PARAM_WRITABLE));

	g_object_class_install_property (
		o_class,
		PROP_ZOOM,
		g_param_spec_double ("zoom",
				     NULL,
				     NULL,
				     -G_MAXDOUBLE, G_MAXDOUBLE, 7,
				     G_PARAM_WRITABLE));

	signals[DATE_HINT_CHANGED] =
		g_signal_new ("date-hint-changed",
			      G_TYPE_FROM_CLASS (class),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      planner_marshal_VOID__STRING,
			      G_TYPE_NONE, 1,
			      G_TYPE_STRING);
}

static void
gantt_header_init (PlannerGanttHeader *header)
{
	PlannerGanttHeaderPriv *priv;

	gtk_widget_set_redraw_on_allocate (GTK_WIDGET (header), FALSE);

	priv = g_new0 (PlannerGanttHeaderPriv, 1);
	header->priv = priv;

	gantt_header_set_adjustments (header, NULL, NULL);

	priv->hscale = 1.0;
	priv->x1 = 0;
	priv->x2 = 0;
	priv->height = -1;
	priv->width = -1;

	priv->major_unit = MRP_TIME_UNIT_MONTH;
	priv->minor_unit = MRP_TIME_UNIT_WEEK;

	priv->layout = gtk_widget_create_pango_layout (GTK_WIDGET (header),
						       NULL);
}

static void
gantt_header_set_zoom (PlannerGanttHeader *header, gdouble zoom)
{
	PlannerGanttHeaderPriv *priv;
	gint                   level;

	priv = header->priv;

	level = planner_scale_clamp_zoom (zoom);

	priv->major_unit = planner_scale_conf[level].major_unit;
	priv->major_format = planner_scale_conf[level].major_format;

	priv->minor_unit = planner_scale_conf[level].minor_unit;
	priv->minor_format = planner_scale_conf[level].minor_format;
}

static void
gantt_header_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
	PlannerGanttHeader     *header;
	PlannerGanttHeaderPriv *priv;
	gdouble                 tmp;
	gint                    width;
	gdouble                 tmp_scale;
	gboolean                change_width = FALSE;
	gboolean                change_height = FALSE;
	gboolean                change_scale = FALSE;

	header = PLANNER_GANTT_HEADER (object);
	priv = header->priv;

	switch (prop_id) {
	case PROP_HEIGHT:
		priv->height = g_value_get_int (value);
		change_height = TRUE;
		break;
	case PROP_X1:
		tmp = g_value_get_double (value);
		if (tmp != priv->x1) {
			priv->x1 = tmp;
			change_width = TRUE;
		}
		break;
	case PROP_X2:
		tmp = g_value_get_double (value);
		if (tmp != priv->x2) {
			priv->x2 = tmp;
			change_width = TRUE;
		}
		break;
	case PROP_SCALE:
		tmp_scale = g_value_get_double (value);
		if (tmp_scale != priv->hscale) {
			priv->hscale = tmp_scale;
			change_scale = TRUE;
		}
		break;
	case PROP_ZOOM:
		gantt_header_set_zoom (header, g_value_get_double (value));
		break;
	default:
		break;
	}

	if (change_width) {
		if (priv->x1 > 0 && priv->x2 > 0) {
			width = floor (priv->x2 - priv->x1 + 0.5);

			/* If both widths aren't set yet, this can happen: */
			if (width < -1) {
				width = -1;
			}
		} else {
			width = -1;
		}
		priv->width = width;
	}

	if (change_width || change_height) {
		gtk_widget_set_size_request (GTK_WIDGET (header),
					     priv->width,
					     priv->height);
	}

	if ((change_width || change_height || change_scale) && GTK_WIDGET_REALIZED (header)) {
		gdk_window_invalidate_rect (priv->bin_window,
					    NULL,
					    FALSE);
	}
}

static void
gantt_header_get_property (GObject    *object,
			   guint       prop_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gantt_header_finalize (GObject *object)
{
	PlannerGanttHeader *header = PLANNER_GANTT_HEADER (object);

	g_object_unref (header->priv->layout);

	g_free (header->priv->date_hint);

	g_free (header->priv);

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}

static void
gantt_header_destroy (GtkObject *object)
{
	/*PlannerGanttHeader *header = PLANNER_GANTT_HEADER (object);*/

	if (GTK_OBJECT_CLASS (parent_class)->destroy) {
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
	}
}

static void
gantt_header_map (GtkWidget *widget)
{
	PlannerGanttHeader *header;

	header = PLANNER_GANTT_HEADER (widget);

	GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);

	gdk_window_show (header->priv->bin_window);
	gdk_window_show (widget->window);
}

static void
gantt_header_realize (GtkWidget *widget)
{
	PlannerGanttHeader *header;
	GdkWindowAttr       attributes;
	gint                attributes_mask;

	header = PLANNER_GANTT_HEADER (widget);

	GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

	/* Create the main, clipping window. */
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = widget->allocation.width;
	attributes.height = widget->allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual (widget);
	attributes.colormap = gtk_widget_get_colormap (widget);
	attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK;

	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

	widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
					 &attributes, attributes_mask);
	gdk_window_set_user_data (widget->window, widget);

	/* Bin window. */
	attributes.x = 0;
	attributes.y = header->priv->height;
	attributes.width = header->priv->width;
	attributes.height = widget->allocation.height;
	attributes.event_mask = GDK_EXPOSURE_MASK |
		GDK_SCROLL_MASK |
		GDK_POINTER_MOTION_MASK |
		GDK_ENTER_NOTIFY_MASK |
		GDK_LEAVE_NOTIFY_MASK |
		GDK_BUTTON_PRESS_MASK |
		GDK_BUTTON_RELEASE_MASK |
		gtk_widget_get_events (widget);

	header->priv->bin_window = gdk_window_new (widget->window,
						   &attributes,
						   attributes_mask);
	gdk_window_set_user_data (header->priv->bin_window, widget);

	widget->style = gtk_style_attach (widget->style, widget->window);
	gdk_window_set_background (widget->window,
				   &widget->style->base[widget->state]);
	gdk_window_set_background (header->priv->bin_window,
				   &widget->style->base[widget->state]);
}

static void
gantt_header_unrealize (GtkWidget *widget)
{
	PlannerGanttHeader *header;

	header = PLANNER_GANTT_HEADER (widget);

	gdk_window_set_user_data (header->priv->bin_window, NULL);
	gdk_window_destroy (header->priv->bin_window);
	header->priv->bin_window = NULL;

	if (GTK_WIDGET_CLASS (parent_class)->unrealize) {
		(* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
	}
}

static void
gantt_header_size_allocate (GtkWidget     *widget,
			    GtkAllocation *allocation)
{
	PlannerGanttHeader *header;

	header = PLANNER_GANTT_HEADER (widget);

	if (GTK_WIDGET_REALIZED (widget)) {
		gdk_window_move_resize (widget->window,
					allocation->x, allocation->y,
					allocation->width, allocation->height);
		gdk_window_move_resize (header->priv->bin_window,
					- (gint) header->priv->hadjustment->value,
					0,
					MAX (header->priv->width, allocation->width),
					allocation->height);
	}
}

static gboolean
gantt_header_expose_event (GtkWidget      *widget,
			   GdkEventExpose *event)
{
	PlannerGanttHeader     *header;
	PlannerGanttHeaderPriv *priv;
	gint                    width, height;
	gdouble                 hscale;
	gint                    x;
	mrptime                 t0;
	mrptime                 t1;
	mrptime                 t;
	gchar                  *str;
	gint                    minor_width;
	gint                    major_width;
	GdkGC                  *gc;
	GdkRectangle            rect;

	header = PLANNER_GANTT_HEADER (widget);
	priv = header->priv;
	hscale = priv->hscale;

	t0 = floor ((priv->x1 + event->area.x) / hscale + 0.5);
	t1 = floor ((priv->x1 + event->area.x + event->area.width) / hscale + 0.5);

	gdk_drawable_get_size (event->window, &width, &height);

	/* Draw background. We only draw over the exposed area, padding with +/-
	 * 5 so we don't mess up the header with button edges all over.
	 */
	gtk_paint_box (widget->style,
		       event->window,
		       GTK_STATE_NORMAL,
		       GTK_SHADOW_OUT,
		       &event->area,
		       widget,
		       "button",
		       event->area.x - 5,
		       0,
		       event->area.width + 10,
		       height);

	gdk_draw_line (event->window,
		       widget->style->fg_gc[GTK_STATE_INSENSITIVE],
		       event->area.x,
		       height / 2,
		       event->area.x + event->area.width,
		       height / 2);

	/* Get the widths of major/minor ticks so that we know how wide to make
	 * the clip region.
	 */
	major_width = hscale * (mrp_time_align_next (t0, priv->major_unit) -
				mrp_time_align_prev (t0, priv->major_unit));

	minor_width = hscale * (mrp_time_align_next (t0, priv->minor_unit) -
				mrp_time_align_prev (t0, priv->minor_unit));

	gc = gdk_gc_new (widget->window);
	gdk_gc_copy (gc, widget->style->text_gc[GTK_STATE_NORMAL]);

	rect.y = 0;
	rect.height = height;

	/* Draw the major scale. */
	if (major_width < 2 || priv->major_unit == MRP_TIME_UNIT_NONE) {
		/* Unless it's too thin to make sense. */
		goto minor_ticks;
	}

	t = mrp_time_align_prev (t0, priv->major_unit);

	while (t <= t1) {
		x = floor (t * hscale - priv->x1 + 0.5);

		gdk_draw_line (event->window,
			       widget->style->fg_gc[GTK_STATE_INSENSITIVE],
			       x, 0,
			       x, height / 2);

		str = planner_scale_format_time (t,
					    priv->major_unit,
					    priv->major_format);
		pango_layout_set_text (priv->layout,
				       str,
				       -1);
		g_free (str);

		rect.x = x;
		rect.width = major_width;
		gdk_gc_set_clip_rectangle (gc, &rect);

		gdk_draw_layout (event->window,
				 gc,
				 x + 3,
				 2,
				 priv->layout);

		t = mrp_time_align_next (t, priv->major_unit);
	}

 minor_ticks:

	/* Draw the minor scale. */
	if (minor_width < 2 || priv->major_unit == MRP_TIME_UNIT_NONE) {
		/* Unless it's too thin to make sense. */
		goto done;
	}

	t = mrp_time_align_prev (t0, priv->minor_unit);

	while (t <= t1) {
		x = floor (t * hscale - priv->x1 + 0.5);

		gdk_draw_line (event->window,
			       widget->style->fg_gc[GTK_STATE_INSENSITIVE],
			       x, height / 2,
			       x, height);

		str = planner_scale_format_time (t,
					    priv->minor_unit,
					    priv->minor_format);
		pango_layout_set_text (priv->layout,
				       str,
				       -1);
		g_free (str);

		rect.x = x;
		rect.width = minor_width;
		gdk_gc_set_clip_rectangle (gc, &rect);

		gdk_draw_layout (event->window,
				 gc,
				 x + 3,
				 height / 2 + 2,
				 priv->layout);

		t = mrp_time_align_next (t, priv->minor_unit);
	}

 done:
	gdk_gc_unref (gc);

	return TRUE;
}

static gboolean
gantt_header_motion_notify_event (GtkWidget	 *widget,
				  GdkEventMotion *event)
{
	PlannerGanttHeader     *header;
	PlannerGanttHeaderPriv *priv;
	mrptime                 t;
	char                   *str;

	header = PLANNER_GANTT_HEADER (widget);
	priv = header->priv;

	t = floor ((priv->x1 + event->x) / priv->hscale + 0.5);
	str = mrp_time_format (_("%a, %e %b %Y"), t);

	if (!priv->date_hint || strcmp (str, priv->date_hint) != 0) {
		g_signal_emit (widget, signals[DATE_HINT_CHANGED], 0, str);

		g_free (priv->date_hint);
		priv->date_hint = str;
	} else {
		g_free (str);
	}

	return FALSE;
}

static gboolean
gantt_header_leave_notify_event (GtkWidget	  *widget,
				 GdkEventCrossing *event)
{
	PlannerGanttHeader     *header;
	PlannerGanttHeaderPriv *priv;

	header = PLANNER_GANTT_HEADER (widget);
	priv = header->priv;

	if (priv->date_hint) {
		g_signal_emit (widget, signals[DATE_HINT_CHANGED], 0, NULL);

		g_free (priv->date_hint);
		priv->date_hint = NULL;
	}

	return FALSE;
}

/* Callbacks */
static void
gantt_header_set_adjustments (PlannerGanttHeader *header,
			      GtkAdjustment      *hadj,
			      GtkAdjustment      *vadj)
{
	if (hadj == NULL) {
		hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
	}

	if (header->priv->hadjustment && (header->priv->hadjustment != hadj)) {
		gtk_object_unref (GTK_OBJECT (header->priv->hadjustment));
	}

	if (header->priv->hadjustment != hadj) {
		header->priv->hadjustment = hadj;
		gtk_object_ref (GTK_OBJECT (header->priv->hadjustment));
		gtk_object_sink (GTK_OBJECT (header->priv->hadjustment));

		g_signal_connect (hadj,
				  "value_changed",
				  G_CALLBACK (gantt_header_adjustment_changed),
				  header);

		gtk_widget_set_scroll_adjustments (GTK_WIDGET (header),
						   hadj,
						   NULL);
	}
}

static void
gantt_header_adjustment_changed (GtkAdjustment *adjustment,
				 PlannerGanttHeader *header)
{
	if (GTK_WIDGET_REALIZED (header)) {
		gdk_window_move (header->priv->bin_window,
				 -header->priv->hadjustment->value,
				 0);
	}
}

GtkWidget *
planner_gantt_header_new (void)
{
	return g_object_new (PLANNER_TYPE_GANTT_HEADER, NULL);
}

