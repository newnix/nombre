# NOMBRE(1)

A simple SQLite3 database manager for recording and retrtieving terms, like having your own dictionary in 
a single database file with a small binary utility to manage it.

## Interface
The `nombre(1)` interface is a semi-natural language command line primarily driven by the use of subcommands,
which will then construct and run SQL queries to interface with the database on your behalf. This allows 
users with no knowledge of SQL or SQLite3 in particular to get full functionality from this utility and
makes it easy for more advanced users and developers to extend or add new interfaces to. 

## Usage
The usage, or "help" function is not complete at this time, but will retain the same general look and expand to
cover more subcommands over time. Simply run `make install` and the `nombre(1)` utility will be installed to 
`~/bin/nombre` unless otherwise overridden in the Makefile. You will want to ensure that this is in your `$PATH`
by running `printf "PATH=%s:%s\n" "${HOME}/bin" "${PATH}" >> ~/.profile` or similar config file for your interactive shell.

```
$ nombre -h

nombre: A simple, local definition database
	nombre [-DIv] -d database -i initfile -f I/O file [subcommand] term...
	  -D Enable run-time debug printouts
	  -I Initialize the database
	  -v Perform a verification test on the database
	  -i Initialization SQL script to use (only useful with -I)
	  -d The location of the nombre database (default: ~/.local/nombre.db)
	  -f Use the given file for import/export operations

Subcommands:
	(def)ine: Look up a definition
	(add)def: Add a new definition to the database
	(key)word: Perform a keyword search on saved entries
```

The basic use case would look similar to the following:

```
# Retrieve a recorded definition
$ nombre def tls
tls: Transport Layer Security

# The 'def' subcommand is assumed if omitted
$ nombre tls
tls: Transport Layer Security

# Add a new definition to the database
$ nombre add test garbage data
Added definition for TEST

# View that same definition
$ nombre test
test: garbage data
```

This will allow simple inserts and selects on the database to enable storage of whatever terms are desired.
There is a table for alternative definitions, allowing the same term to have multiple meanings in several different contexts,
and even allow separate categorizations of such definitions. 

It's also possible to perform a keyword search or just list all the currently known terms and definitions:

```
# Perform a search for anything that contains the string "sec" (case insensitive)
$ nombre key sec
Found the following matches:
  SSL: Secure Sockets Layer
  TLS: Transport Layer Security

# List all known terms and their definitions
$ nombre lst
Here's what I know:
  (*NIX/POSIX): Portable Operating Systems Interface
  (APPS/SQL): Structured Query Language
  (DEVEL/API): Application Programming Interface
  (NET/UDP): User Datagram Protocol
  (NET/TLS): Transport Layer Security
  (NET/TCP): Transmission Control Protocol
  (NET/SSL): Secure Sockets Layer
  (SEC/AES): Advanced Encryption Standard
  (UNCAT/TEST): garbage data
  (UNCAT/MASTO): Shorthand for the "Mastodon" social networking platform
  (UNCAT/BSD): Berkeley Software Distribution
```

As you can see, there's more information in the listing than there was when just looking up definitions, this is the
categorization functionality. Every entry can be given a category, if no category is given, it will default to "UNCAT",
or "UNCATEGORIZED". While not currently implemented, it will be possible to list all currently defined categories and 
create new ones. 

It **IS**, however, currently possible to enter new terms with their own category *AND* do lookups restricted to a given category:

```
# Only show definitions for "tcp" in the "net" category
$ nombre grp def net tcp
tcp: Transmission Control Protocol

# Create an entry under the "sec" category for "mac"
$ nombre grp add sec mac Mandatory Access Control
Added definition for SEC/MAC

# Verify the entry exists
$ nombre grp def sec mac
mac: Mandatory Access Control
```

Other planned features:

	* Data interchange via import/export subcommands
	* Database integrity/version checking
	* Listing by category
	* Listing known categories
	* Adding sources/references for definitions
	* Updating entries (all kinds)
	* Deleting entries (all kinds)
	* Creating custom categories
	* Automatic handling of alternate definitions
	* Database/application versioning
