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

#ifndef __MRP_APPLICATION_H__
#define __MRP_APPLICATION_H__

#include <glib-object.h>

#define MRP_TYPE_APPLICATION         (mrp_application_get_type ())
#define MRP_APPLICATION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MRP_TYPE_APPLICATION, MrpApplication))
#define MRP_APPLICATION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MRP_TYPE_APPLICATION, MrpApplicationClass))
#define MRP_IS_APPLICATION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MRP_TYPE_APPLICATION))
#define MRP_IS_APPLICATION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MRP_TYPE_APPLICATION))
#define MRP_APPLICATION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MRP_TYPE_APPLICATION, MrpApplicationClass))

typedef struct _MrpApplication      MrpApplication;
typedef struct _MrpApplicationClass MrpApplicationClass;
typedef struct _MrpApplicationPriv  MrpApplicationPriv;

struct _MrpApplication {
	GObject             parent;

	MrpApplicationPriv *priv;
};

struct _MrpApplicationClass {
	GObjectClass        parent_class;
};


/* General functions.
 */

GType            mrp_application_get_type      (void) G_GNUC_CONST;

MrpApplication * mrp_application_new           (void);
guint            mrp_application_get_unique_id (void);
gpointer         mrp_application_id_get_data   (guint object_id);

#endif /* __MRP_APPLICATION_H__ */
