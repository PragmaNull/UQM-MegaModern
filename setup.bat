@echo off
rem Installs the conan dependencies and generates the CMake build files. See scripts/setup.py.
python "%~dp0scripts\setup.py" %*
exit /b %ERRORLEVEL%
