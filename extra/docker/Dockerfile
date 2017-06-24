FROM ubuntu:16.04

RUN apt-get update && apt-get install -y build-essential libphysfs-dev cmake

RUN mkdir -p /staging/tm

COPY . /staging/tm

RUN cd /staging/tm && \
cmake . && \
make
