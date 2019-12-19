#!/bin/sh

## Test the initialization capabilities of nombre
builtin echo -n "Verifying that database initialization works... "
nombre -Ii nombre.sql -d "test/nombre.db" >> "test/log" 2>> "test/log"
if [ $? -eq 0 ]
then
	builtin echo "Initialization: passed" >> "test/results"
	builtin echo "Pass"
else
	builtin echo "Initialization: failed" >> "test/results"
	builtin echo "Failed"
fi
