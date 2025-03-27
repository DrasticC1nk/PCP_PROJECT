@echo off
setlocal enabledelayedexpansion

set BUILD_DIR=builds
set LOG_FILE=run_log.txt

if exist %LOG_FILE% del %LOG_FILE%

set EXE_LIST=stm0 stm1 stm2 stm3

for %%E in (%EXE_LIST%) do (
    echo Running %%E.exe with argument 5... >> %LOG_FILE%
    echo ---------------------------------- >> %LOG_FILE% 
    %BUILD_DIR%\%%E.exe 5 >> %LOG_FILE% 2>&1
    echo. >> %LOG_FILE%
)

echo Execution complete. Results stored in %LOG_FILE%.
