/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2002 CodeFactory AB
 * Copyright (C) 2001-2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
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

#ifndef __MRP_ASSIGNMENT_H__
#define __MRP_ASSIGNMENT_H__

#include <libplanner/mrp-object.h>
#include <libplanner/mrp-time.h>
#include <libplanner/mrp-types.h>

#define MRP_TYPE_ASSIGNMENT         (mrp_assignment_get_type ())
#define MRP_ASSIGNMENT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MRP_TYPE_ASSIGNMENT, MrpAssignment))
#define MRP_ASSIGNMENT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MRP_TYPE_ASSIGNMENT, MrpAssignmentClass))
#define MRP_IS_ASSIGNMENT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MRP_TYPE_ASSIGNMENT))
#define MRP_IS_ASSIGNMENT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MRP_TYPE_ASSIGNMENT))
#define MRP_ASSIGNMENT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MRP_TYPE_ASSIGNMENT, MrpAssignmentClass))

typedef struct _MrpAssignmentClass MrpAssignmentClass;
typedef struct _MrpAssignmentPriv  MrpAssignmentPriv;

struct _MrpAssignment {
        MrpObject          parent;

        MrpAssignmentPriv *priv;
};

struct _MrpAssignmentClass {
        MrpObjectClass parent_class;
};

GType              mrp_assignment_get_type     (void) G_GNUC_CONST;

MrpAssignment     *mrp_assignment_new          (void);

MrpTask           *mrp_assignment_get_task     (MrpAssignment *assignment);
MrpResource       *mrp_assignment_get_resource (MrpAssignment *assignment);
gint               mrp_assignment_get_units    (MrpAssignment *assignment);

#endif /* __MRP_ASSIGNMENT_H__ */
