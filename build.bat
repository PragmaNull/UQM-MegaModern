@echo off
python "%~dp0scripts\build.py" %*
exit /b %ERRORLEVEL%
