@echo off
rem Runs the game with the content directory. See scripts/run.py.
python "%~dp0scripts\run.py" %*
exit /b %ERRORLEVEL%
