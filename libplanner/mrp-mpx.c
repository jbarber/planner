/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nill; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003-2004 Imendio AB
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <string.h>
#include <gmodule.h>
#include "mrp-file-module.h"
#include "mrp-private.h"

void init (MrpFileModule  *module,
	   MrpApplication *application);


static gboolean
mpx_read_string (MrpFileReader  *reader,
		 const gchar    *str,
		 MrpProject     *project,
		 GError        **error)
{
	gboolean ret;

	g_return_val_if_fail (str != NULL, FALSE);

	ret = TRUE;

	return ret;
}

G_MODULE_EXPORT void
init (MrpFileModule *module, MrpApplication *application)
{
        MrpFileReader *reader;

        reader         = g_new0 (MrpFileReader, 1);
        reader->module = module;
        reader->priv   = NULL;

	reader->read_string = mpx_read_string;

        imrp_application_register_reader (application, reader);
}
