#Created by Rowan Barr
FROM ubuntu


# Install necessary build tools and libraries
RUN apt-get update
#Compile and network tools
RUN apt-get install -y gcc make net-tools netcat tcpdump inetutils-ping wireshark nano
#Image viewer tool
RUN apt-get install -y imagemagick
# Install required dependencies
# Set environment variables for non-interactive installation
# Fix for librarys that have interactive installers
ENV DEBIAN_FRONTEND=noninteractive

# Set the timezone to UTC during the image build
RUN ln -fs /usr/share/zoneinfo/UTC /etc/localtime

#Essential libraries and dependancies
RUN apt-get update
RUN apt-get install -y pkg-config
RUN apt-get install -y cmake
RUN apt-get install -y build-essential

#VIdeo encoder/decoder
RUN apt-get install -y ffmpeg
RUN apt-get install -y libavcodec-dev
RUN apt-get install -y libavformat-dev
RUN apt-get install -y libswscale-dev

#JPEG encoder/decoder
RUN apt-get install -y libsdl2-dev
RUN apt-get install -y libffms2-dev
RUN apt-get install -y libjpeg-dev



# Run the script to set the kernel parameters
# RUN /usr/local/bin/set_kernel_params.sh


# Set the working directory
WORKDIR /work_space/
COPY Producer /work_space/Producer
COPY Consumer /work_space/Consumer
COPY Broker /work_space/Broker

COPY scripts /scripts

COPY shared /work_space/Producer
COPY shared /work_space/Consumer
COPY shared /work_space/Broker


# Setup for wireshark
ENV DISPLAY=host.docker.internal:0
ENV LIBGL_ALWAYS_INDIRECT=1
ENV XDG_RUNTIME_DIR=/tmp/foobar

#Install script for easy wireshark open
ENV PATH="/scripts:${PATH}"



#Set STDERR to STDOUT
CMD ["bash", "-c", "2>&1"]

#Sleep is for time for CMD windows to open
CMD ["bash", "-c", "sleep 1.2 && gcc -g -o main main.c -ljpeg -lavformat -lavcodec -lavutil  -lffms2 -lswscale `sdl2-config --cflags --libs` && ./main"]