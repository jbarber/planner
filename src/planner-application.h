/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003 Imendio HB
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

#ifndef __PLANNER_APPLICATION_H__
#define __PLANNER_APPLICATION_H__

#include <bonobo/bonobo-window.h>
#include <bonobo/bonobo-ui-component.h>
#include <gconf/gconf-client.h>
#include <libplanner/mrp-application.h>
#include <libplanner/mrp-project.h>
#include <libegg/recent-files/egg-recent-model.h>


#define PLANNER_TYPE_APPLICATION                (planner_application_get_type ())
#define PLANNER_APPLICATION(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), PLANNER_TYPE_APPLICATION, PlannerApplication))
#define PLANNER_APPLICATION_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), PLANNER_TYPE_APPLICATION, PlannerApplicationClass))
#define PLANNER_IS_APPLICATION(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PLANNER_TYPE_APPLICATION))
#define PLANNER_IS_APPLICATION_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), PLANNER_TYPE_APPLICATION))
#define PLANNER_APPLICATION_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), PLANNER_TYPE_APPLICATION, PlannerApplicationClass))

typedef struct _PlannerApplication        PlannerApplication;
typedef struct _PlannerApplicationClass   PlannerApplicationClass;
typedef struct _PlannerApplicationPriv    PlannerApplicationPriv;

struct _PlannerApplication
{
        MrpApplication          parent;
	PlannerApplicationPriv *priv;
};

struct _PlannerApplicationClass
{
        MrpApplicationClass parent_class;
};

GType                 planner_application_get_type         (void) G_GNUC_CONST;
PlannerApplication   *planner_application_new              (void);
GtkWidget       *     planner_application_new_window       (PlannerApplication *app);
void                  planner_application_exit             (PlannerApplication *app);
EggRecentModel  *     planner_application_get_recent_model (PlannerApplication *app);
GConfClient     *     planner_application_get_gconf_client (void);


#endif /* __PLANNER_APPLICATION_H__ */
