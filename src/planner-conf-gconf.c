/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Imendio AB
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
#include <gconf/gconf-client.h>

#include "planner-conf.h"

#define GCONF_PREFIX "/apps/planner"

static GConfClient *  conf_get_gconf_client (void);
static gchar *        conf_get_key          (const gchar *key);

static GConfClient *
conf_get_gconf_client (void)
{
	static GConfClient *client = NULL;

	if (!client) {
		client = gconf_client_get_default ();
	}

	return client;
}

static gchar *
conf_get_key (const gchar *key)
{
	return g_strconcat (GCONF_PREFIX, key, NULL);
}

gboolean
planner_conf_dir_exists (const gchar *dir, GError **error)
{
	GConfClient *client;
	gchar       *full_dir;
	gboolean     ret_val;

	client = conf_get_gconf_client ();

	full_dir = conf_get_key (dir);
	ret_val = gconf_client_dir_exists (client, full_dir, error);
	g_free (full_dir);

	return ret_val;
}

gboolean
planner_conf_get_bool (const gchar *key, GError **error)
{
	GConfClient *client;
	gchar       *full_key;
	gboolean     ret_val;

	client = conf_get_gconf_client ();

	full_key = conf_get_key (key);
	ret_val = gconf_client_get_bool (client, full_key, error);
	g_free (full_key);

	return ret_val;
}

gchar *
planner_conf_get_string (const gchar *key, GError **error)
{
	GConfClient *client;
	gchar       *full_key;
	gchar       *ret_val;

	client = conf_get_gconf_client ();

	full_key = conf_get_key (key);
	ret_val = gconf_client_get_string (client, full_key, error);
	g_free (full_key);

	return ret_val;
}

gint
planner_conf_get_int (const gchar *key, GError **error)
{
	GConfClient *client;
	gchar       *full_key;
	gint         ret_val;

	client = conf_get_gconf_client ();

	full_key = conf_get_key (key);
	ret_val = gconf_client_get_int (client, full_key, error);
	g_free (full_key);

	return ret_val;
}

gboolean
planner_conf_set_bool (const gchar *key, gboolean value, GError **error)
{
	GConfClient *client;
	gchar       *full_key;
	gboolean     ret_val;

	client = conf_get_gconf_client ();

	full_key = conf_get_key (key);
	ret_val = gconf_client_set_bool (client, full_key, value, error);
	g_free (full_key);

	return ret_val;
}

gboolean
planner_conf_set_string (const gchar *key, const gchar *value, GError **error)
{
	GConfClient *client;
	gchar       *full_key;
	gboolean     ret_val;

	client = conf_get_gconf_client ();

	full_key = conf_get_key (key);
	ret_val = gconf_client_set_string (client, full_key, value, error);
	g_free (full_key);

	return ret_val;
}

gboolean
planner_conf_set_int (const gchar *key, gint value, GError **error)
{
	GConfClient *client;
	gchar       *full_key;
	gboolean     ret_val;

	client = conf_get_gconf_client ();

	full_key = conf_get_key (key);
	ret_val = gconf_client_set_int (client, full_key, value, error);
	g_free (full_key);

	return ret_val;
}

