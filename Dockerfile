FROM ubuntu:22.04
ENV TZ=Asia/Shanghai
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt update && apt install -y git curl wget vim cmake g++-12 meson uuid-dev libprotobuf-dev libevent-dev librocksdb-dev \
libzstd-dev libsnappy-dev libbz2-dev libsecp256k1-dev libssl-dev libminizip-dev libcrypto++-dev libboost-dev libgrpc-dev libgrpc++-dev \
libc-ares-dev  libgtest-dev libgmock-dev libmsgpack-dev libgflags-dev libabsl-dev libcli11-dev liburing-dev && \
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 12 && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-12 12

RUN curl https://sh.rustup.rs -sSf | sh -s -- -y
ENV PATH "$PATH:/root/.cargo/bin"

WORKDIR /root/workspace/
RUN mkdir -p /root/topnetwork
COPY README.md /root/workspace
COPY topio /bin/topio
ENTRYPOINT ["tail","-f","/dev/null"]