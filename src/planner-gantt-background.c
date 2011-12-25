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
 * Free Software Foundation, Inc., 59 Temgle Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <glib/gi18n.h>
#include <libplanner/mrp-project.h>
#include <libplanner/mrp-time.h>
#include "planner-gantt-background.h"
#include "planner-scale-utils.h"


/* Object argument IDs */
enum {
	PROP_0,
	PROP_PROJECT,
	PROP_PROJECT_START,
	PROP_SCALE,
	PROP_ZOOM,
	PROP_ROW_HEIGHT,
	PROP_SHOW_GUIDELINES
};

struct _PlannerGanttBackgroundPriv {
	GdkGC       *border_gc;
	GdkGC       *fill_gc;
	GdkGC       *timeline_gc;
	GdkGC       *start_gc;
	GdkGC       *guidelines_gc;

	PangoLayout *layout;

	guint        timeout_id;
	mrptime      timeline;

	MrpProject  *project;
	MrpCalendar *calendar;
	mrptime      project_start;

	gdouble      hscale;
	gdouble      zoom;
	gint	     row_height;
	gboolean     show_guidelines;
};


static void     gantt_background_class_init       (PlannerGanttBackgroundClass *class);
static void     gantt_background_init             (PlannerGanttBackground      *background);
static void     gantt_background_finalize         (GObject                *object);
static void     gantt_background_set_property     (GObject                *object,
						   guint                   param_id,
						   const GValue           *value,
						   GParamSpec             *pspec);
static void     gantt_background_update           (GnomeCanvasItem        *item,
						   double                 *affine,
						   ArtSVP                 *clip_path,
						   gint                    flags);
static double   gantt_background_point            (GnomeCanvasItem        *item,
						   double                  x,
						   double                  y,
						   gint                    cx,
						   gint                    cy,
						   GnomeCanvasItem       **actual_item);
static void     gantt_background_realize          (GnomeCanvasItem        *item);
static void     gantt_background_unrealize        (GnomeCanvasItem        *item);
static void     gantt_background_draw             (GnomeCanvasItem        *item,
						   GdkDrawable            *drawable,
						   gint                    x,
						   gint                    y,
						   gint                    width,
						   gint                    height);
static gboolean gantt_background_update_timeline  (gpointer                data);
static void     gantt_background_calendar_changed (MrpCalendar            *calendar,
						   PlannerGanttBackground      *background);
static void
gantt_background_project_calendar_notify_cb       (MrpProject             *project,
						   GParamSpec             *spec,
						   PlannerGanttBackground      *background);
static void        gantt_background_set_calendar  (PlannerGanttBackground      *background,
						   MrpCalendar            *calendar);


static GnomeCanvasItemClass *parent_class;


GType
planner_gantt_background_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (PlannerGanttBackgroundClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) gantt_background_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (PlannerGanttBackground),
			0,              /* n_preallocs */
			(GInstanceInitFunc) gantt_background_init
		};

		type = g_type_register_static (GNOME_TYPE_CANVAS_ITEM,
					       "PlannerGanttBackground",
					       &info,
					       0);
	}

	return type;
}

static void
gantt_background_class_init (PlannerGanttBackgroundClass *class)
{
	GObjectClass         *gobject_class;
	GnomeCanvasItemClass *item_class;

	gobject_class = (GObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	parent_class = g_type_class_peek_parent (class);

	gobject_class->set_property = gantt_background_set_property;
	gobject_class->finalize = gantt_background_finalize;

	item_class->update = gantt_background_update;
	item_class->bounds = NULL;
	item_class->point = gantt_background_point;
	item_class->realize = gantt_background_realize;
	item_class->unrealize = gantt_background_unrealize;
	item_class->draw = gantt_background_draw;

        g_object_class_install_property (
                gobject_class,
		PROP_PROJECT,
		g_param_spec_object ("project",
				     NULL,
				     NULL,
				     MRP_TYPE_PROJECT,
				     G_PARAM_WRITABLE));

        g_object_class_install_property (
                gobject_class,
		PROP_PROJECT_START,
		mrp_param_spec_time ("project-start",
				     NULL,
				     NULL,
				     G_PARAM_WRITABLE));

        g_object_class_install_property (
                gobject_class,
		PROP_SCALE,
		g_param_spec_double ("scale",
				     NULL,
				     NULL,
				     G_MINDOUBLE, G_MAXDOUBLE,
				     1.0,
				     G_PARAM_WRITABLE));

        g_object_class_install_property (
                gobject_class,
		PROP_ZOOM,
		g_param_spec_double ("zoom",
				     NULL,
				     NULL,
				     -G_MAXDOUBLE, G_MAXDOUBLE,
				     7,
				     G_PARAM_WRITABLE));
	g_object_class_install_property (
		gobject_class,
		PROP_ROW_HEIGHT,
		g_param_spec_int ("row-height",
				  NULL,
				  NULL,
				  0, G_MAXINT, 0,
				  G_PARAM_WRITABLE));

	g_object_class_install_property (
		gobject_class,
		PROP_SHOW_GUIDELINES,
		g_param_spec_boolean ("show-guidelines",
				      NULL,
				      NULL,
				      FALSE,
				      G_PARAM_WRITABLE));
}

static void
gantt_background_init (PlannerGanttBackground *background)
{
	PlannerGanttBackgroundPriv *priv;

	priv = g_new0 (PlannerGanttBackgroundPriv, 1);
	background->priv = priv;

	priv->hscale = 1.0;
	priv->project_start = MRP_TIME_INVALID;
	priv->timeline = mrp_time_current_time ();
	priv->row_height = 0;
	priv->show_guidelines = FALSE;
}

static void
gantt_background_finalize (GObject *object)
{
	PlannerGanttBackground     *background;
	PlannerGanttBackgroundPriv *priv;

	g_return_if_fail (PLANNER_IS_GANTT_BACKGROUND (object));

	background = PLANNER_GANTT_BACKGROUND (object);
	priv = background->priv;

	if (priv->timeout_id) {
		g_source_remove (priv->timeout_id);
		priv->timeout_id = 0;
	}

	g_free (priv);
	background->priv = NULL;

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}

static void
gantt_background_get_bounds (PlannerGanttBackground *background,
			     gdouble           *px1,
			     gdouble           *py1,
			     gdouble           *px2,
			     gdouble           *py2)
{
	/* FIXME: Get the items bbox in canvas pixel coordinates. */

	*px1 = -G_MAXINT;
	*py1 = -G_MAXINT;
	*px2 = G_MAXINT;
	*py2 = G_MAXINT;
}

static double
gantt_background_point (GnomeCanvasItem  *item,
			double            x,
			double            y,
			gint              cx,
			gint              cy,
			GnomeCanvasItem **actual_item)
{
	*actual_item = item;

	return 1000;
}

static void
gantt_background_set_property (GObject      *object,
			       guint         param_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
	GnomeCanvasItem       *item;
	PlannerGanttBackground     *background;
	PlannerGanttBackgroundPriv *priv;
	MrpCalendar           *calendar;

	g_return_if_fail (PLANNER_IS_GANTT_BACKGROUND (object));

	item = GNOME_CANVAS_ITEM (object);
	background = PLANNER_GANTT_BACKGROUND (object);
	priv = background->priv;

	switch (param_id) {
	case PROP_PROJECT:
		if (priv->project) {
			g_signal_handlers_disconnect_by_func (
				priv->project,
				gantt_background_project_calendar_notify_cb,
				background);
		}

		priv->project = g_value_get_object (value);

		g_signal_connect (priv->project,
				  "notify::calendar",
				  G_CALLBACK (gantt_background_project_calendar_notify_cb),
				  background);

		calendar = mrp_project_get_calendar (priv->project);
		gantt_background_set_calendar (background, calendar);
		break;

	case PROP_PROJECT_START:
		priv->project_start = g_value_get_long (value);
		break;

	case PROP_SCALE:
		priv->hscale = g_value_get_double (value);
		break;

	case PROP_ZOOM:
		priv->zoom = g_value_get_double (value);
		break;

	case PROP_ROW_HEIGHT:
		priv->row_height = g_value_get_int (value);
		break;

	case PROP_SHOW_GUIDELINES:
		priv->show_guidelines = g_value_get_boolean (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}

	gnome_canvas_item_request_update (item);
}

static void
gantt_background_update (GnomeCanvasItem *item,
			 double          *affine,
			 ArtSVP          *clip_path,
			 int              flags)
{
	PlannerGanttBackground *background;
	double             x1, y1, x2, y2;

	background = PLANNER_GANTT_BACKGROUND (item);

	GNOME_CANVAS_ITEM_CLASS (parent_class)->update (item,
							affine,
							clip_path,
							flags);

	gantt_background_get_bounds (background, &x1, &y1, &x2, &y2);

	gnome_canvas_update_bbox (item, x1, y1, x2, y2);
}

static void
gantt_background_realize (GnomeCanvasItem *item)
{
	PlannerGanttBackground     *background;
	PlannerGanttBackgroundPriv *priv;
	GdkColor               color;

	background = PLANNER_GANTT_BACKGROUND (item);
	priv = background->priv;

	GNOME_CANVAS_ITEM_CLASS (parent_class)->realize (item);

	priv->fill_gc = gdk_gc_new (item->canvas->layout.bin_window);
	gnome_canvas_get_color (item->canvas, "grey96", &color);
	gdk_gc_set_foreground (priv->fill_gc, &color);

	gnome_canvas_get_color (item->canvas, "grey80", &color);
	priv->border_gc = gdk_gc_new (item->canvas->layout.bin_window);
	gdk_gc_set_foreground (priv->border_gc, &color);
	gdk_gc_set_line_attributes (priv->border_gc,
				    0,
				    GDK_LINE_SOLID,
				    GDK_CAP_BUTT,
				    GDK_JOIN_MITER);

	gnome_canvas_get_color (item->canvas, "royal blue", &color);
	priv->timeline_gc = gdk_gc_new (item->canvas->layout.bin_window);
	gdk_gc_set_foreground (priv->timeline_gc, &color);
	gdk_gc_set_line_attributes (priv->timeline_gc,
				    0,
				    GDK_LINE_ON_OFF_DASH,
				    GDK_CAP_BUTT,
				    GDK_JOIN_MITER);

	gnome_canvas_get_color (item->canvas, "grey", &color);
	priv->start_gc = gdk_gc_new (item->canvas->layout.bin_window);
	gdk_gc_set_foreground (priv->start_gc, &color);
	gdk_gc_set_line_attributes (priv->start_gc,
				    0,
				    GDK_LINE_ON_OFF_DASH,
				    GDK_CAP_BUTT,
				    GDK_JOIN_MITER);

	gnome_canvas_get_color (item->canvas, "grey80", &color);
	priv->guidelines_gc = gdk_gc_new (item->canvas->layout.bin_window);
	gdk_gc_set_foreground (priv->guidelines_gc, &color);
	gdk_gc_set_line_attributes (priv->guidelines_gc,
				    0,
				    GDK_LINE_SOLID,
				    GDK_CAP_BUTT,
				    GDK_JOIN_MITER);

	priv->layout = gtk_widget_create_pango_layout (GTK_WIDGET (item->canvas),
						       NULL);

	pango_layout_set_alignment (priv->layout, PANGO_ALIGN_RIGHT);

	priv->timeout_id = g_timeout_add_seconds (60,
						  gantt_background_update_timeline,
						  background);
}

static void
gantt_background_unrealize (GnomeCanvasItem *item)
{
	PlannerGanttBackground *background;

	background = PLANNER_GANTT_BACKGROUND (item);

	g_object_unref (background->priv->border_gc);
	background->priv->border_gc = NULL;

	g_object_unref (background->priv->fill_gc);
	background->priv->fill_gc = NULL;

	g_object_unref (background->priv->timeline_gc);
	background->priv->timeline_gc = NULL;

	g_object_unref (background->priv->start_gc);
	background->priv->start_gc = NULL;

	g_object_unref (background->priv->guidelines_gc);
	background->priv->guidelines_gc = NULL;

	g_object_unref (background->priv->layout);
	background->priv->layout = NULL;

	GNOME_CANVAS_ITEM_CLASS (parent_class)->unrealize (item);
}

static gboolean
gantt_background_update_timeline (gpointer data)
{
	PlannerGanttBackground     *background;
	PlannerGanttBackgroundPriv *priv;;

	background = PLANNER_GANTT_BACKGROUND (data);
	priv = background->priv;

	priv->timeline = mrp_time_current_time ();

	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (background));

	return TRUE;
}

static void
gantt_background_draw (GnomeCanvasItem *item,
		       GdkDrawable     *drawable,
		       int              x,
		       int              y,
		       int              width,
		       int              height)
{
	PlannerGanttBackground     *background;
	PlannerGanttBackgroundPriv *priv;
	gint                   cx1, cx2;  /* Canvas pixel coordinates */
	gint                   cy1, cy2;
	gdouble                wx1, wx2;  /* World coordinates */
	gdouble                hscale;
	mrptime                t0;
	mrptime                t1, t2;    /* First and last exposed times */
	mrptime                ival_start, ival_end, ival_prev;
	MrpCalendar           *calendar;
	MrpDay                *day;
	GList                 *ivals, *l;
	MrpInterval           *ival;
	gint                   level;
	gdouble                i2w_dx,i2w_dy;
	gint                   xx,yy;

	background = PLANNER_GANTT_BACKGROUND (item);
	priv = background->priv;

	if (!priv->project) {
		return;
	}

	calendar = mrp_project_get_calendar (priv->project);

	hscale = priv->hscale;
	level = planner_scale_clamp_zoom (priv->zoom);

	gnome_canvas_c2w (item->canvas, x, 0, &wx1, NULL);
	gnome_canvas_c2w (item->canvas, x + width, 0, &wx2, NULL);

	cy1 = y;
	cy2 = y + height;

	t1 = floor (wx1 / hscale + 0.5);
	t2 = floor (wx2 / hscale + 0.5);

	t0 = t1 = mrp_time_align_day (t1 - 24*60*60);
	t2 = mrp_time_align_day (t2 + 24*60*60);

	/* Loop through the days between t0 and t2. */
	while (t1 <= t2) {
		day = mrp_calendar_get_day (calendar, t1, TRUE);

		ivals = mrp_calendar_day_get_intervals (calendar, day, TRUE);

		ival_prev = t1;

		/* Loop through the intervals for this day. */
		for (l = ivals; l; l = l->next) {
			ival = l->data;

			mrp_interval_get_absolute (ival,
						   t1,
						   &ival_start,
						   &ival_end);

			/* Draw the section between the end of the last working
			 * time interval and the start of the current one,
			 * i.e. [ival_prev, ival_start].
			 */

			wx1 = ival_prev * hscale;
			wx2 = ival_start * hscale;

			gnome_canvas_w2c (item->canvas, wx1, 0, &cx1, NULL);
			gnome_canvas_w2c (item->canvas, wx2, 0, &cx2, NULL);

			/* Don't draw if the interval is shorter than what we
			 * want at this zoom level.
			 */
			if (planner_scale_conf[level].nonworking_limit <= ival_start - ival_prev) {
				gdk_draw_rectangle (drawable,
						    priv->fill_gc,
						    TRUE,
						    cx1 - x,
						    cy1 - y,
						    cx2 - cx1,
						    cy2 - cy1);

				gdk_draw_line (drawable,
					       priv->border_gc,
					       cx1 - x,
					       cy1 - y,
					       cx1 - x,
					       cy2 - y);
			}

			ival_prev = ival_end;
		}

		t1 += 60*60*24;

		/* Draw the remaining interval if there is one. */
		if (ival_prev < t1 && planner_scale_conf[level].nonworking_limit <= t1 - ival_prev) {
			wx1 = ival_prev * hscale;
			wx2 = t1 * hscale;

			gnome_canvas_w2c (item->canvas, wx1, 0, &cx1, NULL);
			gnome_canvas_w2c (item->canvas, wx2, 0, &cx2, NULL);

			gdk_draw_rectangle (drawable,
					    priv->fill_gc,
					    TRUE,
					    cx1 - x,
					    cy1 - y,
					    cx2 - cx1,
					    cy2 - cy1);

			gdk_draw_line (drawable,
				       priv->border_gc,
				       cx1 - x,
				       cy1 - y,
				       cx1 - x,
				       cy2 - y);
		}
	}

	/* Guidelines */
	if (priv->show_guidelines) {
		i2w_dx = 0.0;
		i2w_dy = 0.0;
		gnome_canvas_item_i2w (item, &i2w_dx, &i2w_dy);
		gnome_canvas_w2c (item->canvas,
				  0 + i2w_dx,
			  0 + i2w_dy,
				  &xx,
				  &yy);
		while (yy < height+y)
		{
			if (yy >= y)
				gdk_draw_line (drawable,priv->guidelines_gc,0,yy-y,width,yy-y);
			yy += priv->row_height;
		}

	}

	/* Time line for project start .*/
	wx1 = priv->project_start * hscale;
	gnome_canvas_w2c (item->canvas, wx1, 0, &cx1, NULL);

#define DASH_LENGTH 8

	if (priv->project_start >= t0 && priv->project_start <= t2) {
		gint snap;

		/* Align the dashed line, which has a dash length of 8. A bit
		 * ugly :/
		 */
		snap = cy1 - floor (cy1 / (double)DASH_LENGTH + 0.5) * DASH_LENGTH;

		gdk_draw_line (drawable,
			       priv->start_gc,
			       cx1 - x,
			       cy1 - snap - DASH_LENGTH - y,
			       cx1 - x,
			       cy2 + DASH_LENGTH - y);
	}

	if (priv->project_start >= t0) {
		gchar *str, *tmp;
		gint   label_width;

		/* i18n: project start, the date format is described in
		 * libmrproject/docs/DateFormat. */
		str = mrp_time_format (_("%Y %b %d"), priv->project_start);

		tmp = g_strconcat ("<span size=\"smaller\">", _("Project start"),
				   "\n", str, "</span>", NULL);
		pango_layout_set_markup (priv->layout, tmp, -1);
		g_free (tmp);

		g_free (str);

		pango_layout_get_pixel_size (priv->layout, &label_width, NULL);

		gdk_draw_layout (drawable,
				 GTK_WIDGET (item->canvas)->style->text_gc[GTK_STATE_NORMAL],
				 cx1 - label_width - 5 - x,
				 5 - y,
				 priv->layout);
	}

	/* Time line for current time .*/
	if (priv->timeline >= t0 && priv->timeline <= t2) {
		gint snap;

		wx1 = priv->timeline * hscale;
		gnome_canvas_w2c (item->canvas, wx1, 0, &cx1, NULL);

		snap = cy1 - floor (cy1 / (double) DASH_LENGTH + 0.5) * DASH_LENGTH;

		gdk_draw_line (drawable,
			       priv->timeline_gc,
			       cx1 - x,
			       cy1 - snap - DASH_LENGTH - y,
			       cx1 - x,
			       cy2 + DASH_LENGTH - y);
	}
}

static void
gantt_background_calendar_changed (MrpCalendar       *calendar,
				   PlannerGanttBackground *background)
{
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (background));
}

static void
gantt_background_project_calendar_notify_cb (MrpProject        *project,
					     GParamSpec        *spec,
					     PlannerGanttBackground *background)
{
	MrpCalendar *calendar;

	calendar = mrp_project_get_calendar (project);

	gantt_background_set_calendar (background, calendar);
}

static void
gantt_background_set_calendar (PlannerGanttBackground *background,
			       MrpCalendar       *calendar)
{
	PlannerGanttBackgroundPriv *priv;

	priv = background->priv;

	if (calendar == priv->calendar) {
		return;
	}

	if (priv->calendar) {
		g_signal_handlers_disconnect_by_func (
			priv->calendar,
			gantt_background_calendar_changed,
			background);
	}

	if (calendar) {
		g_signal_connect (calendar,
				  "calendar_changed",
				  G_CALLBACK (gantt_background_calendar_changed),
				  background);
	}

	priv->calendar = calendar;

	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (background));
}
