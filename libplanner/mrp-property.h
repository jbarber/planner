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

#ifndef __MRP_PROPERTY_H__
#define __MRP_PROPERTY_H__

#include <glib.h>
#include <glib-object.h>

#define MRP_PROPERTY(x)    ((MrpProperty *) x)
#define MRP_TYPE_PROPERTY  (mrp_property_get_type ())

typedef GParamSpec MrpProperty;

typedef enum {
	MRP_PROPERTY_TYPE_NONE,
	MRP_PROPERTY_TYPE_INT,
	MRP_PROPERTY_TYPE_FLOAT,
	MRP_PROPERTY_TYPE_STRING,
	MRP_PROPERTY_TYPE_STRING_LIST,
	MRP_PROPERTY_TYPE_DATE,
	MRP_PROPERTY_TYPE_DURATION,
	MRP_PROPERTY_TYPE_COST
} MrpPropertyType;

GType           mrp_property_get_type          (void) G_GNUC_CONST;

MrpProperty *   mrp_property_new               (const gchar     *name,
						MrpPropertyType  type,
						const gchar     *label,
						const gchar     *description,
						gboolean         user_defined);

const gchar *   mrp_property_get_name          (MrpProperty     *property);

MrpPropertyType mrp_property_get_property_type (MrpProperty     *property);

void            mrp_property_set_label         (MrpProperty     *property,
						const gchar     *label);

const gchar *   mrp_property_get_label         (MrpProperty     *property);

void            mrp_property_set_description   (MrpProperty     *property,
						const gchar     *description);

const gchar *   mrp_property_get_description   (MrpProperty     *property);

void            mrp_property_set_user_defined  (MrpProperty     *property,
						gboolean         user_defined);

gboolean        mrp_property_get_user_defined  (MrpProperty     *property);

MrpProperty *   mrp_property_ref               (MrpProperty     *property);

void            mrp_property_unref             (MrpProperty     *property);

const gchar *   mrp_property_type_as_string    (MrpPropertyType  type);


#endif /* __MRP_PROPERTY_H__ */
