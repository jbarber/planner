/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003 CodeFactory AB
 * Copyright (C) 2003 Richard Hult <richard@imendio.com>
 * Copyright (C) 2003 Mikael Hallendal <micke@imendio.com>
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

#ifndef __MRP_SQL_H__
#define __MRP_SQL_H__

#include "mrp-storage-sql.h"

void     mrp_sql_init         (void);

gboolean mrp_sql_load_project (MrpStorageSQL  *storage,
			       const gchar    *host,
			       const gchar    *port,
			       const gchar    *database,
			       const gchar    *login,
			       const gchar    *password,
			       gint            project_id,
			       GError        **error);

gboolean mrp_sql_save_project (MrpStorageSQL  *storage,
			       gboolean        force,
			       const gchar    *host,
			       const gchar    *port,
			       const gchar    *database,
			       const gchar    *login,
			       const gchar    *password,
			       gint           *project_id,
			       GError        **error);


#endif /* __MRP_SQL_H__ */
