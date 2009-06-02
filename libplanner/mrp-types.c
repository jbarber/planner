/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002-2003 CodeFactory AB
 * Copyright (C) 2002-2003 Richard Hult <richard@imendio.com>
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

#include <config.h>
#include <glib.h>
#include "mrp-types.h"
#include "mrp-property.h"

GType
mrp_relation_type_get_type (void)
{
	static GType etype = 0;
	if (etype == 0) {
		static const GEnumValue values[] = {
			{ MRP_RELATION_NONE, "MRP_RELATION_NONE", "none" },
			{ MRP_RELATION_FS, "MRP_RELATION_FS", "fs" },
			{ MRP_RELATION_FF, "MRP_RELATION_FF", "ff" },
			{ MRP_RELATION_SS, "MRP_RELATION_SS", "ss" },
			{ MRP_RELATION_SF, "MRP_RELATION_SF", "sf" },
			{ 0, NULL, NULL }
		};
		etype = g_enum_register_static ("MrpRelationType", values);
	}
	return etype;
}

GType
mrp_task_type_get_type (void)
{
	static GType etype = 0;
	if (etype == 0) {
		static const GEnumValue values[] = {
			{ MRP_TASK_TYPE_NORMAL, "MRP_TASK_TYPE_NORMAL", "normal" },
			{ MRP_TASK_TYPE_MILESTONE, "MRP_TASK_TYPE_MILESTONE", "milestone" },
			{ 0, NULL, NULL }
		};
		etype = g_enum_register_static ("MrpTaskType", values);
	}
	return etype;
}

GType
mrp_task_sched_get_type (void)
{
	static GType etype = 0;
	if (etype == 0) {
		static const GEnumValue values[] = {
			{ MRP_TASK_SCHED_FIXED_WORK, "MRP_TASK_SCHED_FIXED_WORK", "fixed_work" },
			{ MRP_TASK_SCHED_FIXED_DURATION, "MRP_TASK_SCHED_FIXED_DURATION", "fixed_duration" },
			{ 0, NULL, NULL }
		};
		etype = g_enum_register_static ("MrpTaskSched", values);
	}
	return etype;
}

GType
mrp_property_type_get_type (void)
{
	static GType etype = 0;
	if (etype == 0) {
		static const GEnumValue values[] = {
			{ MRP_PROPERTY_TYPE_NONE, "MRP_PROPERTY_TYPE_NONE", "none" },
			{ MRP_PROPERTY_TYPE_INT, "MRP_PROPERTY_TYPE_INT", "int" },
			{ MRP_PROPERTY_TYPE_FLOAT, "MRP_PROPERTY_TYPE_FLOAT", "float" },
			{ MRP_PROPERTY_TYPE_STRING, "MRP_PROPERTY_TYPE_STRING", "string" },
			{ MRP_PROPERTY_TYPE_STRING_LIST, "MRP_PROPERTY_TYPE_STRING_LIST", "string_list" },
			{ MRP_PROPERTY_TYPE_DATE, "MRP_PROPERTY_TYPE_DATE", "date" },
			{ MRP_PROPERTY_TYPE_DURATION, "MRP_PROPERTY_TYPE_DURATION", "duration" },
			{ MRP_PROPERTY_TYPE_COST, "MRP_PROPERTY_TYPE_COST", "cost" },
			{ 0, NULL, NULL }
		};
		etype = g_enum_register_static ("MrpPropertyType", values);
	}
	return etype;
}

GList *
mrp_string_list_copy (const GList *list)
{
	const GList *l;
	GList       *copy;

	if (!list) {
		return NULL;
	}

	copy = NULL;
	for (l = list; l; l = l->next) {
		copy = g_list_prepend (copy, g_strdup (l->data));
	}

	return g_list_reverse (copy);
}

void
mrp_string_list_free (GList *list)
{
	GList *l;

	if (!list) {
		return;
	}

	for (l = list; l; l = l->next) {
		g_free (l->data);
	}

	g_list_free (list);
}

