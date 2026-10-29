@echo off
rem Installs the conan dependencies. See scripts/install.py.
python "%~dp0scripts\install.py" %*
exit /b %ERRORLEVEL%
