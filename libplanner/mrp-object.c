/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2003-2004 Imendio HB
 * Copyright (C) 2001-2003 CodeFactory AB
 * Copyright (C) 2001-2003 Richard Hult <richard@imendio.com>
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2004      Alvaro del Castillo <acs@barrapunto.com>
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
#include <gobject/gvaluecollector.h>
#include <string.h>

#include "mrp-marshal.h"
#include "mrp-project.h"
#include "mrp-application.h"
#include "mrp-private.h"
#include "mrp-object.h"

struct _MrpObjectPriv {
	MrpProject *project;
	guint       id;
	GHashTable *property_hash;
};

/* Signals */
enum {
	REMOVED,
	PROP_CHANGED,
	LAST_SIGNAL
};

/* Properties */
enum {
	PROP_0,
	PROP_PROJECT
};


static GObjectClass *parent_class;
static guint         signals[LAST_SIGNAL];


static void object_class_init          (MrpObjectClass      *klass);
static void object_init                (MrpObject           *object);
static void object_finalize            (GObject             *g_object);
static void object_set_g_property      (GObject             *g_object,
					guint                prop_id,
					const GValue        *value,
					GParamSpec          *pspec);
static void object_get_g_property      (GObject             *g_object,
					guint                prop_id,
					GValue              *value,
					GParamSpec          *pspec);
static void object_property_removed_cb (MrpProject          *project,
					MrpProperty         *property,
					MrpObject           *object);


GType
mrp_object_get_type (void)
{
	static GType object_type = 0;

	if (!object_type) {
		static const GTypeInfo object_info = {
			sizeof (MrpObjectClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) object_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (MrpObject),
			0,              /* n_preallocs */
			(GInstanceInitFunc) object_init,
		};

		object_type = g_type_register_static (G_TYPE_OBJECT, 
                                                      "MrpObject", 
                                                      &object_info, 0);
	}

	return object_type;
}

static void
object_class_init (MrpObjectClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        
        parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
        
        object_class->finalize     = object_finalize;
        object_class->set_property = object_set_g_property;
        object_class->get_property = object_get_g_property;

	klass->removed             = NULL;

	/* Signals */
	signals[REMOVED] =
		g_signal_new ("removed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MrpObjectClass, removed),
			      NULL, NULL,
			      mrp_marshal_VOID__VOID,
			      G_TYPE_NONE, 
			      0);

	signals[PROP_CHANGED] =
		g_signal_new ("prop_changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
			      G_STRUCT_OFFSET (MrpObjectClass, prop_changed),
			      NULL, NULL,
			      mrp_marshal_VOID__POINTER_POINTER,
			      G_TYPE_NONE,
			      2, G_TYPE_POINTER, G_TYPE_VALUE);

	/* Properties */
	g_object_class_install_property (
		object_class,
		PROP_PROJECT,
		g_param_spec_object ("project",
				     "Project",
				     "The project this object belongs to",
				     MRP_TYPE_PROJECT,
				     G_PARAM_READWRITE));
}

static void
object_init (MrpObject *object)
{
        MrpObjectPriv *priv;
        
        priv = g_new0 (MrpObjectPriv, 1);
        object->priv = priv;

	priv->id = mrp_application_get_unique_id ();
	imrp_application_id_set_data (object, priv->id);

        priv->property_hash = g_hash_table_new (NULL, NULL);
}

static void
object_finalize (GObject *g_object)
{
        MrpObject     *object = MRP_OBJECT (g_object);
        MrpObjectPriv *priv;
        
        priv = object->priv;

        if (G_OBJECT_CLASS (parent_class)->finalize) {
                (* G_OBJECT_CLASS (parent_class)->finalize) (g_object);
        }
}

/**
 * mrp_object_set_property:
 * @object: an #MrpObject
 * @property: the property to set
 * @value: the value to set
 * 
 * Sets a custom property. This is mostly for language bindings. C programmers
 * should use mrp_object_set instead.
 **/
void
mrp_object_set_property (MrpObject *object, MrpProperty *property, GValue *value)
{
	MrpObjectPriv *priv;
	GValue        *value_cpy;
	GParamSpec    *pspec;
	GValue        *tmp_value;

	priv  = object->priv;
	pspec = G_PARAM_SPEC (property);
	
	value_cpy = g_new0 (GValue, 1);

	g_value_init (value_cpy, G_PARAM_SPEC_VALUE_TYPE (pspec));

	g_value_copy (value, value_cpy);
		
	tmp_value = g_hash_table_lookup (priv->property_hash,
					 property);

	if (tmp_value) {
		g_hash_table_steal (priv->property_hash, property);
		g_value_unset (tmp_value);
	} else {
		mrp_property_ref (property);
	}
		
	g_hash_table_insert (priv->property_hash, 
			     property, value_cpy);

	g_signal_emit (object, signals[PROP_CHANGED], 
		       g_quark_from_string (G_PARAM_SPEC (property)->name),
		       property, value);

	mrp_object_changed (object);
}

/**
 * mrp_object_get_property:
 * @object: an #MrpObject
 * @property: the property to set
 * @value: the value to set
 * 
 * Gets a custom property. This is mostly for language bindings. C programmers
 * should use mrp_object_get instead.
 **/
void
mrp_object_get_property (MrpObject *object, MrpProperty *property, GValue *value)
{
	MrpObjectPriv *priv;
	GValue        *tmp_value;
	
	priv = object->priv;

	tmp_value = g_hash_table_lookup (priv->property_hash,
					 property);

	if (!tmp_value) {
		g_param_value_set_default (G_PARAM_SPEC (property),
					   value);
		return;
	}

	g_value_copy (tmp_value, value);
}

static void
object_set_g_property (GObject        *g_object,
		       guint           prop_id,
		       const GValue   *value,
		       GParamSpec     *pspec)
{
	MrpObject     *object;
	MrpObjectPriv *priv;
	
	g_return_if_fail (MRP_IS_OBJECT (g_object));
	
	object = MRP_OBJECT (g_object);
	priv   = object->priv;

	switch (prop_id) {
	case PROP_PROJECT:
		if (priv->project) {
			g_signal_handlers_disconnect_by_func (priv->project,
							      G_CALLBACK (object_property_removed_cb),
							      object);
			g_object_unref (priv->project);
		}

		priv->project = g_value_get_object (value);
		if (priv->project) {
			g_object_ref (priv->project);
			g_signal_connect_object (priv->project, 
						 "property_removed",
						 G_CALLBACK (object_property_removed_cb),
						 object,
						 G_CONNECT_AFTER);
		}
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
object_get_g_property (GObject      *g_object,
		       guint         prop_id,
		       GValue       *value,
		       GParamSpec   *pspec)
{
	MrpObject     *object;
	MrpObjectPriv *priv;

	g_return_if_fail (MRP_IS_OBJECT (g_object));
	
	object = MRP_OBJECT (g_object);
	priv   = object->priv;
	
	switch (prop_id) {
	case PROP_PROJECT:
		g_value_set_object (value, priv->project);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
object_property_removed_cb (MrpProject  *project,
			    MrpProperty *property,
			    MrpObject   *object)
{
	MrpObjectPriv *priv;
	GValue        *value;

	g_return_if_fail (MRP_IS_PROJECT (project));
	g_return_if_fail (property != NULL);
	g_return_if_fail (MRP_IS_OBJECT (object));

	priv = object->priv;

	value = g_hash_table_lookup (priv->property_hash, property);

	if (value) {
		g_hash_table_steal (priv->property_hash, property);
		g_value_unset (value);
		g_free (value);
		mrp_property_unref (property);
	}
}

/**
 * mrp_object_removed:
 * @object: an #MrpObject
 *
 * Emits the signal %removed. This means that any references to the object
 * should be dropped, since the object is removed from the project.
 **/
void
mrp_object_removed (MrpObject *object)
{
	g_return_if_fail (MRP_IS_OBJECT (object));
	
	g_signal_emit (object, signals[REMOVED], 0);
}

/**
 * mrp_object_changed:
 * @object: an #MrpObject
 *
 * Emits the signal needs-saving on the project that this object belongs to,
 * indicating that the project has unsaved changes.
 **/
void
mrp_object_changed (MrpObject *object)
{
	MrpObjectPriv *priv;
	
	g_return_if_fail (MRP_IS_OBJECT (object));
	
	priv = object->priv;

	if (priv->project) {
		imrp_project_set_needs_saving (priv->project, TRUE);
	}
}

/**
 * mrp_object_set:
 * @object: an #MrpObject
 * @first_property_name: the name of the first property to set
 * @...: first value to set, followed by additional name/value pairs to set, NULL terminated
 *
 * #MrpObject can have custom properties, which are added runtime, for example
 * by a user or plugin. mrp_object_set() allows you to set these the value of
 * those properties. It also handles the regular #GObject properties, so you can
 * use it to set both custom properties and regular properties at the same time.
 **/
void
mrp_object_set (gpointer     pobject,
		const gchar *first_property_name,
		...)
{  
	MrpObject *object = MRP_OBJECT (pobject);
	va_list    var_args;
  
	g_return_if_fail (MRP_IS_OBJECT (object));
  
	va_start (var_args, first_property_name);
	mrp_object_set_valist (object, first_property_name, var_args);
	va_end (var_args);
}

/**
 * mrp_object_get:
 * @object: an #MrpProject
 * @first_property_name: the name of the first property to get
 * @...: first value to get, followed by additional name/value pairs to get, NULL terminated
 * 
 * Retrieves the values of a variable number of custom properties or regular
 * properties from an object. See mrp_object_set().
 **/
void
mrp_object_get (gpointer     pobject,
		const gchar *first_property_name,
		...)
{
	MrpObject *object = MRP_OBJECT (pobject);
	va_list    var_args;
  
	g_return_if_fail (MRP_IS_OBJECT (object));
	
	va_start (var_args, first_property_name);
	mrp_object_get_valist (object, first_property_name, var_args);
	va_end (var_args);
}

/**
 * mrp_object_set_valist:
 * @object: an #MrpObject
 * @first_property_name: the name of the first property to set
 * @var_args: va_list of arguments
 *
 * va_list version of mrp_object_set().
 **/
void
mrp_object_set_valist (MrpObject   *object,
		       const gchar *first_property_name,
		       va_list      var_args)
{
	MrpObjectPriv *priv;
	const gchar   *name;
  
	g_return_if_fail (MRP_IS_OBJECT (object));

	priv = object->priv;
	
	g_object_ref (object);
  
	name = first_property_name;

	while (name) {
		GValue       value = { 0, };
		GParamSpec  *pspec;
		gchar       *error = NULL;
		
		pspec = g_object_class_find_property (
			G_OBJECT_GET_CLASS (object), name);

		if (pspec) {
			/* Normal g_object property */
			g_value_init (&value, 
				      G_PARAM_SPEC_VALUE_TYPE (pspec));
			
			G_VALUE_COLLECT (&value, var_args, 0, &error);
			g_object_set_property (G_OBJECT (object),
					       name,
					       &value);
		} else {
			/* Our custom properties */
			pspec = G_PARAM_SPEC (
				mrp_project_get_property (priv->project, 
							  name,
							  G_OBJECT_TYPE (object)));
			
			if (!pspec) {
				g_warning ("%s: object class `%s' has no property named `%s'",
					   G_STRLOC,
					   G_OBJECT_TYPE_NAME (object),
					   name);
				break;
			}
			
			if (!(pspec->flags & G_PARAM_WRITABLE)) {
				g_warning ("%s: property `%s' of object class `%s' is not writable",
					   G_STRLOC,
					   pspec->name,
					   G_OBJECT_TYPE_NAME (object));
				break;
			}
      
			g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
			
			G_VALUE_COLLECT (&value, var_args, 0, &error);
		}

		if (error) {
			g_warning ("%s: %s", G_STRLOC, error);
			g_free (error);
	  
			/* we purposely leak the value here, it might not be
			 * in a sane state if an error condition occoured
			 */
			break;
		}

		mrp_object_set_property (object, MRP_PROPERTY (pspec), &value);
		
		/* set the property value */
		g_value_unset (&value);
      
		name = va_arg (var_args, gchar*);
	}

	g_object_unref (object);
}

/**
 * mrp_object_get_valist:
 * @object: an #MrpObject
 * @first_property_name: the name of the first property to get
 * @var_args: va_list of arguments
 *
 * va_list version of mrp_object_get().
 **/
void
mrp_object_get_valist (MrpObject   *object,
		       const gchar *first_property_name,
		       va_list      var_args)
{
	MrpObjectPriv *priv;
	const gchar   *name;
  
	g_return_if_fail (MRP_IS_OBJECT (object));
  
	priv = object->priv;
	
	g_object_ref (object);
  
	name = first_property_name;
  
	while (name) {
		GValue      value = { 0, };
		GParamSpec *pspec;
		gchar      *error;

		pspec = g_object_class_find_property (
			G_OBJECT_GET_CLASS (object),
			name);
		
		if (pspec) {
			g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));

			g_object_get_property (G_OBJECT (object),
					       name,
					       &value);
		} else {
			pspec = mrp_project_get_property (priv->project,
							  name,
							  G_OBJECT_TYPE (object));
			
			if (!pspec) {
				break;
			}

			if (!(pspec->flags & G_PARAM_READABLE))	{
				g_warning ("%s: property `%s' of object class `%s' is not readable",
					   G_STRLOC,
					   pspec->name,
					   G_OBJECT_TYPE_NAME (object));
				break;
			}
			
			g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
			
			mrp_object_get_property (object,
						 MRP_PROPERTY (pspec), 
						 &value);
		}
      
		G_VALUE_LCOPY (&value, var_args, 0, &error);

		if (error) {
			g_warning ("%s: %s", G_STRLOC, error);
			g_free (error);
			g_value_unset (&value);
			break;
		}
      
		g_value_unset (&value);
		
		name = va_arg (var_args, gchar *);
	}
	
	g_object_unref (object);
}

/**
 * mrp_object_get_properties:
 * @object: an #MrpObject
 * 
 * Retrieves the list of custom properties for the type of @object.
 * 
 * Return value: A list of #MrpProperty, must not be changed or freed.
 **/
GList *
mrp_object_get_properties (MrpObject *object)
{
	MrpObjectPriv *priv;

	g_return_val_if_fail (MRP_IS_OBJECT (object), NULL);

	priv = object->priv;
	
	return mrp_project_get_properties_from_type (priv->project, 
						     G_OBJECT_TYPE (object));
}

/**
 * mrp_object_get_id:
 * @object: an #MrpObject
 * 
 * Retrieves the unique object id in the application
 * 
 * Return value: 0 if fails, object id if everything is ok.
 **/
guint
mrp_object_get_id (MrpObject *object)
{
	MrpObjectPriv *priv;
	
	g_return_val_if_fail (MRP_IS_OBJECT (object), 0);
	
	priv = object->priv;

	return priv->id;
}


/**
 * mrp_object_set_id:
 * @object: an #MrpObject
 * 
 * Change the unique object id in the application.
 * This function must be called only from Undo/Redo operations. 
 * 
 * Return value: FALSE if fails, TRUE is everything is ok.
 **/
gboolean
mrp_object_set_id (MrpObject *object,
		   guint      id)
{
	MrpObjectPriv *priv;

	g_return_val_if_fail (MRP_IS_OBJECT (object), FALSE);

	priv = object->priv;
	
	if (imrp_application_id_set_data (object, id)) {		
		priv->id = id;
		return TRUE;
	} else {
		return FALSE;
	}
}

gpointer
mrp_object_get_project (MrpObject *object)
{
	MrpObjectPriv *priv;

	g_return_val_if_fail (MRP_IS_OBJECT (object), FALSE);

	priv = object->priv;

	return priv->project;
}
