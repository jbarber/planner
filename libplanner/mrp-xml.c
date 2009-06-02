/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nill; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Imendio AB
 * Copyright (C) 2002-2003 CodeFactory AB
 * Copyright (C) 2002-2003 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <gmodule.h>
#include <libxml/parser.h>
#include <string.h>

#include <libplanner/mrp-file-module.h>
#include <libplanner/mrp-private.h>
#include "mrp-paths.h"
#include "mrp-old-xml.h"

typedef enum {
	XML_TYPE_UNKNOWN,
	XML_TYPE_MRP_1,
	XML_TYPE_MRP_0_6,
	XML_TYPE_MRP_0_5_1
} XmlType;


void            init             (MrpFileModule   *module,
				  MrpApplication  *application);
static gboolean xml_read_context (xmlParserCtxt   *ctxt,
				  MrpProject      *project,
				  GError         **error);
static gboolean xml_read_string  (MrpFileReader   *reader,
				  const gchar     *str,
				  MrpProject      *project,
				  GError         **error);
static XmlType  xml_locate_type  (xmlDoc          *doc);
static gboolean xml_validate     (xmlDoc          *doc,
				  const gchar     *dtd_path);



static gboolean
xml_read_context (xmlParserCtxt  *ctxt,
		  MrpProject     *project,
		  GError        **error)
{
	xmlDoc   *doc;
	gboolean  ret_val;

	xmlParseDocument (ctxt);

	doc = ctxt->myDoc;
	if (!doc) {
		g_warning ("Could not read XML.");
		return FALSE;
	}

	if (!ctxt->wellFormed) {
		g_warning ("Document not well formed.");
		xmlFreeDoc (doc);
		return FALSE;
	}

	switch (xml_locate_type (doc)) {
	case XML_TYPE_MRP_1:
		g_print ("Isn't implemented yet\n");
		ret_val = FALSE;
		break;
	case XML_TYPE_MRP_0_6:
	case XML_TYPE_MRP_0_5_1:
		ret_val = mrp_old_xml_parse (project, doc, error);
		break;
	default:
		ret_val = FALSE;
		break;
	};

	xmlFreeDoc (doc);

	return ret_val;
}

static gboolean
xml_read_string (MrpFileReader  *reader,
		 const gchar    *str,
		 MrpProject     *project,
		 GError        **error)
{
	xmlParserCtxt *ctxt;
	gboolean       ret_val;

	g_return_val_if_fail (str != NULL, FALSE);

	ctxt = xmlCreateDocParserCtxt ((xmlChar* ) str);
	if (!ctxt) {
		return FALSE;
	}

	ret_val = xml_read_context (ctxt, project, error);

 	xmlFreeParserCtxt (ctxt);

	return ret_val;
}

static XmlType
xml_locate_type (xmlDoc *doc)
{
	XmlType  ret_val = XML_TYPE_UNKNOWN;
	gchar   *filename;

	filename = mrp_paths_get_dtd_dir ("mrproject-0.6.dtd");
	if (xml_validate (doc, filename)) {
		ret_val = XML_TYPE_MRP_0_6;
	} else {
		g_free (filename);
		filename = mrp_paths_get_dtd_dir ("mrproject-0.5.1.dtd");
		if (xml_validate (doc, filename)) {
			ret_val = XML_TYPE_MRP_0_5_1;
		}
	}

	g_free (filename);

	return ret_val;
}

static gboolean
xml_validate (xmlDoc *doc, const gchar *dtd_path)
{
	xmlValidCtxt  cvp;
	xmlDtd       *dtd;
	gboolean      ret_val;

	g_return_val_if_fail (doc != NULL, FALSE);
	g_return_val_if_fail (dtd_path != NULL, FALSE);

	memset (&cvp, 0, sizeof (cvp));

	dtd = xmlParseDTD (NULL, dtd_path);

	ret_val = xmlValidateDtd (&cvp, doc, dtd);

	xmlFreeDtd (dtd);

        return ret_val;
}

G_MODULE_EXPORT void
init (MrpFileModule *module, MrpApplication *application)
{
        MrpFileReader *reader;

        reader         = g_new0 (MrpFileReader, 1);
        reader->module = module;
        reader->priv   = NULL;

	reader->read_string = xml_read_string;

        imrp_application_register_reader (application, reader);
}
