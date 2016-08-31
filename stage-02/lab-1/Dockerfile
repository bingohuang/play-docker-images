# Redis for Ubuntu 14.04
#
# Reference:  https://github.com/dockerfile/redis
#
#

FROM ubuntu:trusty

ENV VER     3.0.0
ENV TARBALL http://download.redis.io/releases/redis-$VER.tar.gz


# ==> Install curl and helper tools...
RUN apt-get update
RUN apt-get install -y  curl make gcc


# ==> Download, compile, and install...
RUN curl -L $TARBALL | tar zxv
WORKDIR  redis-$VER
RUN make
RUN make install


# ==> Configure for Dockerized version of Redis...
RUN mkdir -p /etc/redis
RUN cp -f *.conf /etc/redis


# ==> Clean up...
WORKDIR /
RUN apt-get remove -y --auto-remove curl make gcc
RUN apt-get clean
RUN rm -rf /var/lib/apt/lists/*  /redis-$VER


# Redis port.
EXPOSE 6379


CMD ["redis-server"]
