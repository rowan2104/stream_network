FROM ubuntu

# Install necessary build tools and libraries
RUN apt-get update
RUN apt-get install -y gcc make net-tools netcat tcpdump inetutils-ping wireshark nano
RUN apt-get install -y imagemagick

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
CMD ["bash", "-c", "sleep 1.2 && gcc -o main main.c && ./main && bash"]