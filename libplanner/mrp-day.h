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

#ifndef __MRP_DAY_H__
#define __MRP_DAY_H__

#define MRP_TYPE_DAY               (mrp_day_get_type ())

typedef struct _MrpDay MrpDay;

#include <libplanner/mrp-project.h>

#define MRP_DAY(o)       (MrpDay *) o

/* Used while saving/loading */
enum {
	MRP_DAY_WORK = 0,
	MRP_DAY_NONWORK = 1,
	MRP_DAY_USE_BASE = 2,
	MRP_DAY_NEXT = 3
};

GType          mrp_day_get_type           (void) G_GNUC_CONST;
MrpDay *       mrp_day_add                (MrpProject  *project,
					   const gchar *name,
					   const gchar *description);
GList *        mrp_day_get_all            (MrpProject  *project);
void           mrp_day_remove             (MrpProject  *project,
					   MrpDay      *day);
gint           mrp_day_get_id             (MrpDay      *day);
const gchar *  mrp_day_get_name           (MrpDay      *day);
void           mrp_day_set_name           (MrpDay      *day,
					   const gchar *name);

const gchar *  mrp_day_get_description    (MrpDay      *day);
void           mrp_day_set_description    (MrpDay      *day,
					   const gchar *description);

MrpDay *       mrp_day_ref                (MrpDay      *day);
void           mrp_day_unref              (MrpDay      *day);

MrpDay *       mrp_day_get_work           (void);
MrpDay *       mrp_day_get_nonwork        (void);
MrpDay *       mrp_day_get_use_base       (void);

#endif /* __MRP_DAY_H__ */
