REM created by Rowan Barr
@echo off
setlocal enabledelayedexpansion

set "container_base_name=producer"
set "container_name=%container_base_name%1"
set "container_index=1"

:CHECK_EXISTING
REM Check if a container with the current name already exists
docker inspect --format="{{.Name}}" %container_name% 2>nul
if %errorlevel%==0 (
    REM If it exists, increment the index and check again
    set /a container_index+=1
    set "container_name=%container_base_name%%container_index%"
    goto CHECK_EXISTING
)

REM Run the container using the "docker run" command (as mentioned in the previous responses)
docker run -d --name %container_name%  --cap-add=ALL -w /work_space/Producer -it stream_network

REM Connect the container to the "streaming_network" network and assign an IP address
docker network connect --ip=172.22.1.%container_index% streaming_network %container_name%

echo Container %container_name% created.
start cmd /k docker start -i %container_name%