-- $Id$

-- Planner Database Schema

-- Daniel Lundin <daniel@codefactory.se>
-- Richard Hult <richard@imendio.com>
-- Copyright 2003 CodeFactory AB

--
-- Project
--
CREATE TABLE project (
       	proj_id	         serial,
       	name           	 text NOT NULL,
	company		 text,
	manager		 text,
	proj_start	 date NOT NULL DEFAULT CURRENT_TIMESTAMP,
	cal_id	         integer,
	phase	         text,
	default_group_id integer,
	revision         integer,
	last_user        text NOT NULL DEFAULT (user),
	PRIMARY KEY (proj_id)
);
GRANT select,insert,update,delete ON project TO GROUP planner;
GRANT select,update ON project_proj_id_seq TO GROUP planner;

--
-- Phases
--
CREATE TABLE phase (
        phase_id        serial,
        proj_id         integer,
        name            text NOT NULL,
	FOREIGN KEY (proj_id) REFERENCES project (proj_id) 
		ON DELETE CASCADE ON UPDATE CASCADE,
	PRIMARY KEY (phase_id)
);
GRANT select,insert,update,delete ON phase TO GROUP planner;
GRANT select,update ON phase_phase_id_seq TO GROUP planner;

--
-- Day Types
--
CREATE TABLE daytype (
	dtype_id	serial,
       	proj_id		integer,
       	name           	text,
       	descr          	text,
	is_work	        boolean NOT NULL DEFAULT FALSE,
	is_nonwork      boolean NOT NULL DEFAULT FALSE,
	UNIQUE (proj_id, name),
	FOREIGN KEY (proj_id) REFERENCES project (proj_id) 
		ON DELETE CASCADE ON UPDATE CASCADE,
	PRIMARY KEY (dtype_id)
);
GRANT select,insert,update,delete ON daytype TO GROUP planner;
GRANT select,update ON daytype_dtype_id_seq TO GROUP planner;


--
-- Calendar
--
CREATE TABLE calendar (
	cal_id		serial,
       	proj_id		integer,
	parent_cid	integer,
       	name           	text,
	day_mon		integer DEFAULT NULL,
	day_tue		integer DEFAULT NULL,
	day_wed		integer DEFAULT NULL,
	day_thu		integer DEFAULT NULL,
	day_fri		integer DEFAULT NULL,
	day_sat		integer DEFAULT NULL,
	day_sun		integer DEFAULT NULL,
	FOREIGN KEY (proj_id) REFERENCES project (proj_id) 
		ON DELETE CASCADE ON UPDATE CASCADE,
	FOREIGN KEY (day_mon) REFERENCES daytype (dtype_id) 
		ON DELETE SET DEFAULT ON UPDATE CASCADE
		DEFERRABLE INITIALLY DEFERRED,
	FOREIGN KEY (day_tue) REFERENCES daytype (dtype_id) 
		ON DELETE SET DEFAULT ON UPDATE CASCADE
		DEFERRABLE INITIALLY DEFERRED,
	FOREIGN KEY (day_wed) REFERENCES daytype (dtype_id) 
		ON DELETE SET DEFAULT ON UPDATE CASCADE
		DEFERRABLE INITIALLY DEFERRED,
	FOREIGN KEY (day_thu) REFERENCES daytype (dtype_id) 
		ON DELETE SET DEFAULT ON UPDATE CASCADE
		DEFERRABLE INITIALLY DEFERRED,
	FOREIGN KEY (day_fri) REFERENCES daytype (dtype_id) 
		ON DELETE SET DEFAULT ON UPDATE CASCADE
		DEFERRABLE INITIALLY DEFERRED,
	FOREIGN KEY (day_sat) REFERENCES daytype (dtype_id) 
		ON DELETE SET DEFAULT ON UPDATE CASCADE
		DEFERRABLE INITIALLY DEFERRED,
	FOREIGN KEY (day_sun) REFERENCES daytype (dtype_id) 
		ON DELETE SET DEFAULT ON UPDATE CASCADE
		DEFERRABLE INITIALLY DEFERRED,
	FOREIGN KEY (parent_cid) REFERENCES calendar (cal_id) 
		ON DELETE SET NULL ON UPDATE CASCADE,
	PRIMARY KEY (cal_id)
);
GRANT select,insert,update,delete ON calendar TO GROUP planner;
GRANT select,update ON calendar_cal_id_seq TO GROUP planner;
ALTER TABLE project ADD CONSTRAINT project_cal_id 
	FOREIGN KEY (cal_id) REFERENCES calendar (cal_id) 
	ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE INITIALLY DEFERRED;


--
-- Day
--
CREATE TABLE day (
	day_id		serial,
       	cal_id		integer,
	dtype_id	integer,
	date		date,	
	FOREIGN KEY (dtype_id) REFERENCES daytype (dtype_id) 
		ON DELETE CASCADE ON UPDATE CASCADE,
	FOREIGN KEY (cal_id) REFERENCES calendar (cal_id) 
		ON DELETE CASCADE ON UPDATE CASCADE,
	PRIMARY KEY (day_id)
);
GRANT select,insert,update,delete ON day TO GROUP planner;


--
-- Day (working) Interval
--
CREATE TABLE day_interval (
       	cal_id		integer,
	dtype_id	integer,
       	start_time      time with time zone,
       	end_time       	time with time zone,
	FOREIGN KEY (dtype_id) REFERENCES daytype (dtype_id) 
		ON DELETE CASCADE ON UPDATE CASCADE DEFERRABLE,
	FOREIGN KEY (cal_id) REFERENCES calendar (cal_id) 
		ON DELETE CASCADE ON UPDATE CASCADE,
	PRIMARY KEY (dtype_id, cal_id, start_time, end_time)
);
GRANT select,insert,update,delete ON day_interval TO GROUP planner;


--
-- Task
--
CREATE TABLE task (
       	task_id           serial,
	parent_id	  integer,
	proj_id	          integer,
       	name              text NOT NULL,
	note		  text,
	start	          timestamp with time zone,
	finish	          timestamp with time zone,
	work	 	  integer DEFAULT 0,
	duration	  integer DEFAULT 0,
	percent_complete  integer DEFAULT 0,
	is_milestone	  boolean NOT NULL DEFAULT FALSE,
	is_fixed_work     boolean NOT NULL DEFAULT TRUE,
	constraint_type   text NOT NULL DEFAULT 'ASAP',
        constraint_time   timestamp with time zone,
	CHECK (constraint_type = 'ASAP' OR constraint_type = 'MSO' OR constraint_type = 'FNLT' OR constraint_type = 'SNET'),
 	CHECK (percent_complete > -1 AND percent_complete < 101),
	FOREIGN KEY (proj_id) REFERENCES project (proj_id) 
		ON DELETE CASCADE ON UPDATE CASCADE,
	FOREIGN KEY (parent_id) REFERENCES task (task_id) 
		ON DELETE SET NULL ON UPDATE CASCADE,
	PRIMARY KEY (task_id)
);
GRANT select,insert,update,delete ON task TO GROUP planner;
GRANT select,update ON task_task_id_seq TO GROUP planner;

-- FIXME: Add triggers to handle different types of tasks/milestones


--
-- Predecessor (tasks)
--
CREATE TABLE predecessor (
	task_id	 	 integer NOT NULL,
	pred_task_id	 integer NOT NULL,
       	pred_id          serial,
       	type             text NOT NULL DEFAULT 'FS',
        lag              integer DEFAULT 0,
	CHECK (type = 'FS' OR type = 'FF' OR type = 'SS' OR type = 'SF'),
	UNIQUE (pred_id),
	FOREIGN KEY (task_id) REFERENCES task (task_id) 
		ON DELETE CASCADE ON UPDATE CASCADE,
	FOREIGN KEY (pred_task_id) REFERENCES task (task_id) 
		ON DELETE CASCADE ON UPDATE CASCADE,
	PRIMARY KEY (task_id, pred_task_id)
);
GRANT select,insert,update,delete ON predecessor TO GROUP planner;
GRANT select,update ON predecessor_pred_id_seq TO GROUP planner;


--
-- Property types
--
CREATE TABLE property_type (
       	proptype_id    	  serial,
       	proj_id           integer,
       	name           	  text NOT NULL,
	label		  text NOT NULL,
	type		  text NOT NULL DEFAULT 'text',
	owner		  text NOT NULL DEFAULT 'project',
	descr		  text,
	CHECK (type = 'date' OR type = 'duration' OR type = 'float' 
	       OR type = 'int' OR type = 'text' OR type = 'text-list'
	       OR type = 'cost'),
	CHECK (owner = 'project' OR owner = 'task' OR owner = 'resource'),
	UNIQUE (proj_id, name),
	FOREIGN KEY (proj_id) REFERENCES project (proj_id) 
		ON DELETE CASCADE ON UPDATE CASCADE,
	PRIMARY KEY (proptype_id)
);
GRANT select,insert,update,delete ON property_type TO GROUP planner;
GRANT select,update ON property_type_proptype_id_seq TO GROUP planner;


--
-- Properties
--
CREATE TABLE property (
       	prop_id    	  serial,
	proptype_id	  integer NOT NULL,
	value		  text,
	FOREIGN KEY (proptype_id) REFERENCES property_type (proptype_id) 
		ON DELETE CASCADE ON UPDATE CASCADE,
	PRIMARY KEY (prop_id)
);
GRANT select,insert,update,delete ON property TO GROUP planner;
GRANT select,update ON property_prop_id_seq TO GROUP planner;


--
-- Project properties
--
CREATE TABLE project_to_property (
       	proj_id         integer,
	prop_id	        integer,
	FOREIGN KEY (proj_id) REFERENCES project (proj_id) 
		ON DELETE CASCADE ON UPDATE CASCADE,
	FOREIGN KEY (prop_id) REFERENCES property (prop_id) 
		ON DELETE CASCADE ON UPDATE CASCADE,
	PRIMARY KEY (proj_id, prop_id)
);
GRANT select,insert,update,delete ON project_to_property TO GROUP planner;


--
-- Task properties
--
CREATE TABLE task_to_property (
	prop_id	        integer,
       	task_id         integer,
	FOREIGN KEY (task_id) REFERENCES task (task_id) 
		ON DELETE CASCADE ON UPDATE CASCADE,
	FOREIGN KEY (prop_id) REFERENCES property (prop_id) 
		ON DELETE CASCADE ON UPDATE CASCADE,
	PRIMARY KEY (task_id, prop_id)
);
GRANT select,insert,update,delete ON task_to_property TO GROUP planner;


--
-- Resource Group
--
CREATE TABLE resource_group (
       	group_id         serial,
	proj_id	        integer,
       	name            text NOT NULL,
	admin_name	text,
	admin_phone	text,
	admin_email	text,
	FOREIGN KEY (proj_id) REFERENCES project (proj_id) 
		ON DELETE CASCADE ON UPDATE CASCADE,
	PRIMARY KEY (group_id)
);
GRANT select,insert,update,delete ON resource_group TO GROUP planner;
GRANT select,update ON resource_group_group_id_seq TO GROUP planner;


--
-- Resource
--
CREATE TABLE resource (
       	res_id          serial,
	proj_id	        integer,
	group_id	integer,
       	name            text,
	email		text,
	note		text,
	is_worker	boolean NOT NULL DEFAULT TRUE,
	units		real NOT NULL DEFAULT 1.0,
	std_rate	real NOT NULL DEFAULT 0.0,	
	ovt_rate	real NOT NULL DEFAULT 0.0,
	cal_id		integer,
	FOREIGN KEY (proj_id) REFERENCES project (proj_id) 
		ON DELETE CASCADE ON UPDATE CASCADE,
	FOREIGN KEY (group_id) REFERENCES resource_group (group_id) 
		ON DELETE SET NULL ON UPDATE CASCADE,
	FOREIGN KEY (cal_id) REFERENCES calendar (cal_id) 
		ON DELETE CASCADE ON UPDATE CASCADE,
	PRIMARY KEY (res_id)
);
GRANT select,insert,update,delete ON resource TO GROUP planner;
GRANT select,update ON resource_res_id_seq TO GROUP planner;


--
-- Resource properties
--
CREATE TABLE resource_to_property (
	prop_id	        integer,
       	res_id          integer,
	FOREIGN KEY (res_id) REFERENCES resource (res_id) 
		ON DELETE CASCADE ON UPDATE CASCADE,
	FOREIGN KEY (prop_id) REFERENCES property (prop_id) 
		ON DELETE CASCADE ON UPDATE CASCADE,
	PRIMARY KEY (res_id, prop_id)
);
GRANT select,insert,update,delete ON resource_to_property TO GROUP planner;


--
-- Allocations (of resources)
--
CREATE TABLE allocation (
	task_id	        integer,
       	res_id          integer,
	units		real NOT NULL DEFAULT 1.0,
	FOREIGN KEY (res_id) REFERENCES resource (res_id) 
		ON DELETE CASCADE ON UPDATE CASCADE,
	FOREIGN KEY (task_id) REFERENCES task (task_id) 
		ON DELETE CASCADE ON UPDATE CASCADE,
	PRIMARY KEY (res_id, task_id)
);
GRANT select,insert,update,delete ON allocation TO GROUP planner;


