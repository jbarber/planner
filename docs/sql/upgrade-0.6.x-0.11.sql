
-- Planner Database Schema Upgrade from 0.6.x to 0.11

-- Lincoln Phipps <lincoln.phipps@openmutual.net>
-- Copyright 2004 Lincoln Phipps

--
-- Resource Schema Changes from 0.6.x to 0.11
--

ALTER TABLE resource ADD COLUMN short_name text;
ALTER TABLE task ADD COLUMN priority integer;
ALTER TABLE task ALTER COLUMN priority SET DEFAULT 0;
ALTER TABLE task ADD CHECK (priority > -1 AND priority < 10000);

