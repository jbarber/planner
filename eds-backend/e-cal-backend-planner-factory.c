/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *
 * Copyright 2004 (C) Alvaro del Castillo <acs@barrapunto.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pthread.h>
#include <string.h>

#include "e-cal-backend-planner-factory.h"
#include "e-cal-backend-planner.h"

typedef struct {
	ECalBackendFactory  parent_object;
} ECalBackendPlannerFactory;

typedef struct {
	ECalBackendFactoryClass parent_class;
} ECalBackendPlannerFactoryClass;

static void
e_cal_backend_planner_factory_instance_init (ECalBackendPlannerFactory *factory)
{
}

static const char *
_get_protocol (ECalBackendFactory *factory)
{
	return "planner";
}

static ECalBackend*
_todos_new_backend (ECalBackendFactory *factory, ESource *source)
{
	return g_object_new (e_cal_backend_planner_get_type (),
			     "source", source,
			     "kind", ICAL_VTODO_COMPONENT,
			     NULL);
}

static icalcomponent_kind
_todos_get_kind (ECalBackendFactory *factory)
{
	return ICAL_VTODO_COMPONENT;
}

static void
todos_backend_factory_class_init (ECalBackendPlannerFactoryClass *klass)
{
	E_CAL_BACKEND_FACTORY_CLASS (klass)->get_protocol = _get_protocol;
	E_CAL_BACKEND_FACTORY_CLASS (klass)->get_kind     = _todos_get_kind;
	E_CAL_BACKEND_FACTORY_CLASS (klass)->new_backend  = _todos_new_backend;
}

static GType
todos_backend_factory_get_type (GTypeModule *module)
{
	GType type;

	GTypeInfo info = {
		sizeof (ECalBackendPlannerFactoryClass),
		NULL, /* base_class_init */
		NULL, /* base_class_finalize */
		(GClassInitFunc)  todos_backend_factory_class_init,
		NULL, /* class_finalize */
		NULL, /* class_data */
		sizeof (ECalBackend),
		0,    /* n_preallocs */
		(GInstanceInitFunc) e_cal_backend_planner_factory_instance_init
	};

	type = g_type_module_register_type (module,
					    E_TYPE_CAL_BACKEND_FACTORY,
					    "ECalBackendPlannerTodosFactory",
					    &info, 0);

	return type;
}

static GType planner_types[1];

void
eds_module_initialize (GTypeModule *module)
{
	planner_types[0] = todos_backend_factory_get_type (module);
}

void
eds_module_shutdown   (void)
{
}

void
eds_module_list_types (const GType **types, int *num_types)
{
	*types = planner_types;
	*num_types = 1;
}
