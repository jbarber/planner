/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nill; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2005 Imendio AB
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
#include <glib/gi18n.h>
#include <gmodule.h>
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libexslt/exslt.h>
#include <string.h>
#include <libxslt/extensions.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libplanner/mrp-file-module.h>
#include <libplanner/mrp-private.h>
#include "libplanner/mrp-paths.h"
#include <libplanner/mrp-time.h>

void            init                     (MrpFileModule   *module,
					  MrpApplication  *application);
static gboolean html_write               (MrpFileWriter   *writer,
					  MrpProject      *project,
					  const gchar     *uri,
					  gboolean         force,
					  GError         **error);
static gboolean xml_planner_pre012_write (MrpFileWriter   *writer,
					  MrpProject      *project,
					  const gchar     *uri,
					  gboolean         force,
					  GError         **error);


static void xslt_module_gettext		(xmlXPathParserContextPtr ctxt,
					int argc);

static void xslt_module_getdate		(xmlXPathParserContextPtr ctxt,
					int argc);

static void xslt_module_shutdown	(xsltTransformContextPtr ctxt,
					const xmlChar *uri,
					void *data);

static void * xslt_module_init		(xsltTransformContextPtr ctxt,
					const xmlChar *uri);

extern mrptime    			mrp_time_from_tm (struct tm *tm);


void
xslt_module_gettext (xmlXPathParserContextPtr ctxt, int argc)
{
	xmlXPathObjectPtr name = valuePop(ctxt);
	name = xmlXPathConvertString(name);
	valuePush (ctxt, xmlXPathNewString ((const xmlChar *) gettext (name->stringval)));
}

void
xslt_module_getdate (xmlXPathParserContextPtr ctxt, int argc)
{
	gchar *svalue;
	struct tm ts;
	mrptime time;

	xmlXPathObjectPtr name = valuePop (ctxt);
	time_t t = name->floatval;

	ts = *localtime(&t);
	time = mrp_time_from_tm (&ts);

	/* Translators: Example output: October 9, 2006 */
	svalue = mrp_time_format (gettext("%B %e, %Y"), time);
	valuePush (ctxt, xmlXPathNewString ((const xmlChar *)svalue));
	g_free (svalue);
}

void
xslt_module_shutdown (xsltTransformContextPtr ctxt, const xmlChar *uri, void *data)
{
}

void  *
xslt_module_init (xsltTransformContextPtr ctxt, const xmlChar *uri)
{
	xsltRegisterExtFunction (ctxt, (const xmlChar *)"gettext", uri,
		xslt_module_gettext);

	xsltRegisterExtFunction (ctxt, (const xmlChar *)"getdate", uri,
		xslt_module_getdate);

	return NULL;
}


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
	gboolean        ret;
	gchar          *filename;

	if (!mrp_project_save_to_xml (project, &xml_project, error)) {
		return FALSE;
	}

        /* libxml housekeeping */
        xmlSubstituteEntitiesDefault (1);
        xmlLoadExtDtdDefaultValue = 1;
        exsltRegisterAll ();

	xsltRegisterExtModule((const xmlChar *)"http://www.gnu.org/software/gettext/",
                                 xslt_module_init, xslt_module_shutdown);

	filename = mrp_paths_get_stylesheet_dir ("planner2html.xsl");
        stylesheet = xsltParseStylesheetFile (filename);
	g_free (filename);

        doc = xmlParseMemory (xml_project, strlen (xml_project));
        final_doc = xsltApplyStylesheet (stylesheet, doc, NULL);
        xmlFree (doc);

	ret = TRUE;

	if (!final_doc ||
	    xsltSaveResultToFilename (uri, final_doc, stylesheet, 0) == -1) {
		g_set_error (error,
			     MRP_ERROR,
			     MRP_ERROR_EXPORT_FAILED,
			     _("Export to HTML failed"));
		ret = FALSE;
	}

	xsltFreeStylesheet (stylesheet);
        xmlFree (final_doc);

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
	gboolean        ret;
        gchar          *filename;

	if (!mrp_project_save_to_xml (project, &xml_project, error)) {
		return FALSE;
	}

        /* libxml housekeeping */
        xmlSubstituteEntitiesDefault(1);
        xmlLoadExtDtdDefaultValue = 1;
        exsltRegisterAll ();

	filename = mrp_paths_get_stylesheet_dir ("planner2plannerv011.xsl");
        stylesheet = xsltParseStylesheetFile (filename);
	g_free (filename);

        doc = xmlParseMemory (xml_project, strlen (xml_project));
        final_doc = xsltApplyStylesheet (stylesheet, doc, NULL);
        xmlFree (doc);

	ret = TRUE;

	if (!final_doc ||
	    xsltSaveResultToFilename (uri, final_doc, stylesheet, 0) == -1) {
		g_set_error (error,
			     MRP_ERROR,
			     MRP_ERROR_EXPORT_FAILED,
			     _("Export to HTML failed"));
		ret = FALSE;
	}

	xsltFreeStylesheet (stylesheet);
        xmlFree (final_doc);

	return ret;
}

G_MODULE_EXPORT void
init (MrpFileModule *module, MrpApplication *application)
{
        MrpFileWriter *writer;

        writer = g_new0 (MrpFileWriter, 1);

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

