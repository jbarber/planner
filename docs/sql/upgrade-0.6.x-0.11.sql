
-- Planner Database Schema Upgrade from 0.6.x to 0.7

-- Lincoln Phipps <lincoln.phipps@openmutual.net>
-- Copyright 2004 Lincoln Phipps

--
-- Resource Schema Changes from 0.6.x to 0.7 
--

ALTER TABLE resource ADD COLUMN short_name text

