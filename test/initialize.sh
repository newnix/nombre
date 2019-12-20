#!/bin/sh

DBNAME="test/nombre.db"
DBISQL="nombre.sql"
LOGFILE="test/log"
RESULTS="test/results"
TESTS="prepare initialize"

initialize() {
	## Test the initialization capabilities of nombre
	builtin echo -n "Verifying that database initialization works... "
	nombre -Ii "${DBISQL}" -d "${DBNAME}" >> "${LOGFILE}" 2>> "${LOGFILE}"
	return $?
}

prepare() {
	:>${LOGFILE}
	rm -f ${DBNAME}
	return 0
}

## Lastly, get the number of passes/failures and present that info to the user
showresults(){
	pass=$(($(fgrep -c "pass" ${RESULTS})))
	fail=$(($(fgrep -c "fail" ${RESULTS})))
	printf "Results:\n\tPassed: %d\tFailed: %d\n\n" "${pass}" "${fail}"
	while read line
	do
		builtin echo "${line}"
	done < ${RESULTS}
}

for t in ${TESTS}
do
	result=$(( 0 + ${t}))
	if [ ${result} -eq 0 ]
	then
		builtin echo "${t}: passed" >> "${RESULTS}"
		builtin echo "Pass"
	else
		builtin echo "${t}: failed" >> "${RESULTS}"
		builtin echo "Failed"
	fi
done

showresults
