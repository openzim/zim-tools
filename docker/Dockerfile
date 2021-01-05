ARG VERSION=
FROM alpine:3
ARG VERSION
LABEL org.opencontainers.image.source https://github.com/openzim/zim-tools
RUN echo "Build image for version: $VERSION"

RUN wget -O zim-tools.tar.gz -q https://download.openzim.org/release/zim-tools/zim-tools_linux-x86_64-$VERSION.tar.gz && \
  tar -xzf zim-tools.tar.gz && \
  cp zim-tools*/* /usr/local/bin/ && \
  rm -rf zim-tools*

CMD ["/bin/sh", "-c", "echo 'Welcome to zim-tools! The following binaries are available:' && ls /usr/local/bin/"]
