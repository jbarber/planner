/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Jani Tiainen
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

#include "planner-conf.h"

#define WIN32_KEYNAME "Software\\Planner"

#include <windows.h>

gboolean
planner_conf_dir_exists (const gchar *dir, GError **error)
{
	gboolean     ret_val;
	HKEY hKey;
    
	ret_val = RegOpenKeyEx (HKEY_CURRENT_USER, WIN32_KEYNAME, 0, KEY_READ, &hKey);
	RegCloseKey (hKey);
	
	return ret_val == ERROR_SUCCESS;
}

static gboolean planner_conf_create()
{
	gboolean     ret_val;
	DWORD disp = 0;
	HKEY hKey;
	
	ret_val = RegOpenKeyEx (HKEY_CURRENT_USER, WIN32_KEYNAME, 0, KEY_READ, &hKey);
	RegCloseKey (hKey);
	if(ret_val == ERROR_SUCCESS)
		return TRUE;

	ret_val = RegCreateKeyEx (HKEY_CURRENT_USER, WIN32_KEYNAME, 0, "",
				 REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
				 &hKey,
				 &disp);
	RegCloseKey (hKey);

	return ret_val == ERROR_SUCCESS;
}

gboolean
planner_conf_get_bool (const gchar *key, GError **error)
{
	gboolean     ret_val = FALSE;
	gchar *keyBuffer;
	glong keyLen;
	HKEY hKey;
	
	RegOpenKeyEx (HKEY_CURRENT_USER, WIN32_KEYNAME, 0, KEY_READ, &hKey);
	if (RegQueryValueEx (hKey, key, 0, NULL, NULL,  &keyLen) == ERROR_SUCCESS) {
		keyBuffer = g_malloc (keyLen + 1);
		RegQueryValueEx (hKey, key, 0, NULL, keyBuffer,  &keyLen);
		RegCloseKey (hKey);

		ret_val = g_ascii_strncasecmp (keyBuffer, "TRUE", 4) == 0 ? TRUE : FALSE;
		g_free (keyBuffer);
	}

	return ret_val;
}

gchar *
planner_conf_get_string (const gchar *key, GError **error)
{
	gchar *keyBuffer = NULL;
	glong keyLen;
	HKEY hKey;

	RegOpenKeyEx (HKEY_CURRENT_USER, WIN32_KEYNAME, 0, KEY_READ, &hKey);
	if(RegQueryValueEx (hKey, key, 0, NULL, NULL,  &keyLen) == ERROR_SUCCESS) {
		keyBuffer = g_malloc (keyLen + 1);
		RegQueryValueEx (hKey, key, 0, NULL, keyBuffer,  &keyLen);
		RegCloseKey (hKey);
	}

	return keyBuffer;
}

gint
planner_conf_get_int (const gchar *key, GError **error)
{
	gint         ret_val = 0;
	gchar *keyBuffer;
	glong keyLen;
	HKEY hKey;

	RegOpenKeyEx (HKEY_CURRENT_USER, WIN32_KEYNAME, 0, KEY_READ, &hKey);
	if(RegQueryValueEx (hKey, key, 0, NULL, NULL,  &keyLen) == ERROR_SUCCESS) {
		keyBuffer = g_malloc (keyLen + 1);
		RegQueryValueEx (hKey, key, 0, NULL, keyBuffer,  &keyLen);
		
		ret_val = g_ascii_strtoull (keyBuffer, NULL, 10);
		
		g_free (keyBuffer);
		
		RegCloseKey (hKey);
	}

	return ret_val;
}

gboolean
planner_conf_set_bool (const gchar *key, gboolean value, GError **error)
{
	gboolean     ret_val = 0;
	gchar keyBuffer[10];
	gint keyType = REG_SZ;
	HKEY hKey;
	g_return_val_if_fail (planner_conf_create (), FALSE);

	RegOpenKeyEx (HKEY_CURRENT_USER, WIN32_KEYNAME, 0, KEY_WRITE, &hKey);

	g_stpcpy (keyBuffer, value == TRUE ? "TRUE" : "FALSE");
	ret_val = RegSetValueEx (hKey, key, 0, keyType, keyBuffer,  sizeof(keyBuffer));
	RegCloseKey (hKey);

	return ret_val;
}

gboolean
planner_conf_set_string (const gchar *key, const gchar *value, GError **error)
{
	gboolean     ret_val = 0;
	gint keyType = REG_SZ;
	HKEY hKey;
	g_return_val_if_fail (planner_conf_create (), FALSE);

	RegOpenKeyEx (HKEY_CURRENT_USER, WIN32_KEYNAME, 0, KEY_WRITE, &hKey);

	ret_val = RegSetValueEx (hKey, key, 0, keyType, value,  strlen(value) + 1);

	RegCloseKey (hKey);

	return ret_val;
}

gboolean
planner_conf_set_int (const gchar *key, gint value, GError **error)
{
	gboolean     ret_val = 0;
	gint keyType = REG_SZ;
	gchar keyBuffer[20];
	HKEY hKey;
	g_return_val_if_fail (planner_conf_create (), FALSE);
	RegOpenKeyEx (HKEY_CURRENT_USER, WIN32_KEYNAME, 0, KEY_WRITE, &hKey);

	g_snprintf (keyBuffer, sizeof(keyBuffer), "%d", value);
	ret_val = RegSetValueEx (hKey, key, 0, keyType, keyBuffer,  strlen(keyBuffer) + 1);

	RegCloseKey (hKey);

	return ret_val;
}

