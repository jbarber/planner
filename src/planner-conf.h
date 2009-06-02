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

#ifndef __PLANNER_CONF_H__
#define __PLANNER_CONF_H__

gboolean      planner_conf_dir_exists    (const gchar     *dir,
					  GError         **error);
gboolean      planner_conf_get_bool      (const gchar     *key,
					  GError         **error);
gchar *       planner_conf_get_string    (const gchar     *key,
					  GError         **error);
gint          planner_conf_get_int       (const gchar     *key,
					  GError         **error);

gboolean      planner_conf_set_bool      (const gchar     *key,
					  gboolean         value,
					  GError         **error);
gboolean      planner_conf_set_string    (const gchar     *key,
					  const gchar     *value,
					  GError         **error);
gboolean      planner_conf_set_int       (const gchar     *key,
					  gint             value,
					  GError         **error);

#endif /* __PLANNER_CONF_H__ */

