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

#ifndef __MRP_OBJECT_H__
#define __MRP_OBJECT_H__

#include <glib-object.h>
#include <libplanner/mrp-property.h>

#define MRP_TYPE_OBJECT         (mrp_object_get_type ())
#define MRP_OBJECT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MRP_TYPE_OBJECT, MrpObject))
#define MRP_OBJECT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MRP_TYPE_OBJECT, MrpObjectClass))
#define MRP_IS_OBJECT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MRP_TYPE_OBJECT))
#define MRP_IS_OBJECT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MRP_TYPE_OBJECT))
#define MRP_OBJECT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MRP_TYPE_OBJECT, MrpObjectClass))

typedef struct _MrpObject      MrpObject;
typedef struct _MrpObjectClass MrpObjectClass;
typedef struct _MrpObjectPriv  MrpObjectPriv;

struct _MrpObject {
        GObject        parent;

        MrpObjectPriv *priv;
};

struct _MrpObjectClass {
        GObjectClass parent_class;

        /* Signals */
        void (*removed)        (MrpObject   *object);
	void (*prop_changed)   (MrpObject   *object,
				MrpProperty *property,
				GValue      *new_value);
};


GType       mrp_object_get_type       (void) G_GNUC_CONST;
void        mrp_object_removed        (MrpObject   *object);
void        mrp_object_changed        (MrpObject   *object);
void        mrp_object_set            (gpointer     object,
				       const gchar *first_property_name,
				       ...);
void        mrp_object_get            (gpointer     object,
				       const gchar *first_property_name,
				       ...);
void        mrp_object_set_property   (MrpObject   *object,
				       MrpProperty *property,
				       GValue      *value);
void        mrp_object_get_property   (MrpObject   *object,
				       MrpProperty *property,
				       GValue      *value);
void        mrp_object_set_valist     (MrpObject   *object,
				       const gchar *first_property_name,
				       va_list      var_args);
void        mrp_object_get_valist     (MrpObject   *object,
				       const gchar *first_property_name,
				       va_list      var_args);
GList    *  mrp_object_get_properties (MrpObject   *object);
guint       mrp_object_get_id         (MrpObject   *object);
gboolean    mrp_object_set_id         (MrpObject   *object,
				       guint        id);

/* FIXME: Sucks but we have a circular dependency. Could fix properly later. */
gpointer    mrp_object_get_project    (MrpObject   *object);


#endif /* __MRP_OBJECT_H__ */
