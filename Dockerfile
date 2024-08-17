# docker build -t riblet -f Dockerfile .

# syntax=docker/dockerfile:1.9

FROM debian:bookworm-slim

RUN apt-get update 
RUN apt-get install -y build-essential git libssl-dev

ENV PATH="$PATH:/riblet"

WORKDIR /riblet

COPY . .

RUN git submodule update --init
RUN make setup-golpe
RUN make -j
