FROM ubuntu:latest as build
LABEL author="hyperb0rean (Greg) greg.sosnovtsev@gmail.com"

ARG LLVM_VERSION=14.0.0

RUN apt-get update -y && apt-get install -y \
    autoconf \
    automake \
    cmake \
    g++ \
    gcc \
    libxml2-dev \
    make \
    musl-dev \
    ncurses-dev \
    python2 \
    lsb-release \
    wget \
    software-properties-common \
    gnupg \
    git 

RUN wget https://apt.llvm.org/llvm.sh  &&\
    chmod +x llvm.sh && \
    ./llvm.sh 

RUN apt-get update && \
    apt-get install -y libc++-18-dev

ADD ./src /ddb/src
ADD ./tests /ddb/tests
ADD ./CMakeLists.txt /ddb/CMakeLists.txt
ADD ./Makefile /ddb/Makefile

WORKDIR /ddb/

RUN git clone https://github.com/google/googletest.git

RUN ls -l
RUN ls /usr/bin/ 

RUN make compile-asan
RUN make compile-release
RUN make compile-asan

RUN make test TEST=-Performance TYPE=-asan

FROM ubuntu:latest as run


RUN apt-get update -y && \
    apt-get install -y libc++-dev


RUN groupadd -r usermode && useradd -r -g usermode usermode
WORKDIR /ddb
RUN chown -R usermode:usermode .

USER usermode

COPY --from=build /ddb/build-release/ddb .

ENTRYPOINT [ "./ddb" ]