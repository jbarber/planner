/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <richard@imendio.com>
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

#include <stdlib.h>

#include "mrp-intl.h"
#include "mrp-file-module.h"
#include "mrp-private.h"
#include "mrp-application.h"

struct _MrpApplicationPriv {
	GList *file_readers;
	GList *file_writers;
};

static void application_class_init        (MrpApplicationClass    *klass);
static void application_init              (MrpApplication         *app);
static void application_finalize          (GObject                *object);

static void application_init_gettext      (void);
static void application_init_file_modules (MrpApplication         *app);

static GObjectClass *parent_class;

GType
mrp_application_get_type (void)
{
	static GType type = 0;

	if (!type) {
		static const GTypeInfo info = {
			sizeof (MrpApplicationClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) application_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (MrpApplication),
			0,              /* n_preallocs */
			(GInstanceInitFunc) application_init,
		};

		type = g_type_register_static (G_TYPE_OBJECT, 
					       "MrpApplication", 
					       &info, 0);
	}

	return type;
}

static void
application_class_init (MrpApplicationClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
	
	object_class->finalize = application_finalize;
}

static void
application_init (MrpApplication *app)
{
	MrpApplicationPriv *priv;
	static gboolean     first = TRUE;

	if (!first) {
		g_error ("You can only create one instance of MrpApplication");
		exit (1);
	}

	priv = g_new0 (MrpApplicationPriv, 1);
	priv->file_readers = NULL;
	priv->file_writers = NULL;

	app->priv = priv;

	application_init_gettext ();
	application_init_file_modules (app);
	
	first = FALSE;
}

static void
application_finalize (GObject *object)
{
	/* FIXME: free stuff. */

	if (parent_class->finalize) {
		parent_class->finalize (object);
	}
}

static void
application_init_gettext (void)
{
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	imrp_time_init ();
}

static void
application_init_file_modules (MrpApplication *app)
{
 	mrp_file_module_load_all (app);
}

void
imrp_application_register_reader (MrpApplication *app, MrpFileReader *reader)
{
	MrpApplicationPriv *priv;

	g_return_if_fail (MRP_IS_APPLICATION (app));
	g_return_if_fail (reader != NULL);
	
	priv = app->priv;
	
	priv->file_readers = g_list_prepend (priv->file_readers, reader);
}

void
imrp_application_register_writer (MrpApplication *app, MrpFileWriter *writer)
{
	MrpApplicationPriv *priv;

	g_return_if_fail (MRP_IS_APPLICATION (app));
	g_return_if_fail (writer != NULL);
	
	priv = app->priv;
	
	priv->file_writers = g_list_prepend (priv->file_writers, writer);
}

GList *         
imrp_application_get_all_file_readers (MrpApplication *app)
{
	MrpApplicationPriv *priv;

	g_return_val_if_fail (MRP_IS_APPLICATION (app), NULL);
	
	priv = app->priv;

	return priv->file_readers;
}

GList *         
imrp_application_get_all_file_writers (MrpApplication *app)
{
	MrpApplicationPriv *priv;

	g_return_val_if_fail (MRP_IS_APPLICATION (app), NULL);
	
	priv = app->priv;

	return priv->file_writers;
}

/**
 * mrp_application_new:
 * 
 * Creates a new #MrpApplication.
 * 
 * Return value: the newly created application
 **/
MrpApplication *
mrp_application_new (void)
{
	return g_object_new (MRP_TYPE_APPLICATION, NULL);
}

