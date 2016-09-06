# Dockerfile for building rootfs.tar.gz
#
# USAGE:
#   $ docker build  -t rootfs  -f Dockerfile.rootfs  .
#   $ docker run  -v $(pwd):/data  rootfs
#
# Version  1.0
#


# pull base image
FROM ubuntu:trusty

MAINTAINER William Yeh <william.pjyeh@gmail.com>


# install toolchain for building...
RUN DEBIAN_FRONTEND=noninteractive  \
    apt-get update  &&  \
    apt-get -f -y install curl make gcc

# install extract-elf-so
RUN curl -sSL http://bit.ly/install-extract-elf-so | sudo bash



WORKDIR /tmp
COPY    .  /tmp
RUN     ./build-rootfs.sh


# copy generated rootfs tarball...
VOLUME [ "/data" ]
CMD ["cp", "rootfs.tar.gz", "redis.conf", "/data/"]
