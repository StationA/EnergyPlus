FROM ubuntu:16.04

RUN apt-get update -y && apt-get install -y \
    build-essential \
    cmake

RUN apt-get install -y \
    python-dev

ADD . /energyplus

WORKDIR /energyplus
RUN mkdir build

WORKDIR /energyplus/build
RUN cmake ..

RUN make && make install

# TODO: Find out why it builds here instead of /usr/local/bin
CMD ["/usr/local/energyplus"]
