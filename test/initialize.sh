#!/bin/sh

DBNAME="test/nombre.db"
DBISQL="nombre.sql"
LOGFILE="test/log"
RESULTS="test/results"
TESTS="prepare initialize add_term read_term"
RET=0
ADD_TERM="test"
ADD_DEF="garbage test data"

initialize() {
	## Test the initialization capabilities of nombre
	builtin echo -n "Verifying that database initialization works... "
	nombre -Ii "${DBISQL}" -d "${DBNAME}" >> "${LOGFILE}" 2>> "${LOGFILE}"
	RET=$?
	if [ ${RET} -eq 0 ]
	then
		builtin echo "Pass"
		return ${RET}
	else
		builtin echo "Fail"
		return ${RET}
	fi
}

add_term() {
	## Check to see if we can add a new term to the database without issue
	builtin echo -n "Validating new term creation... "
	nombre -d "${DBNAME}" add ${ADD_TERM} ${ADD_DEF} 2>&1 >> "${LOGFILE}"
	RET=$?
	if [ ${RET} -eq 0 ]
	then
		builtin echo "Pass"
		return ${RET}
	else
		builtin echo "Fail"
		return ${RET}
	fi
}

read_term() {
	## See if it's possible to retrieve the same string written earlier
	builtin echo -n "Validating correctness of record... "
	RES=$(nombre -d "${DBNAME}" def ${ADD_TERM} 2>> "${LOGFILE}")
	RET=$?
	if [ ${RET} -eq 0 ]
	then
		if [ "${ADD_DEF}" = "${RES#${ADD_TERM}: }" ]
		then
			builtin echo "Pass"
			return ${RET}
		else
			builtin echo "Reported success with invalid data!"
		fi
	else
		builtin echo "Fail"
	fi
}

prepare() {
	: > ${LOGFILE}
	: > ${RESULTS}
	rm -f ${DBNAME}
	return 0
}

## Lastly, get the number of passes/failures and present that info to the user
showresults(){
	pass=$(($(fgrep -c "pass" ${RESULTS})))
	fail=$(($(fgrep -c "fail" ${RESULTS})))
	printf "\n[Results]:\n\tPassed: %d\tFailed: %d\n\n" "${pass}" "${fail}"
	builtin echo "Recorded results:"
	while read line
	do
		builtin echo "${line}"
	done < ${RESULTS}
}

for t in ${TESTS}
do
	${t}
	result=$(( 0 + $? ))
	if [ ${result} -eq 0 ]
	then
		builtin echo "${t}: passed" >> "${RESULTS}"
	else
		builtin echo "${t}: failed" >> "${RESULTS}"
	fi
done

showresults
