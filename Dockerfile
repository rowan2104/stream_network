FROM ubuntu

# Install necessary build tools and libraries
RUN apt-get update
RUN apt-get install -y gcc make net-tools netcat tcpdump inetutils-ping wireshark nano


# Set the working directory
WORKDIR /work_space/

# Copy the Producer source code into the container
COPY Producer /work_space/Producer
COPY Consumer /work_space/Consumer
COPY Broker /work_space/Broker
COPY scripts /scripts

ENV DISPLAY=host.docker.internal:0
ENV LIBGL_ALWAYS_INDIRECT=1
ENV XDG_RUNTIME_DIR=/tmp/foobar
ENV PATH="/scripts:${PATH}"

CMD ["bash", "-c", "2>&1"]
#CMD ["bash", "-c", "mkdir /tmp/foobar && chmod 700 /tmp/foobar"]
CMD ["bash", "-c", "bash"]
CMD ["bash", "-c", "gcc -o main main.c && bash"]