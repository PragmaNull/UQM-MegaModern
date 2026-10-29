@echo off
rem Runs the input scripts in test/scripts against the game. See scripts/regress.py.
python "%~dp0scripts\regress.py" %*
exit /b %ERRORLEVEL%
