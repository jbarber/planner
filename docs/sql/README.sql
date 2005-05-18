This is a plugin that implements a backend for keeping project data in a
postgres SQL database.

A very brief getting started howto (change username etc to use...):


* Init database (WARNING: This is usually already done on a working system):

 initdb -D /tmp/test-db

* Start server (WARNING: This is usually already done on a working system):

  postmaster -D /tmp/test-db


-- This is usualy done by a database administrator  

* Enable access to another user than the default:

  echo 'CREATE USER rhult CREATEDB;' | psql -e template1

* Create database group:

  echo 'CREATE GROUP planner WITH USER rhult;' | psql -e -d plannerdb

-- This is done by the user

* Create a UNICODE database:

  createdb -E UNICODE -U rhult plannerdb

* Create tables from new. This depends upon the schema that you are up to 
  in your code. Use the highest versioned SQL file.

  e.g.
  cat database.sql | psql -e -U rhult -d plannerdb

  or e.g.
  cat database-0.11.sql | psql -e -U rhult -d plannerdb

* Update Tables (in existing database). If you already have a database then 
  just update this to the new schema. Use the appropriate upgrade file based
  on your old and new schema,
  
  e.g.
  cat upgrade-0.6.x-0.11.sql | psql -e -U rhult -d plannerdb

-- How to start over
  
* Drop database and group to start over:

  echo 'DROP GROUP planner;' | psql -e -U rhult -d plannerdb
  dropdb plannerdb
 


-- And then...

Start planner
Add some items to a project
Export to a database

Import from the database next time you use planner.

