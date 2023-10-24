REM created by Rowan Barr
call rm_producer.bat
call rm_consumer.bat
call rm_broker.bat
docker build -t stream_network .

REM docker network create -d bridge --subnet 172.22.0.0/16 streaming_network

