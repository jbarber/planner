/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Imendio AB
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

#ifndef __MRP_FILE_MODULE_H__
#define __MRP_FILE_MODULE_H__

#include <gmodule.h>
#include "mrp-application.h"
#include "mrp-project.h"

typedef struct _MrpFileModule     MrpFileModule;
typedef struct _MrpFileReader     MrpFileReader;
typedef struct _MrpFileReaderPriv MrpFileReaderPriv;
typedef struct _MrpFileWriter     MrpFileWriter;
typedef struct _MrpFileWriterPriv MrpFileWriterPriv;

struct _MrpFileModule {
	GModule        *handle;
	MrpApplication *app;

	/* Methods */
	void (*init)   (MrpFileModule  *plugin,
			MrpApplication   *app);

	/* This function calls mrp_application_register_output_writer (...) */
};

struct _MrpFileReader {
	MrpFileModule     *module;

	MrpFileReaderPriv *priv;

	gboolean (*read_string) (MrpFileReader   *reader,
				 const gchar     *str,
				 MrpProject      *project,
				 GError         **error);
};

struct _MrpFileWriter {
	MrpFileModule     *module;
	const gchar       *identifier;
	const gchar       *mime_type;

	MrpFileWriterPriv *priv;

	/* Methods */
	gboolean      (*write)                     (MrpFileWriter    *writer,
						    MrpProject       *project,
						    const gchar      *uri,
						    gboolean          force,
						    GError          **error);
	const gchar * (*get_mime_type)             (MrpFileWriter    *writer);
	const gchar * (*get_string)                (MrpFileWriter    *writer);
};

GList *         mrp_file_module_load_all  (MrpApplication   *app);

MrpFileModule * mrp_file_module_new       (void);


void            mrp_file_module_init      (MrpFileModule     *module,
					   MrpApplication    *app);

/* File Reader */
gboolean        mrp_file_reader_read_string   (MrpFileReader     *reader,
					       const gchar       *str,
					       MrpProject        *project,
					       GError           **error);

/* File Writer */
const gchar *   mrp_file_writer_get_string         (MrpFileWriter   *writer);
const gchar *   mrp_file_writer_get_mime_type      (MrpFileWriter   *writer);
gboolean        mrp_file_writer_write              (MrpFileWriter   *writer,
						    MrpProject      *project,
						    const gchar     *uri,
						    gboolean         force,
						    GError         **error);

#endif /* __MRP_FILE_MODULE_H__ */
