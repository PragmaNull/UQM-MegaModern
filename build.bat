@echo off
rem Builds the project. See scripts/build.py.
python "%~dp0scripts\build.py" %*
exit /b %ERRORLEVEL%
