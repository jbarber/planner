/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2005      Imendio AB
 * Copyright (C) 2001-2002 CodeFactory AB
 * Copyright (C) 2001-2002 Richard Hult <richard@imendio.com>
 * Copyright (C) 2001-2002 Mikael Hallendal <micke@imendio.com>
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

#ifndef __PLANNER_FORMAT_H__
#define __PLANNER_FORMAT_H__

#include <glib.h>
#include <libplanner/mrp-project.h>

gchar *planner_format_int                      (gint         number);
gchar *planner_format_duration                 (MrpProject  *project,
						gint         duration);
gchar *planner_format_duration_with_day_length (gint         duration,
						gint         day_length);
gchar *planner_format_date                     (mrptime      date);
gchar *planner_format_float                    (gfloat       number,
						guint        precision,
						gboolean     fill_with_zeroes);
gint   planner_parse_int                       (const gchar *str);
gfloat planner_parse_float                     (const gchar *str);
gint   planner_parse_duration                  (MrpProject  *project,
						const gchar *input);
gint   planner_parse_duration_with_day_length  (const gchar *input,
						gint         day_length);

#endif /* __PLANNER_FORMAT_H__ */
