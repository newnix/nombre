-- SQL script to build the nombre database

-- These commands only work with SQLite3, primarily used for integrity checking
PRAGMA cell_size_check=true;
PRAGMA case_sensitive_like=true; -- Most searches will explicitly use 'ilike' anyway
PRAGMA secure_delete=true;
PRAGMA foreign_keys=0; -- Just in case it's not enforced by default

-- These pragma commands set version info
PRAGMA user_version=0;
-- Add up N+O+M == 78 + 79 + 77
PRAGMA application_id=234;

-- Build out the tables

DROP TABLE IF EXISTS categories;
-- Allow categorization of definitions
CREATE TABLE IF NOT EXISTS categories (
	id integer UNIQUE NOT NULL, -- Numeric ID of the category
	name text UNIQUE NOT NULL, -- Human friendly category name
	PRIMARY KEY (id,name)
);

-- Initialize the categories table with some default values
-- can be extended with the nombre command, or manually, of course
-- NOTE: Look at normalizing the category lengths, with a longer optional name
BEGIN;
	INSERT INTO categories VALUES
	(-1, 'UNCAT'),
	(0, '*NIX'),
	(1, 'NET'),
	(2, 'APPS'),
	(3, 'SEC'),
	(4, 'DEVEL');
COMMIT;

DROP TABLE IF EXISTS category_verbose;
-- Allow longer category labels
CREATE TABLE IF NOT EXISTS category_verbose (
	id integer UNIQUE NOT NULL, -- Same primary key as in categories
	short text UNIQUE NOT NULL, -- Same as the 'name' in categories
	nlong text UNIQUE NOT NULL, -- Longer/more verbose category name
	PRIMARY KEY (id,nlong),
	FOREIGN KEY (id) REFERENCES categories(id),
	FOREIGN KEY (short) REFERENCES categories(name)
);

-- Add in the long category definitions
BEGIN;
	INSERT INTO category_verbose (id, short, nlong) VALUES
	(-1, 'UNCAT', 'UNCATEGORIZED'),
	(0, '*NIX', 'UNIX Like Systems'),
	(1, 'NET', 'Networking'),
	(2, 'APPS', 'Applications'),
	(3, 'SEC', 'Security'),
	(4, 'DEVEL', 'Programming/Development');
COMMIT;

PRAGMA foreign_keys=1;

-- Primary definition table
CREATE TABLE IF NOT EXISTS definitions (
	term text UNIQUE NOT NULL, -- Primary key in this table, if already exists, create altdef
	meaning text NOT NULL, -- Cannot guarantee this will be unique
	category integer NOT NULL DEFAULT -1, -- Default everything to "uncategorized" if not specified
	CHECK (category > -2), -- -1 is the only valid value under 0
	PRIMARY KEY (term),
	FOREIGN KEY (category) REFERENCES categories(id)
);

-- Secondary table if a term has multiple meanings
CREATE TABLE IF NOT EXISTS altdefs (
	term text NOT NULL, -- Refers to an entry in the definitions table
	defno integer NOT NULL DEFAULT 0, -- Should be incremented for each new alternate definition
	altdef text NOT NULL, -- The actual alternative definition
	category integer NOT NULL DEFAULT -1, -- same as definitions table
	FOREIGN KEY (term) REFERENCES definitions(term),
	FOREIGN KEY (category) REFERENCES categories(id)
);

-- Allow storage of reference links/notes 
-- using the hash of the term and category as a primary key
-- while still maintaining the term and category as columns
-- for easier manual querying and use of the search functionality
CREATE TABLE IF NOT EXISTS defrefs (
	idhash blob UNIQUE NOT NULL, -- Hashed combination of the term + category + definition number, used to build a B-tree index
	defno integer NOT NULL, -- -1 signifies main definition
	term text NOT NULL, 
	category integer NOT NULL,
	source text NOT NULL,
	PRIMARY KEY (idhash),
	FOREIGN KEY (term) REFERENCES definitions(term),
	FOREIGN KEY (category) REFERENCES categories(id)
);


-- Define some indices for quicker lookups on certain values expected to be common
CREATE INDEX IF NOT EXISTS altdata_idx ON altdefs (term, defno);

-- Provide some baseline data for the database to have available
BEGIN;
	INSERT INTO definitions VALUES
	('SSL', 'Secure Sockets Layer', 1),
	('TLS', 'Transport Layer Security', 1),
	('TCP', 'Transmission Control Protocol', 1),
	('UDP', 'User Datagram Protocol', 1),
	('POSIX', 'Portable Operating Systems Interface', 0),
	('SQL', 'Structured Query Language', 2),
	('AES', 'Advanced Encryption Standard', 3),
	('API', 'Application Programming Interface', 4),
	('MASTO', 'Sharthand for "Mastodon" social networking', -1)
	;
COMMIT;

-- Last line of executed code, run an optimization pass
PRAGMA optimize;
