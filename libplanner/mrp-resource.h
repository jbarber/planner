/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __MRP_RESOURCE_H__
#define __MRP_RESOURCE_H__

#include <libplanner/mrp-object.h>
#include <libplanner/mrp-task.h>


#define MRP_TYPE_RESOURCE         (mrp_resource_get_type ())
#define MRP_RESOURCE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MRP_TYPE_RESOURCE, MrpResource))
#define MRP_RESOURCE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MRP_TYPE_RESOURCE, MrpResourceClass))
#define MRP_IS_RESOURCE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MRP_TYPE_RESOURCE))
#define MRP_IS_RESOURCE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MRP_TYPE_RESOURCE))
#define MRP_RESOURCE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MRP_TYPE_RESOURCE, MrpResourceClass))

typedef struct _MrpResourceClass MrpResourceClass;
typedef struct _MrpResourcePriv  MrpResourcePriv;

#include <libplanner/mrp-calendar.h>

struct _MrpResource {
        MrpObject        parent;

        MrpResourcePriv *priv;
};

struct _MrpResourceClass {
        MrpObjectClass parent_class;
};

typedef enum {
	MRP_RESOURCE_TYPE_NONE,
	MRP_RESOURCE_TYPE_WORK,
	MRP_RESOURCE_TYPE_MATERIAL
} MrpResourceType;

GType           mrp_resource_get_type           (void) G_GNUC_CONST;
MrpResource *   mrp_resource_new                (void);
const gchar *   mrp_resource_get_name           (MrpResource     *resource);
void            mrp_resource_set_name           (MrpResource     *resource,
						 const gchar     *name);
const gchar *   mrp_resource_get_short_name     (MrpResource     *resource);
void            mrp_resource_set_short_name     (MrpResource     *resource,
						 const gchar     *short_name);
void            mrp_resource_assign             (MrpResource     *resource,
						 MrpTask         *task,
						 gint             units);

GList *         mrp_resource_get_assignments    (MrpResource     *resource);
 
GList *         mrp_resource_get_assigned_tasks (MrpResource     *resource);

gint            mrp_resource_compare            (gconstpointer    a,
						 gconstpointer    b);

MrpCalendar *   mrp_resource_get_calendar       (MrpResource     *resource);

void            mrp_resource_set_calendar       (MrpResource     *resource,
						 MrpCalendar     *calendar);

#endif /* __MRP_RESOURCE_H__ */
