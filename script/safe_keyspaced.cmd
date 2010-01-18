@echo off
:start
%1 %2
if %ERRORLEVEL% == 0 goto end
if %ERRORLEVEL% == 1 goto end
if %ERRORLEVEL% == 143 goto end
if %ERRORLEVEL% == 2 goto delete_database
goto start
:delete_database
set KEYSPACE_DIR=%3
if "%KEYSPACE_DIR%" == "" set KEYSPACE_DIR=%~dp2
del %KEYSPACE_DIR%\__*
del %KEYSPACE_DIR%\log*
del %KEYSPACE_DIR%\keyspace
goto start
:end
