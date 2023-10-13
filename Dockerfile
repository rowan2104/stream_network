FROM ubuntu


# Install necessary build tools and libraries
RUN apt-get update
RUN apt-get install -y gcc make net-tools netcat tcpdump inetutils-ping wireshark nano
RUN apt-get install -y imagemagick
# Install required dependencies
# Set environment variables for non-interactive installation
ENV DEBIAN_FRONTEND=noninteractive

# Set the timezone to UTC during the image build
RUN ln -fs /usr/share/zoneinfo/UTC /etc/localtime

RUN apt-get update
RUN apt-get install -y pkg-config
RUN apt-get install -y cmake
RUN apt-get install -y build-essential


RUN apt-get install -y ffmpeg
RUN apt-get install -y libavcodec-dev
RUN apt-get install -y libavformat-dev
RUN apt-get install -y libswscale-dev
# Set the working directory
WORKDIR /work_space/
COPY Producer /work_space/Producer
COPY Consumer /work_space/Consumer
COPY Broker /work_space/Broker

COPY scripts /scripts

COPY shared /work_space/Producer
COPY shared /work_space/Consumer
COPY shared /work_space/Broker



ENV DISPLAY=host.docker.internal:0
ENV LIBGL_ALWAYS_INDIRECT=1
ENV XDG_RUNTIME_DIR=/tmp/foobar
ENV PATH="/scripts:${PATH}"

CMD ["bash", "-c", "2>&1"]
#CMD ["bash", "-c", "mkdir /tmp/foobar && chmod 700 /tmp/foobar"]
#CMD ["bash", "-c", "bash"]
CMD ["bash", "-c", "sleep 1.2 && gcc -o main main.c -lavformat -lavcodec -lavutil -lswscale &&./main && bash"]