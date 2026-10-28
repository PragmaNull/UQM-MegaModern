@echo off
python "%~dp0scripts\test.py" %*
exit /b %ERRORLEVEL%
