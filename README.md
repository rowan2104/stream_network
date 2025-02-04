# **Stream_Network**
## **About** 
This is a project I hade to make for a college computer networks module

The project is written in C and uses docker to run simulated instances.

The project has a single broker container that recieves content of text, audio and/or visual from one or more producer containers,
and forwards the content to consumer containers who subscribed to the different content.

## **Usage**
### **Setup**
Most of the project running can be handled by docker.

FIRST TIME RUN ONLY:
> docker network create -d bridge --subnet 172.22.0.0/16 streaming_network

This network creation command only needs to be ran once.

> docker build -t stream_network .

Once the image is built, so long as the docker file isn't modified it should not have to be rebuilt.

### To run:

>docker run -d --name broker1  --cap-add=ALL -w /work_space/Broker -it stream_network
>docker network connect --ip=172.22.0.2 streaming_network broker1

>docker run -d --name producer1  --cap-add=ALL -w /work_space/Producer -it stream_network
>docker network connect --ip=172.22.1.1 streaming_network producer1

>docker run -d --name consumer1  --cap-add=ALL -w /work_space/Consumer -it stream_network
>docker network connect --ip=172.22.2.1 streaming_network consumer1

to run more producer or consumers, run the commands with the container name incremeneted,
and the last digit of the ip address incremeneted

The container will immediatly run the code on launch,
here are the commands for each container:

## Producer:
connect \<id\> : connect the producer to the broker, id must be a 6 digit long string of only valid hex characters
eg: 
> connect ab01ff

target \<dir\>: set the target directory for the files that the next stream will use
eg: 
> target wiki_rope

stream \<num\> \<types\>: start streaming with the stream num set and content from the target dir of the given types.
The types are the first letters of each of the 3 core types
eg: 
>stream 01 avt
(this stream is stream 01, eg ab01ff01 and streams audio, video and text content from the given directory)

delete \<streamID\> : stops/deletes a currently active stream.
eg: 
>delete ab01ff01

>disconnect
completely disconnects the producer from the broker, and stops all streams



## Consumer:
>connect
connects the consumer to the broker

list    : request a list of all active streams from the broker
optional, can add content types to filter search
eg: 
>list av
will request streams of that either have a text component or audio component

subscribe \<streamid\> : subscribe to a given stream via its streamID
eg: 
>subscribe ab01ff01

unsubscribe \<streamid\> : unsubscribe to a given stream via its streamID
eg: 
>unsubscribe ab01ff01

>disconnect
disconnects the consumer from the broker



New content can be added that can be streamed. Create a new folder in the producer folder titled whater you want
then add in the content file to it, making sure they are called audio.mp3, text.txt or video.mp4.
From there they will be copied into the container on rebuild and can be streamed
noe for text.txt, you must include timestamps at the beginning of every line in the format used in the existing files.
