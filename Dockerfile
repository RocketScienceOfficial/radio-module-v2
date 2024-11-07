FROM ubuntu:latest


WORKDIR /env


RUN apt-get update
RUN apt-get -y install wget curl git python3 cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib build-essential ninja-build