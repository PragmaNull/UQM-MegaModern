@echo off
rem Compiles the HLSL shaders to headers. See scripts/build_shaders.py.
python "%~dp0scripts\build_shaders.py" %*
exit /b %ERRORLEVEL%
