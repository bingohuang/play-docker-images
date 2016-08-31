FROM busybox
RUN mkdir /tmp/foo
RUN dd if=/dev/zero of=/tmp/foo/bar bs=1048576 count=100
RUN rm /tmp/foo/bar
