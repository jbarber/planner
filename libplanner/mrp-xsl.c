/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nill; c-basic-offset: 8 -*- */
/*
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
#include <glib.h>
#include <gmodule.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libexslt/exslt.h>
#include <string.h>
#include <libplanner/mrp-file-module.h>
#include <libplanner/mrp-private.h>
#include <libplanner/mrp-intl.h>

void              init          (MrpFileModule    *module,
				 MrpApplication   *application);
static gboolean   html_write    (MrpFileWriter    *writer,
				 MrpProject       *project,
				 const gchar      *uri,
				 gboolean          force,
				 GError          **error);

static gboolean   xml_planner_pre012_write    (MrpFileWriter    *writer,
				 MrpProject       *project,
				 const gchar      *uri,
				 gboolean          force,
				 GError          **error);
				 
static gboolean
html_write (MrpFileWriter  *writer,
	    MrpProject     *project,
	    const gchar    *uri,
	    gboolean        force,
	    GError        **error)
{
        gchar          *xml_project;
        xsltStylesheet *stylesheet;
        xmlDoc         *doc;
        xmlDoc         *final_doc;
	GnomeVFSHandle *handle;
	xmlChar        *buffer;
	gint            len;
	GnomeVFSResult  result;
	gboolean        ret;

	mrp_project_save_to_xml (project, &xml_project, NULL);

        /* libxml housekeeping */
        xmlSubstituteEntitiesDefault(1);
        xmlLoadExtDtdDefaultValue = 1;
        exsltRegisterAll ();

        stylesheet = xsltParseStylesheetFile (STYLESHEETDIR "/mrproject2html.xsl");

        doc = xmlParseMemory (xml_project, strlen (xml_project));
                                                                                
        final_doc = xsltApplyStylesheet (stylesheet, doc, NULL);
                                                                                
	ret = TRUE;

	if (xsltSaveResultToString (&buffer, &len, final_doc, stylesheet) != -1) {
		result = gnome_vfs_create (&handle, uri, GNOME_VFS_OPEN_WRITE,
					   FALSE, 0644);
		
		if (result == GNOME_VFS_OK) { 
			gnome_vfs_write (handle, buffer, (GnomeVFSFileSize) len, NULL);
			gnome_vfs_close (handle);
		} else {
		  	g_set_error (error,
				     MRP_ERROR,
				     MRP_ERROR_EXPORT_FAILED,
				     gnome_vfs_result_to_string (result));
			ret = FALSE;
		}
		
		xmlFree (buffer);
	} else {
		g_set_error (error,
			     MRP_ERROR,
			     MRP_ERROR_EXPORT_FAILED,
			     _("Export to HTML failed"));
		ret = FALSE;
	}
	
	xsltFreeStylesheet (stylesheet);
        xmlFree (final_doc);
        xmlFree (doc);

	return ret;
}

				 
static gboolean
xml_planner_pre012_write (MrpFileWriter  *writer,
	    MrpProject     *project,
	    const gchar    *uri,
	    gboolean        force,
	    GError        **error)
{
        gchar          *xml_project;
        xsltStylesheet *stylesheet;
        xmlDoc         *doc;
        xmlDoc         *final_doc;
	GnomeVFSHandle *handle;
	xmlChar        *buffer;
	gint            len;
	GnomeVFSResult  result;
	gboolean        ret;

	mrp_project_save_to_xml (project, &xml_project, NULL);

        /* libxml housekeeping */
        xmlSubstituteEntitiesDefault(1);
        xmlLoadExtDtdDefaultValue = 1;
        exsltRegisterAll ();

        stylesheet = xsltParseStylesheetFile (STYLESHEETDIR "/planner2plannerv011.xsl");

        doc = xmlParseMemory (xml_project, strlen (xml_project));
                                                                                
        final_doc = xsltApplyStylesheet (stylesheet, doc, NULL);
                                                                                
	ret = TRUE;

	if (xsltSaveResultToString (&buffer, &len, final_doc, stylesheet) != -1) {
		result = gnome_vfs_create (&handle, uri, GNOME_VFS_OPEN_WRITE,
					   FALSE, 0644);
		
		if (result == GNOME_VFS_OK) { 
			gnome_vfs_write (handle, buffer, (GnomeVFSFileSize) len, NULL);
			gnome_vfs_close (handle);
		} else {
		  	g_set_error (error,
				     MRP_ERROR,
				     MRP_ERROR_EXPORT_FAILED,
				     gnome_vfs_result_to_string (result));
			ret = FALSE;
		}
		
		xmlFree (buffer);
	} else {
		g_set_error (error,
			     MRP_ERROR,
			     MRP_ERROR_EXPORT_FAILED,
			     _("Export to Planner pre-0.12 format failed"));
		ret = FALSE;
	}
	
	xsltFreeStylesheet (stylesheet);
        xmlFree (final_doc);
        xmlFree (doc);

	return ret;
}


G_MODULE_EXPORT void
init (MrpFileModule *module, MrpApplication *application)
{
        MrpFileWriter *writer;
        
        writer             = g_new0 (MrpFileWriter, 1);
	
	/* The HTML writer registration */

        writer->module     = module;
	writer->identifier = "Planner HTML";
	writer->mime_type  = "text/html";
        writer->priv       = NULL;
	
        writer->write      = html_write;

        imrp_application_register_writer (application, writer);

	/* The older Planner/Mrproject writer registration */

	writer             = g_new0 (MrpFileWriter, 1);
	
	writer->module     = module;
	writer->identifier = "Planner XML pre-0.12";  /* Don't change unless you change plugin to match */
	writer->mime_type  = "text/xml";
        writer->priv       = NULL;
	
        writer->write      = xml_planner_pre012_write;

        imrp_application_register_writer (application, writer);
	
}

