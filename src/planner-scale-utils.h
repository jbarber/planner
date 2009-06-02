/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2005 Imendio AB
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

#ifndef __PLANNER_SCALE_UTILS_H__
#define __PLANNER_SCALE_UTILS_H__

#include <libplanner/mrp-time.h>

#if 0

YEAR,     /* Short: N/A     Medium: 2002      Long: N/A         */
HALFYEAR, /* Short: N/A     Medium: H1        Long: 2002, H1    */
QUARTER,  /* Short: Q1      Medium: N/A       Long: 2002, Qtr 1 */
MONTH,    /* Short: J       Medium: Jan       Long: Jan '02     */
WEEK,     /* Short: Wk 19   Medium: Week 19   Long: Week 19 '02 */
DAY,      /* Short: 1       Medium: Mon 1     Long: Mon, Apr 1  */
HALFDAY,  /* Short: 1       Medium: 1         Long: 1           */
TWO_HOURS,/* Short: 1       Medium: 1         Long: 1           */
HOUR      /* Short: 1       Medium: 1         Long: 1           */

#endif

typedef enum {
	PLANNER_SCALE_FORMAT_SHORT,
	PLANNER_SCALE_FORMAT_MEDIUM,
	PLANNER_SCALE_FORMAT_LONG
} PlannerScaleFormat;

typedef struct {
	MrpTimeUnit        major_unit;
	PlannerScaleFormat major_format;

	MrpTimeUnit        minor_unit;
	PlannerScaleFormat minor_format;

	/* Nonworking intervals shorter than this is not drawn (seconds). */
	gint               nonworking_limit;
} PlannerScaleConf;

extern const PlannerScaleConf *planner_scale_conf;

gchar * planner_scale_format_time (mrptime            t,
				   MrpTimeUnit   unit,
				   PlannerScaleFormat format);
gint    planner_scale_clamp_zoom  (gdouble            zoom);


#endif /* __PLANNER_SCALE_UTILS_H__ */
