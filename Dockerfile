FROM ubuntu:latest
RUN apt update && \
    apt install gcc -y && \
    apt install python3 -y && \
    apt install git -y && \
    apt install vim -y && \
    apt install valgrind -y

RUN mkdir /workspace
WORKDIR workspace/
ADD . .

