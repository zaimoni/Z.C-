#!/bin/sh
# runs only C99 regression tests for zcc.exe -E
# (C)2009,2011 Kenneth Boyd, license: MIT.txt
ASSERT_FAILED=0
ASSERT_FAIL_NAME=

function code_screen {
	if test ${1} -eq 3; then let ++ASSERT_FAILED; ASSERT_FAIL_NAME="$ASSERT_FAIL_NAME ${2}"; fi;
}

function run_tests {
	local BAD_PASS=0
	local BAD_PASS_NAME=
	local REJECT_TEST=0
	local FAILED=0
	local BAD_FAIL_NAME=
	local ACCEPT_TEST=0
	local CPP="../../bin/zcc -E --pedantic"

	echo Checking ISO error requirements
	echo ====
	for F in UNICODE.C99/Error*.h; do let ++REJECT_TEST; echo $CPP $F; if $CPP $F; then let ++BAD_PASS; BAD_PASS_NAME="$BAD_PASS_NAME $F"; else code_screen $? $F; fi; done;
	for F in define.C99/Error*.h; do let ++REJECT_TEST; echo $CPP $F; if $CPP $F; then let ++BAD_PASS; BAD_PASS_NAME="$BAD_PASS_NAME $F"; else code_screen $? $F; fi; done;
	for F in defined.C99/Error*.h; do let ++REJECT_TEST; echo $CPP $F; if $CPP $F; then let ++BAD_PASS; BAD_PASS_NAME="$BAD_PASS_NAME $F"; else code_screen $? $F; fi; done;
	for F in ifdef.C99/Error*.h; do let ++REJECT_TEST; echo $CPP $F; if $CPP $F; then let ++BAD_PASS; BAD_PASS_NAME="$BAD_PASS_NAME $F"; else code_screen $? $F; fi; done;
	for F in if.C99/Error*.h; do let ++REJECT_TEST; echo $CPP $F; if $CPP $F; then let ++BAD_PASS; BAD_PASS_NAME="$BAD_PASS_NAME $F"; else code_screen $? $F; fi; done;
	for F in line.C99/Error*.h; do let ++REJECT_TEST; echo $CPP $F; if $CPP $F; then let ++BAD_PASS; BAD_PASS_NAME="$BAD_PASS_NAME $F"; else code_screen $? $F; fi; done;
	for F in pragma.C99/Error*.h; do let ++REJECT_TEST; echo $CPP $F; if $CPP $F; then let ++BAD_PASS; BAD_PASS_NAME="$BAD_PASS_NAME $F"; else code_screen $? $F; fi; done;

	echo Checking ISO acceptance requirements
	echo ====
	for F in Warn*.h; do let ++ACCEPT_TEST; echo $CPP $F; if $CPP $F; then :; else code_screen $? $F; let ++FAILED; BAD_FAIL_NAME="$BAD_FAIL_NAME $F"; fi; done;
	for F in Pass*.h; do let ++ACCEPT_TEST; echo $CPP $F; if $CPP $F; then :; else code_screen $? $F; let ++FAILED; BAD_FAIL_NAME="$BAD_FAIL_NAME $F"; fi; done;
	for F in define.C99/Pass*.h; do let ++ACCEPT_TEST; echo $CPP $F; if $CPP $F; then :; else code_screen $? $F; let ++FAILED; BAD_FAIL_NAME="$BAD_FAIL_NAME $F"; fi; done;
	for F in if.C99/Pass*.h; do let ++ACCEPT_TEST; echo $CPP $F; if $CPP $F; then :; else code_screen $? $F; let ++FAILED; BAD_FAIL_NAME="$BAD_FAIL_NAME $F"; fi; done;

	echo -E $BAD_PASS of $REJECT_TEST rejection tests accepted
	if test -n "$BAD_PASS_NAME"; then echo -E $BAD_PASS_NAME; fi
	echo -E $FAILED of $ACCEPT_TEST acceptance tests rejected
	if test -n "$BAD_FAIL_NAME"; then echo -E $BAD_FAIL_NAME; fi
	echo -E $ASSERT_FAILED tests failed by critical bugs
	if test -n "$ASSERT_FAIL_NAME"; then echo -E $ASSERT_FAIL_NAME; fi
}

run_tests
