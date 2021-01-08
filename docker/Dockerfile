ARG VERSION=
FROM alpine:3
ARG VERSION
LABEL org.opencontainers.image.source https://github.com/openzim/zim-tools
RUN echo "Build image for version: $VERSION"

RUN wget -O - -q https://download.openzim.org/release/zim-tools/zim-tools_linux-x86_64-$VERSION.tar.gz | tar -xz && \
  cp zim-tools*/* /usr/local/bin/ && \
  rm -rf zim-tools*

CMD ["/bin/sh", "-c", "echo 'Welcome to zim-tools! The following binaries are available:' && ls /usr/local/bin/"]
