@echo off
setlocal enabledelayedexpansion

set "container_base_name=broker"
set "container_index=1"
set "max_consecutive_checks=3"
set "consecutive_checks=0"

:CHECK_EXISTING
set "container_name=%container_base_name%%container_index%"
docker inspect --format="{{.Name}}" %container_name% 2>nul
if %errorlevel%==0 (
    REM If the container exists, stop it
    docker rm -f %container_name%
    set /a consecutive_checks=0
) else (
    set /a consecutive_checks+=1
)

set /a container_index+=1

REM Check if we've reached the maximum consecutive checks
if %consecutive_checks% geq %max_consecutive_checks% (
    echo Maximum consecutive checks reached. Exiting.
    exit /b
) else (
    goto CHECK_EXISTING
)
