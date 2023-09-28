docker rm -f producer
docker rm -f consumer
docker rm -f broker
docker build -t stream_network .