@echo off
python "%~dp0scripts\setup.py" %*
exit /b %ERRORLEVEL%
