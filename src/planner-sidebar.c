/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 *
 * Darkening code stolen from GTK+.
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

#include <math.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "planner-sidebar.h"

struct _PlannerSidebarPriv {
	GList           *buttons;
	GtkWidget       *event_box;
	GtkWidget       *vbox;

	GtkToggleButton *selected_button;
};

typedef struct {
	GtkToggleButton *button;
	GtkWidget       *label;
} ButtonEntry;

enum {
	ICON_SELECTED,
	LAST_SIGNAL
};

static void     sidebar_class_init           (PlannerSidebarClass *klass);
static void     sidebar_init                 (PlannerSidebar      *bar);
static void     sidebar_finalize             (GObject        *object);
static void     sidebar_destroy              (GtkObject      *object);
static void     sidebar_event_box_realize_cb (GtkWidget      *widget,
					      PlannerSidebar      *bar);
static void     sidebar_style_set            (GtkWidget      *widget,
					      GtkStyle       *previous_style);


static GtkVBoxClass *parent_class = NULL;
static guint signals[LAST_SIGNAL];


GtkType
planner_sidebar_get_type (void)
{
	static GtkType planner_sidebar_type = 0;

	if (!planner_sidebar_type) {
		static const GTypeInfo planner_sidebar_info = {
			sizeof (PlannerSidebarClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) sidebar_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (PlannerSidebar),
			0,              /* n_preallocs */
			(GInstanceInitFunc) sidebar_init
		};

		planner_sidebar_type = g_type_register_static (GTK_TYPE_FRAME, "PlannerSidebar",
							  &planner_sidebar_info, 0);
	}

	return planner_sidebar_type;
}

static void
sidebar_class_init (PlannerSidebarClass *class)
{
	GObjectClass   *o_class;
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;

	parent_class = g_type_class_peek_parent (class);

	o_class = (GObjectClass *) class;
	object_class = (GtkObjectClass *) class;
	widget_class = (GtkWidgetClass *) class;

	o_class->finalize = sidebar_finalize;

	object_class->destroy = sidebar_destroy;

	widget_class->style_set = sidebar_style_set;

	/* Signals. */
	signals[ICON_SELECTED] = g_signal_new
		("icon_selected",
		 G_TYPE_FROM_CLASS (class),
		 G_SIGNAL_RUN_LAST,
		 0,
		 NULL, NULL,
		 g_cclosure_marshal_VOID__INT,
		 G_TYPE_NONE, 1, G_TYPE_INT);
}

static void
sidebar_init (PlannerSidebar *bar)
{
	bar->priv = g_new0 (PlannerSidebarPriv, 1);

	bar->priv->event_box = gtk_event_box_new ();
	gtk_widget_show (bar->priv->event_box);
	g_signal_connect (bar->priv->event_box,
			  "realize",
			  G_CALLBACK (sidebar_event_box_realize_cb),
			  bar);

	bar->priv->vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (bar->priv->vbox);

	gtk_container_add (GTK_CONTAINER (bar), bar->priv->event_box);

	gtk_container_add (GTK_CONTAINER (bar->priv->event_box), bar->priv->vbox);

	gtk_container_set_border_width (GTK_CONTAINER (bar->priv->vbox), 6);
	gtk_frame_set_shadow_type (GTK_FRAME (bar), GTK_SHADOW_OUT);
}

static void
sidebar_finalize (GObject *object)
{
	PlannerSidebar *bar = PLANNER_SIDEBAR (object);

	g_list_foreach (bar->priv->buttons, (GFunc) g_free, NULL);
	g_list_free (bar->priv->buttons);

	g_free (bar->priv);

	if (G_OBJECT_CLASS (parent_class)->finalize) {
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
	}
}

static void
sidebar_destroy (GtkObject *object)
{
	if (GTK_OBJECT_CLASS (parent_class)->destroy) {
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
	}
}

static void
sidebar_hls_to_rgb (gdouble *h,
		    gdouble *l,
		    gdouble *s)
{
	gdouble hue;
	gdouble lightness;
	gdouble saturation;
	gdouble m1, m2;
	gdouble r, g, b;

	lightness = *l;
	saturation = *s;

	if (lightness <= 0.5)
		m2 = lightness * (1 + saturation);
	else
		m2 = lightness + saturation - lightness * saturation;
	m1 = 2 * lightness - m2;

	if (saturation == 0) {
		*h = lightness;
		*l = lightness;
		*s = lightness;
	} else {
		hue = *h + 120;
		while (hue > 360)
			hue -= 360;
		while (hue < 0)
			hue += 360;

		if (hue < 60)
			r = m1 + (m2 - m1) * hue / 60;
		else if (hue < 180)
			r = m2;
		else if (hue < 240)
			r = m1 + (m2 - m1) * (240 - hue) / 60;
		else
			r = m1;

		hue = *h;
		while (hue > 360)
			hue -= 360;
		while (hue < 0)
			hue += 360;

		if (hue < 60)
			g = m1 + (m2 - m1) * hue / 60;
		else if (hue < 180)
			g = m2;
		else if (hue < 240)
			g = m1 + (m2 - m1) * (240 - hue) / 60;
		else
			g = m1;

		hue = *h - 120;
		while (hue > 360)
			hue -= 360;
		while (hue < 0)
			hue += 360;

		if (hue < 60)
			b = m1 + (m2 - m1) * hue / 60;
		else if (hue < 180)
			b = m2;
		else if (hue < 240)
			b = m1 + (m2 - m1) * (240 - hue) / 60;
		else
			b = m1;

		*h = r;
		*l = g;
		*s = b;
	}
}

static void
sidebar_rgb_to_hls (gdouble *r,
		    gdouble *g,
		    gdouble *b)
{
	gdouble min;
	gdouble max;
	gdouble red;
	gdouble green;
	gdouble blue;
	gdouble h, l, s;
	gdouble delta;

	red = *r;
	green = *g;
	blue = *b;

	if (red > green) {
		if (red > blue)
			max = red;
		else
			max = blue;

		if (green < blue)
			min = green;
		else
			min = blue;
	} else {
		if (green > blue)
			max = green;
		else
			max = blue;

		if (red < blue)
			min = red;
		else
			min = blue;
	}

	l = (max + min) / 2;
	s = 0;
	h = 0;

	if (max != min) {
		if (l <= 0.5)
			s = (max - min) / (max + min);
		else
			s = (max - min) / (2 - max - min);

		delta = max -min;
		if (red == max)
			h = (green - blue) / delta;
		else if (green == max)
			h = 2 + (blue - red) / delta;
		else if (blue == max)
			h = 4 + (red - green) / delta;

		h *= 60;
		if (h < 0.0)
			h += 360;
	}

	*r = h;
	*g = l;
	*b = s;
}

static void
sidebar_style_shade (GdkColor *a,
		     GdkColor *b,
		     gdouble   k)
{
	gdouble red;
	gdouble green;
	gdouble blue;

	red = (gdouble) a->red / 65535.0;
	green = (gdouble) a->green / 65535.0;
	blue = (gdouble) a->blue / 65535.0;

	sidebar_rgb_to_hls (&red, &green, &blue);

	green *= k;
	if (green > 1.0)
		green = 1.0;
	else if (green < 0.0)
		green = 0.0;

	blue *= k;
	if (blue > 1.0)
		blue = 1.0;
	else if (blue < 0.0)
		blue = 0.0;

	sidebar_hls_to_rgb (&red, &green, &blue);

	b->red = red * 65535.0;
	b->green = green * 65535.0;
	b->blue = blue * 65535.0;
}

static void
sidebar_darken_color (GdkColor *color,
		      gint      darken_count)
{
	GdkColor src = *color;
	GdkColor shaded = { 0 };

	while (darken_count) {
		sidebar_style_shade (&src, &shaded, 0.93);
		src = shaded;
		darken_count--;
	}

	*color = shaded;
}

static void
sidebar_modify_bg (PlannerSidebar *bar)
{
	GList       *l;
	GdkColor     normal_color;
	GdkColor     prelight_color;
	GtkStyle    *style;
	ButtonEntry *entry;
	GtkWidget   *button;

	style = GTK_WIDGET (bar)->style;

	normal_color = style->dark[GTK_STATE_NORMAL];
	sidebar_darken_color (&normal_color, 4);

	prelight_color = style->dark[GTK_STATE_PRELIGHT];
	sidebar_darken_color (&prelight_color, 4);

	gtk_widget_modify_bg (bar->priv->event_box, GTK_STATE_NORMAL, &normal_color);

	for (l = bar->priv->buttons; l; l = l->next) {
		entry = l->data;

		button = GTK_WIDGET (entry->button);

		gtk_widget_modify_bg (button, GTK_STATE_NORMAL, &normal_color);
		gtk_widget_modify_bg (button, GTK_STATE_ACTIVE, &normal_color);
		gtk_widget_modify_bg (button, GTK_STATE_PRELIGHT, &prelight_color);

		gtk_widget_modify_fg (entry->label, GTK_STATE_NORMAL, &style->white);
	}
}

static void
sidebar_event_box_realize_cb (GtkWidget *widget, PlannerSidebar *bar)
{
	sidebar_modify_bg (bar);
}

static void
sidebar_style_set (GtkWidget *widget,
		   GtkStyle  *previous_style)
{
	PlannerSidebar *bar = PLANNER_SIDEBAR (widget);

	if (GTK_WIDGET_CLASS (parent_class)->style_set) {
		GTK_WIDGET_CLASS (parent_class)->style_set (widget,
							    previous_style);
	}

	sidebar_modify_bg (bar);
}

GtkWidget *
planner_sidebar_new (void)
{

	return g_object_new (PLANNER_TYPE_SIDEBAR, NULL);
}

static void
sidebar_button_toggled_cb (GtkToggleButton *button,
			   PlannerSidebar       *sidebar)
{
	PlannerSidebarPriv   *priv;
	ButtonEntry     *entry;
	gint             index;
	GtkToggleButton *selected_button;

	priv = sidebar->priv;

	if (priv->selected_button == button) {
		gtk_toggle_button_set_active (button, TRUE);
		return;
	}

	if (!gtk_toggle_button_get_active (button)) {
		return;
	}

	entry = g_object_get_data (G_OBJECT (button), "entry");
	index = g_list_index (priv->buttons, entry);

	selected_button = priv->selected_button;
	priv->selected_button = button;
	if (selected_button) {
		gtk_toggle_button_set_active (selected_button, FALSE);
	}

	g_signal_emit (sidebar, signals[ICON_SELECTED], 0, index);
}

void
planner_sidebar_append (PlannerSidebar *sidebar,
			const gchar    *icon_filename,
			const gchar    *text)
{
	PlannerSidebarPriv *priv;
	ButtonEntry        *entry;
	GtkWidget          *vbox;
	GtkWidget          *image;

	AtkObject          *atk_label;
	AtkRelationSet     *relation_set;
	AtkRelation        *relation;
	AtkObject          *targets[1];

	g_return_if_fail (PLANNER_IS_SIDEBAR (sidebar));

	priv = sidebar->priv;

	entry = g_new0 (ButtonEntry, 1);

	vbox = gtk_vbox_new (FALSE, 0);

	image = gtk_image_new_from_file (icon_filename);

	entry->button = GTK_TOGGLE_BUTTON (gtk_toggle_button_new ());
	g_object_set_data (G_OBJECT (entry->button), "entry", entry);
	gtk_button_set_relief (GTK_BUTTON (entry->button), GTK_RELIEF_NONE);

	sidebar->priv->buttons = g_list_append (priv->buttons, entry);

	g_signal_connect (entry->button,
			  "toggled",
			  G_CALLBACK (sidebar_button_toggled_cb),
			  sidebar);

	gtk_container_add (GTK_CONTAINER (entry->button), image);

	gtk_box_pack_start (GTK_BOX (vbox),
			    GTK_WIDGET (entry->button),
			    FALSE,
			    TRUE,
			    0);

	entry->label = gtk_label_new (text);
	gtk_label_set_justify (GTK_LABEL (entry->label), GTK_JUSTIFY_CENTER);

	gtk_box_pack_start (GTK_BOX (vbox),
			    entry->label,
			    FALSE,
			    TRUE,
			    2);

	gtk_widget_show_all (vbox);

	gtk_box_pack_start (GTK_BOX (priv->vbox),
			    vbox,
			    FALSE,
			    TRUE,
			    6);

	/* Set a LABEL_FOR relation between the label and the button for accessibility */
	atk_label = gtk_widget_get_accessible (GTK_WIDGET (entry->label));
	relation_set = atk_object_ref_relation_set (atk_label);

	targets[0] = gtk_widget_get_accessible (GTK_WIDGET (entry->button));
	relation = atk_relation_new (targets, 1, ATK_RELATION_LABEL_FOR);

	atk_relation_set_add (relation_set, relation);
	g_object_unref (G_OBJECT (relation));
}

void
planner_sidebar_set_active (PlannerSidebar *sidebar,
			    gint            index)
{
	PlannerSidebarPriv *priv;
	ButtonEntry   *entry;

	g_return_if_fail (PLANNER_IS_SIDEBAR (sidebar));

	priv = sidebar->priv;

	entry = g_list_nth_data (priv->buttons, index);
	if (!entry) {
		return;
	}

	if (entry->button != priv->selected_button) {
		if (priv->selected_button) {
			gtk_toggle_button_set_active (priv->selected_button, FALSE);
		}

		gtk_toggle_button_set_active (entry->button, TRUE);

		priv->selected_button = entry->button;
	}
}
