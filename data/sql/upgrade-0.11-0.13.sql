-- Planner Database Schema update
                                                                                                                             
-- Alvaro del Castillo <acs@barrapunto.com>
-- 
-- New table to store global properties for Planner
-- Actually (0.13) it is used to store the database version

CREATE TABLE property_global (
        prop_id           serial,
        prop_name         text NOT NULL,
        value             text,
        PRIMARY KEY (prop_id)
);