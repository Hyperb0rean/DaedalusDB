FROM ubuntu:latest AS build
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
ADD ./CMakeLists.txt /ddb/CMakeLists.txt
ADD ./Makefile /ddb/Makefile

WORKDIR /ddb/

RUN ls -l
RUN ls /usr/bin/ 

RUN make compile-release

FROM build AS test

ADD ./tests /ddb/tests

RUN git clone https://github.com/google/googletest.git

RUN make compile-asan && make test TEST=-Performance TYPE=-asan

FROM build AS run

RUN groupadd -r usermode && useradd -r -g usermode usermode
RUN chown -R usermode:usermode .

USER usermode

COPY --from=build /ddb/build-release/ddb .

ENTRYPOINT [ "./ddb" ]