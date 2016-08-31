# Redis for Debian jessie, with chaining commands.
#
# Reference:  https://github.com/dockerfile/redis
#
#

FROM debian:jessie

ENV VER     3.0.0
ENV TARBALL http://download.redis.io/releases/redis-$VER.tar.gz


RUN echo "==> Install curl and helper tools..."  && \
    apt-get update                      && \
    apt-get install -y  curl make gcc   && \
    \
    \
    echo "==> Download, compile, and install..."  && \
    curl -L $TARBALL | tar zxv  && \
    cd redis-$VER               && \
    make                        && \
    make install                && \
    \
    \
    echo "==> Configure for Dockerized version of Redis..."     && \
    mkdir -p /etc/redis                                         && \
    cp -f *.conf /etc/redis                                     && \
    \
    \
    echo "==> Clean up..."  && \
    apt-get remove -y --auto-remove curl make gcc  && \
    apt-get clean                                  && \
    rm -rf /var/lib/apt/lists/*  /redis-$VER


# Redis port.
EXPOSE 6379


CMD ["redis-server"]
