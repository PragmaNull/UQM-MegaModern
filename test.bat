@echo off
rem Runs the unit tests. See scripts/test.py.
python "%~dp0scripts\test.py" %*
exit /b %ERRORLEVEL%
