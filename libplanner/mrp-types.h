/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2001-2003 CodeFactory AB
 * Copyright (C) 2001-2003 Richard Hult <richard@imendio.com>
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
 
#ifndef __MRP_TYPES_H__
#define __MRP_TYPES_H__

#include <libplanner/mrp-time.h>

#define MRP_TYPE_RELATION_TYPE (mrp_relation_type_get_type ())
#define MRP_TYPE_TASK_TYPE     (mrp_task_type_get_type ())
#define MRP_TYPE_TASK_SCHED    (mrp_task_sched_get_type ())
#define MRP_TYPE_PROPERTY_TYPE (mrp_property_type_get_type ())
#define MRP_TYPE_STRING_LIST   (mrp_string_list_get_type ())

typedef enum {
	MRP_RELATION_NONE = 0, /* unset */
	MRP_RELATION_FS,       /* finish-to-start */
	MRP_RELATION_FF,       /* finish-to-finish */
	MRP_RELATION_SS,       /* start-to-start */
	MRP_RELATION_SF        /* start-to-finish */
} MrpRelationType;

typedef enum {
	MRP_CONSTRAINT_ASAP = 0, /* as-soon-as-possible */
	MRP_CONSTRAINT_ALAP,     /* as-late-as-possible */
	MRP_CONSTRAINT_SNET,     /* start-no-earlier-than */
	MRP_CONSTRAINT_FNLT,     /* finish-no-later-than */
	MRP_CONSTRAINT_MSO,      /* must-start-on */
} MrpConstraintType;

struct _MrpConstraint {
	MrpConstraintType type;
	mrptime           time;
};

typedef enum {
	MRP_TASK_TYPE_NORMAL,
	MRP_TASK_TYPE_MILESTONE
} MrpTaskType;

typedef enum {
	MRP_TASK_SCHED_FIXED_WORK,
	MRP_TASK_SCHED_FIXED_DURATION
} MrpTaskSched;


typedef struct _MrpTask       MrpTask;
typedef struct _MrpResource   MrpResource;
typedef struct _MrpGroup      MrpGroup;
typedef struct _MrpAssignment MrpAssignment;
typedef struct _MrpConstraint MrpConstraint;

GType   mrp_relation_type_get_type (void) G_GNUC_CONST;

GType   mrp_task_type_get_type     (void) G_GNUC_CONST;

GType   mrp_task_sched_get_type    (void) G_GNUC_CONST;

GType   mrp_property_type_get_type (void) G_GNUC_CONST;

GList * mrp_string_list_copy       (const GList *list);

void    mrp_string_list_free       (GList       *list);

#endif /* __MRP_TYPES_H__ */
