/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002 CodeFactory AB
 * Copyright (C) 2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2002 Mikael Hallendal <micke@imendio.com>
 * Copyright (C) 2002 Alvaro del Castillo <acs@barrapunto.com>
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

#include "mrp-private.h"
#include <glib/gi18n.h>
#include "mrp-project.h"
#include "mrp-property.h"

/* Quarks */
#define LABEL        "label"
#define PROJECT      "project"
#define DESCRIPTION  "description"
#define TYPE         "type"
#define USER_DEFINED "user_defined"

static void         property_set_type               (MrpProperty     *property,
						     MrpPropertyType  type);
static GParamSpec * property_param_spec_string_list (const gchar     *name,
						     const gchar     *nick,
						     const gchar     *blurb,
						     GParamFlags      flags);


static void
property_set_type (MrpProperty *property, MrpPropertyType type)
{
	g_param_spec_set_qdata (G_PARAM_SPEC (property),
				g_quark_from_static_string (TYPE),
				GINT_TO_POINTER (type));
}

void
imrp_property_set_project (MrpProperty *property, MrpProject *project)
{
	g_param_spec_set_qdata (G_PARAM_SPEC (property),
				g_quark_from_static_string (PROJECT),
				project);
}

/**
 * mrp_property_new:
 * @name: the name of the property
 * @type: an #MrpPropertyType
 * @label: the human readable label
 * @description: a string describing the property
 * @user_defined: a #gboolean
 *
 * Creates a new #MrpProperty. @name must be unique in the application.
 * @user_defined specifies if the property was created by a user or a plugin
 * or Planner itself.
 *
 * Return value: a newly create property
 **/
MrpProperty *
mrp_property_new (const gchar     *name,
		  MrpPropertyType  type,
		  const gchar     *label,
		  const gchar     *description,
		  gboolean         user_defined)
{
	MrpProperty *property = NULL;

	switch (type) {
	case MRP_PROPERTY_TYPE_INT:
		property = MRP_PROPERTY (g_param_spec_int (name,
							   "",
							   "",
							   G_MININT,
							   G_MAXINT,
							   0,
							   G_PARAM_READWRITE));
		break;
	case MRP_PROPERTY_TYPE_FLOAT:
		property = MRP_PROPERTY (g_param_spec_float (name,
							     "",
							     "",
							     -G_MAXFLOAT,
							     G_MAXFLOAT,
							     0.0,
							     G_PARAM_READWRITE));
		break;
	case MRP_PROPERTY_TYPE_DURATION:
		property = MRP_PROPERTY (g_param_spec_int (name,
							     "",
							     "",
							     G_MININT,
							     G_MAXINT,
							     0,
							     G_PARAM_READWRITE));
		break;
	case MRP_PROPERTY_TYPE_STRING:
		property = MRP_PROPERTY (g_param_spec_string (name,
							      "",
							      "",
							      NULL,
							      G_PARAM_READWRITE));
		break;
	case MRP_PROPERTY_TYPE_STRING_LIST:
		property = MRP_PROPERTY (property_param_spec_string_list (name,
									  "",
									  "",
									  G_PARAM_READWRITE));
		break;
	case MRP_PROPERTY_TYPE_DATE:
		property = MRP_PROPERTY (mrp_param_spec_time (name,
							      "",
							      "",
							      G_PARAM_READWRITE));
		break;
	case MRP_PROPERTY_TYPE_COST:
		property = MRP_PROPERTY (g_param_spec_float (name,
							     "",
							     "",
							     -G_MAXFLOAT,
							     G_MAXFLOAT,
							     0.0,
							     G_PARAM_READWRITE));
		break;
	default:
		break;
	};

	if (!property) {
		return NULL;
	}

	property_set_type (property, type);
	g_param_spec_set_qdata_full (G_PARAM_SPEC (property),
				     g_quark_from_static_string (LABEL),
				     g_strdup (label),
				     g_free);
	g_param_spec_set_qdata_full (G_PARAM_SPEC (property),
				     g_quark_from_static_string (DESCRIPTION),
				     g_strdup (description),
				     g_free);
	g_param_spec_set_qdata_full (G_PARAM_SPEC (property),
				     g_quark_from_static_string (USER_DEFINED),
				     GINT_TO_POINTER (user_defined),
				     NULL);
	return property;
}

/**
 * mrp_property_get_name:
 * @property: an #MrpProperty
 *
 * Fetches the name of @property
 *
 * Return value: the name of @property
 **/
const gchar *
mrp_property_get_name (MrpProperty *property)
{
	g_return_val_if_fail (property != NULL, NULL);

	return G_PARAM_SPEC (property)->name;
}

/**
 * mrp_property_get_property_type:
 * @property: an #MrpProperty
 *
 * Fetches the type of @property
 *
 * Return value: the type of @property
 **/
MrpPropertyType
mrp_property_get_property_type (MrpProperty *property)
{
	g_return_val_if_fail (property != NULL, MRP_PROPERTY_TYPE_NONE);

	return GPOINTER_TO_INT (
		g_param_spec_get_qdata (G_PARAM_SPEC (property),
					g_quark_from_static_string (TYPE)));
}

/**
 * mrp_property_set_label:
 * @property: an #MrpProperty
 * @label: a string containing the new label
 *
 * Sets the label of @property and signals the "property-changed" signal on
 * the project @property is attached to.
 **/
void
mrp_property_set_label (MrpProperty *property, const gchar *label)
{
	gpointer project;

	g_param_spec_set_qdata_full (G_PARAM_SPEC (property),
				     g_quark_from_static_string (LABEL),
				     g_strdup (label),
				     g_free);

	project = g_param_spec_get_qdata (G_PARAM_SPEC (property),
					  g_quark_from_static_string (PROJECT));
	if (project) {
		imrp_project_property_changed (MRP_PROJECT (project),
					       property);
	}
}

/**
 * mrp_property_get_label:
 * @property: an #MrpProperty
 *
 * Fetches the label of @property
 *
 * Return value: the label of @property
 **/
const gchar *
mrp_property_get_label (MrpProperty *property)
{
	g_return_val_if_fail (property != NULL, NULL);

	return ((const gchar *)
		g_param_spec_get_qdata (G_PARAM_SPEC (property),
					g_quark_from_static_string (LABEL)));
}

/**
 * mrp_property_set_description:
 * @property: an #MrpProperty
 * @description: a string containing the new description
 *
 * Sets the description of @property and signals the "property-changed" signal on the project @property is attached to.
 **/
void
mrp_property_set_description (MrpProperty *property, const gchar *description)
{
	gpointer project;

	g_param_spec_set_qdata_full (G_PARAM_SPEC (property),
				     g_quark_from_static_string (DESCRIPTION),
				     g_strdup (description),
				     g_free);

	project = g_param_spec_get_qdata (G_PARAM_SPEC (property),
					  g_quark_from_static_string (PROJECT));
	if (project) {
		imrp_project_property_changed (MRP_PROJECT (project),
					       property);
	}
}

/**
 * mrp_property_get_description:
 * @property: an #MrpProperty
 *
 * Fetches the description of @property
 *
 * Return value: the description of @property
 **/
const gchar *
mrp_property_get_description (MrpProperty *property)
{
	g_return_val_if_fail (property != NULL, NULL);

	return ((const gchar *)
		g_param_spec_get_qdata (G_PARAM_SPEC (property),
					g_quark_from_static_string (DESCRIPTION)));
}

/**
 * mrp_property_set_user_defined:
 * @property: an #MrpProperty
 * @user_defined: if the property is user defined
 *
 * Sets if @property is user-defined or created by a plugin or Planner
 * itself.
 **/
void
mrp_property_set_user_defined (MrpProperty *property, gboolean user_defined)
{
	g_param_spec_set_qdata_full (G_PARAM_SPEC (property),
				     g_quark_from_static_string (USER_DEFINED),
				     GINT_TO_POINTER (user_defined),
				     NULL);
}

/**
 * mrp_property_get_user_defined:
 * @property: an #MrpProperty
 *
 * Fetches if @property is uesr defined or not.
 *
 * Return value: %TRUE if @property is user defined, otherwise %FALSE
 **/
gboolean
mrp_property_get_user_defined (MrpProperty *property)
{
	return ((gboolean)
		GPOINTER_TO_INT (g_param_spec_get_qdata (G_PARAM_SPEC (property),
							 g_quark_from_static_string (USER_DEFINED))));
}

/**
 * mrp_property_ref:
 * @property: an #MrpProperty
 *
 * Add a reference to @property. User should call this when storing a reference
 * to @property.
 *
 * Return value: the property
 **/
MrpProperty *
mrp_property_ref (MrpProperty *property)
{
	g_return_val_if_fail (property != NULL, NULL);

	return MRP_PROPERTY (g_param_spec_ref (G_PARAM_SPEC (property)));
}

/**
 * mrp_property_unref:
 * @property: an #MrpProperty
 *
 * Remove a reference from @property. If the reference count reaches 0 the
 * property will be freed. User should not use it's reference after calling
 * mrp_property_unref().
 **/
void
mrp_property_unref (MrpProperty *property)
{
	g_return_if_fail (property != NULL);

	g_param_spec_unref (G_PARAM_SPEC (property));
}

/**
 * mrp_property_type_as_string:
 * @type: an #MrpPropertyType
 *
 * Transform a #MrpPropertyTYpe into a human readable string.
 *
 * Return value: a string representation of @type
 **/
const gchar *
mrp_property_type_as_string (MrpPropertyType type)
{
	switch (type) {
	case MRP_PROPERTY_TYPE_STRING:
		return _("Text");
	case MRP_PROPERTY_TYPE_STRING_LIST:
		return _("String list");
	case MRP_PROPERTY_TYPE_INT:
		return _("Integer number");
	case MRP_PROPERTY_TYPE_FLOAT:
		return _("Floating-point number");
	case MRP_PROPERTY_TYPE_DATE:
		return _("Date");
	case MRP_PROPERTY_TYPE_DURATION:
		return _("Duration");
	case MRP_PROPERTY_TYPE_COST:
		return _("Cost");
	case MRP_PROPERTY_TYPE_NONE:
		g_warning ("Requested name for type 'none'.");
		return _("None");
	default:
		g_assert_not_reached ();
	}

	return NULL;
}

static GParamSpec *
property_param_spec_string_list (const gchar *name,
				 const gchar *nick,
				 const gchar *blurb,
				 GParamFlags flags)
{
	GParamSpec *spec;

	spec = g_param_spec_string (name,
				    nick,
				    blurb,
				    NULL,
				    flags);

	return g_param_spec_value_array (name,
					 nick,
					 blurb,
					 spec,
					 flags);
}

/* Boxed types. */

GType
mrp_property_get_type (void)
{
	static GType our_type = 0;

	if (our_type == 0) {
		our_type = g_boxed_type_register_static ("MrpProperty",
							 (GBoxedCopyFunc) g_param_spec_ref,
							 (GBoxedFreeFunc) g_param_spec_unref);
	}

	return our_type;
}

