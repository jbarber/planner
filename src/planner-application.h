/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
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

#ifndef __MG_APPLICATION_H__
#define __MG_APPLICATION_H__

#include <bonobo/bonobo-window.h>
#include <bonobo/bonobo-ui-component.h>
#include <libplanner/mrp-application.h>
#include <libplanner/mrp-project.h>
#include <libegg/recent-files/egg-recent-model.h>

#define MG_TYPE_APPLICATION                (planner_application_get_type ())
#define MG_APPLICATION(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), MG_TYPE_APPLICATION, MgApplication))
#define MG_APPLICATION_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), MG_TYPE_APPLICATION, MgApplicationClass))
#define MG_IS_APPLICATION(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MG_TYPE_APPLICATION))
#define MG_IS_APPLICATION_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), MG_TYPE_APPLICATION))
#define MG_APPLICATION_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), MG_TYPE_APPLICATION, MgApplicationClass))

typedef struct _MgApplication        MgApplication;
typedef struct _MgApplicationClass   MgApplicationClass;
typedef struct _MgApplicationPriv    MgApplicationPriv;

struct _MgApplication
{
        MrpApplication     parent;
        
        MgApplicationPriv *priv;
};

struct _MgApplicationClass
{
        MrpApplicationClass parent_class;
};

GType            planner_application_get_type         (void) G_GNUC_CONST;

MgApplication   *planner_application_new              (void);

GtkWidget       *planner_application_new_window       (MgApplication *app);

void             planner_application_exit             (MgApplication *app);

EggRecentModel  *planner_application_get_recent_model (MgApplication *app);

#endif /* __MG_APPLICATION_H__ */
