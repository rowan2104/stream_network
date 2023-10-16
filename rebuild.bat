call rm_producer.bat
call rm_consumer.bat
call rm_broker.bat
docker build -t stream_network .

REM docker network create -d bridge --subnet 172.20.0.0/16 stream_network

REM run order, [rebuild.bat, run_containers.bat]