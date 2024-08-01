# Use Ubuntu 22.04 as the base image
FROM ubuntu:22.04

# Set the working directory
WORKDIR /chain

# Update packages and install necessary packages
RUN apt-get update && apt-get install -y \
    build-essential \
    curl \
    vim \
    jq \
    && rm -rf /var/lib/apt/lists/* \
    && apt clean all

# Write specified information into /etc/profile
RUN echo 'unset -f pathmunge' >> /etc/profile && \
    echo 'export TMOUT=360' >> /etc/profile && \
    echo 'ulimit -c unlimited' >> /etc/profile && \
    echo 'export TOPIO_HOME=/chain' >> /etc/profile

# Expose necessary TCP ports
EXPOSE 19081/tcp
EXPOSE 19082/tcp
EXPOSE 19085/tcp
EXPOSE 8080/tcp

# Expose necessary UDP ports
EXPOSE 9000/udp
EXPOSE 9001/udp

# Copy specific files to the /script directory
COPY ./run.sh /script/
COPY ./startupcheck.sh /script/
COPY ./livecheck.sh /script/
COPY ./topargus-agent /script/

# Set the user to root
USER root

# Set the default command to run when the container starts
CMD ["bash", "/script/run.sh"]