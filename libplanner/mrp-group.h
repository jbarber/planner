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

#ifndef __MRP_GROUP_H__
#define __MRP_GROUP_H__

#include <libplanner/mrp-object.h>
#include <libplanner/mrp-types.h>

#define MRP_TYPE_GROUP         (mrp_group_get_type ())
#define MRP_GROUP(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MRP_TYPE_GROUP, MrpGroup))
#define MRP_GROUP_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MRP_TYPE_GROUP, MrpGroupClass))
#define MRP_IS_GROUP(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MRP_TYPE_GROUP))
#define MRP_IS_GROUP_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MRP_TYPE_GROUP))
#define MRP_GROUP_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MRP_TYPE_GROUP, MrpGroupClass))

typedef struct _MrpGroupClass MrpGroupClass;
typedef struct _MrpGroupPriv  MrpGroupPriv;

struct _MrpGroup {
        MrpObject     parent;

        MrpGroupPriv *priv;
};

struct _MrpGroupClass {
        MrpObjectClass parent_class;
};

GType         mrp_group_get_type     (void) G_GNUC_CONST;

MrpGroup *    mrp_group_new          (void);
const gchar * mrp_group_get_name     (MrpGroup    *group);
void          mrp_group_set_name     (MrpGroup    *group,
				      const gchar *name);
#endif /* __MRP_GROUP_H__ */
