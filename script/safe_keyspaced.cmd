@echo off
:start
%1 %2
if %ERRORLEVEL% == 0 goto end
if %ERRORLEVEL% == 1 goto end
if %ERRORLEVEL% == 143 goto end
goto start
:end
