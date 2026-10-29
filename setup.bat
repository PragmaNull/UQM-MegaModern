@echo off
rem Generates the CMake build files. See scripts/setup.py.
python "%~dp0scripts\setup.py" %*
exit /b %ERRORLEVEL%
