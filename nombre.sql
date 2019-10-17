-- SQL script to build the nombre database

-- These commands only work with SQLite3, primarily used for integrity checking
PRAGMA cell_size_check=true;
PRAGMA case_sensitive_like=true; -- Most searches will explicitly use 'ilike' anyway
PRAGMA secure_delete=true;
PRAGMA foreign_keys=on; -- Just in case it's not enforced by default

-- These pragma commands set version info
PRAGMA user_version=0;
-- Add up N+O+M == 78 + 79 + 77
PRAGMA application_id=234;

-- Build out the tables

-- Allow categorization of definitions
CREATE TABLE IF NOT EXISTS categories (
	id integer UNIQUE NOT NULL, -- Numeric ID of the category
	name text UNIQUE NOT NULL, -- Human friendly category name
	PRIMARY KEY (id,name)
);

-- Primary definition table
CREATE TABLE IF NOT EXISTS definitions (
	term text UNIQUE NOT NULL, -- Primary key in this table, if already exists, create altdef
	meaning text NOT NULL, -- Cannot guarantee this will be unique
	category integer NOT NULL DEFAULT -1, -- Default everything to "uncategorized" if not specified
	CHECK (category > -2), -- -1 is the only valid value under 0
	PRIMARY KEY (term),
	FOREIGN KEY categories(id)
);

-- Secondary table if a term has multiple meanings
CREATE TABLE IF NOT EXISTS altdefs (
	term text NOT NULL, -- Refers to an entry in the definitions table
	defno integer NOT NULL DEFAULT 0, -- Should be incremented for each new alternate definition
	altdef text NOT NULL, -- The actual alternative definition
	category integer NOT NULL DEFAULT -1, -- same as definitions table
	FOREIGN KEY term REFERENCES definitions(term),
	FOREIGN KEY category REFERENCES categories(id)
);

-- Allow storage of reference links/notes 
-- using the hash of the term and category as a primary key
-- while still maintaining the term and category as columns
-- for easier manual querying and use of the search functionality
CREATE TABLE IF NOT EXISTS references (
);

-- Initialize the categories table with some default values
-- can be extended with the nombre command, or manually, of course
BEGIN;
	INSERT INTO categories VALUES
	(-1, 'UNCATEGORIZED'),
	(0, 'NIX'),
	(1, 'NETWORKING'),
	(2, 'APPLICATIONS'),
	(3, 'SECURITY'),
	(4, 'PROGRAMMING');
COMMIT;

-- Define some indices for quicker lookups on certain values expected to be common

-- Last line of executed code, run an optimization pass
PRAGMA optimize;
