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

#ifndef __MRP_PARSER_H__
#define __MRP_PARSER_H__

#include <glib.h>
#include <libplanner/mrp-error.h>
#include "mrp-storage-mrproject.h"


typedef struct {
	gint            predecessor_id;
	gint            successor_id;
	gint            lag;
	MrpRelationType type;
} DelayedRelation;

gboolean mrp_parser_load     (MrpStorageMrproject  *module,
			      const gchar          *uri,
			      GError              **error);
gboolean mrp_parser_save     (MrpStorageMrproject  *module,
			      const gchar          *uri,
			      gboolean              force,
			      GError              **error);
gboolean mrp_parser_to_xml   (MrpStorageMrproject  *module,
			      gchar               **str,
			      GError              **error);
gboolean mrp_parser_from_xml (MrpStorageMrproject  *module,
			      const gchar          *str,
			      GError              **error);


#endif /* __MRP_PARSER_H__ */

