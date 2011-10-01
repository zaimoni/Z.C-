@REM not sure how far below WinXP this will run
@REM runs only C99 regression tests for zcc.exe -E
@REM (C)2009,2011 Kenneth Boyd, license: MIT.txt
@setlocal
@set BAD_PASS=0
@set BAD_PASS_NAME=LastAccepted:
@set REJECT_TEST=0
@set FAILED=0
@set BAD_FAIL_NAME=LastRejected:
@set ACCEPT_TEST=0
@set CPP=..\..\bin\zcc -E --pedantic

@echo Checking ISO error requirements
@echo ====
@for %%f in (UNICODE.C99\Error*.h) do @echo %CPP% %%f & @%CPP% %%f && (set /a BAD_PASS=BAD_PASS+1 & set BAD_PASS_NAME=%BAD_PASS_NAME% %%f)
@for %%f in (UNICODE.C99\Error*.h) do @set /a REJECT_TEST=REJECT_TEST+1
@for %%f in (define.C99\Error*.h) do @echo %CPP% %%f & @%CPP% %%f && (set /a BAD_PASS=BAD_PASS+1 & set BAD_PASS_NAME=%BAD_PASS_NAME% %%f)
@for %%f in (define.C99\Error*.h) do @set /a REJECT_TEST=REJECT_TEST+1
@for %%f in (defined.C99\Error*.h) do @echo %CPP% %%f & @%CPP% %%f && (set /a BAD_PASS=BAD_PASS+1 & set BAD_PASS_NAME=%BAD_PASS_NAME% %%f)
@for %%f in (defined.C99\Error*.h) do @set /a REJECT_TEST=REJECT_TEST+1
@for %%f in (ifdef.C99\Error*.h) do @echo %CPP% %%f & @%CPP% %%f && (set /a BAD_PASS=BAD_PASS+1 & set BAD_PASS_NAME=%BAD_PASS_NAME% %%f)
@for %%f in (ifdef.C99\Error*.h) do @set /a REJECT_TEST=REJECT_TEST+1
@for %%f in (if.C99\Error*.h) do @echo %CPP% %%f & @%CPP% %%f && (set /a BAD_PASS=BAD_PASS+1 & set BAD_PASS_NAME=%BAD_PASS_NAME% %%f)
@for %%f in (if.C99\Error*.h) do @set /a REJECT_TEST=REJECT_TEST+1
@for %%f in (line.C99\Error*.h) do @echo %CPP% %%f & @%CPP% %%f && (set /a BAD_PASS=BAD_PASS+1 & set BAD_PASS_NAME=%BAD_PASS_NAME% %%f)
@for %%f in (line.C99\Error*.h) do @set /a REJECT_TEST=REJECT_TEST+1
@for %%f in (pragma.C99\Error*.h) do @echo %CPP% %%f & @%CPP% %%f && (set /a BAD_PASS=BAD_PASS+1 & set BAD_PASS_NAME=%BAD_PASS_NAME% %%f)
@for %%f in (pragma.C99\Error*.h) do @set /a REJECT_TEST=REJECT_TEST+1

@echo Checking ISO acceptance requirements
@echo ====
@for %%f in (Warn*.h) do @echo %CPP% %%f & @%CPP% %%f || (set /a FAILED=FAILED+1 & set BAD_FAIL_NAME=%BAD_FAIL_NAME% %%f)
@for %%f in (Warn*.h) do @set /a ACCEPT_TEST=ACCEPT_TEST+1
@for %%f in (Pass*.h) do @echo %CPP% %%f & @%CPP% %%f || (set /a FAILED=FAILED+1 & set BAD_FAIL_NAME=%BAD_FAIL_NAME% %%f)
@for %%f in (Pass*.h) do @set /a ACCEPT_TEST=ACCEPT_TEST+1
@for %%f in (define.C99\Pass*.h) do @echo %CPP% %%f & @%CPP% %%f || (set /a FAILED=FAILED+1 & set BAD_FAIL_NAME=%BAD_FAIL_NAME% %%f)
@for %%f in (define.C99\Pass*.h) do @set /a ACCEPT_TEST=ACCEPT_TEST+1
@for %%f in (if.C99\Pass*.h) do @echo %CPP% %%f & @%CPP% %%f || (set /a FAILED=FAILED+1 & set BAD_FAIL_NAME=%BAD_FAIL_NAME% %%f)
@for %%f in (if.C99\Pass*.h) do @set /a ACCEPT_TEST=ACCEPT_TEST+1

@echo %BAD_PASS% of %REJECT_TEST% rejection tests accepted
@if not "%BAD_PASS_NAME%"=="LastAccepted:" @echo %BAD_PASS_NAME%
@echo %FAILED% of %ACCEPT_TEST% acceptance tests rejected
@if not "%BAD_FAIL_NAME%"=="LastRejected:" @echo %BAD_FAIL_NAME%
