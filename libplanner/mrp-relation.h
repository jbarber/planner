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

#ifndef __MRP_RELATION_H__
#define __MRP_RELATION_H__

#include <glib-object.h>

#define MRP_TYPE_RELATION         (mrp_relation_get_type ())
#define MRP_RELATION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MRP_TYPE_RELATION, MrpRelation))
#define MRP_RELATION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MRP_TYPE_RELATION, MrpRelationClass))
#define MRP_IS_RELATION(o)	 (G_TYPE_CHECK_INSTANCE_TYPE ((o), MRP_TYPE_RELATION))
#define MRP_IS_RELATION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MRP_TYPE_RELATION))
#define MRP_RELATION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MRP_TYPE_RELATION, MrpRelationClass))

typedef struct _MrpRelation     MrpRelation;
typedef struct _MrpRelationPriv MrpRelationPriv;

#include <libplanner/mrp-task.h>

struct _MrpRelation {
	MrpObject        parent;
	MrpRelationPriv *priv;
};

typedef struct {
	MrpObjectClass   parent_class;
} MrpRelationClass;


GType            mrp_relation_get_type             (void) G_GNUC_CONST;

MrpTask         *mrp_relation_get_predecessor      (MrpRelation     *relation);

MrpTask         *mrp_relation_get_successor        (MrpRelation     *relation);

gint             mrp_relation_get_lag              (MrpRelation     *relation);
 
MrpRelationType  mrp_relation_get_relation_type    (MrpRelation     *relation);


#endif /* __MRP_RELATION_H__ */
