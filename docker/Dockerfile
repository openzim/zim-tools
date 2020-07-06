FROM ubuntu:focal

# Update system
RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections

# Configure locales
RUN apt-get update -y && \
    apt-get install -y --no-install-recommends locales && \
    apt-get clean -y && \
    rm -rf /var/lib/apt/lists/*
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8
RUN locale-gen en_US.UTF-8

# Install necessary packages
RUN apt-get update -y && \
    apt-get install -y --no-install-recommends git pkg-config libtool automake autoconf make g++ liblzma-dev coreutils meson ninja-build wget zlib1g-dev libicu-dev libgumbo-dev libmagic-dev ca-certificates xz-utils cmake && \
    apt-get clean -y && \
    rm -rf /var/lib/apt/lists/*

# Update CA certificates
RUN update-ca-certificates

# Install Xapian (wget zlib1g-dev)
RUN wget https://oligarchy.co.uk/xapian/1.4.16/xapian-core-1.4.16.tar.xz
RUN tar xvf xapian-core-1.4.16.tar.xz
RUN cd xapian-core-1.4.16 && ./configure
RUN cd xapian-core-1.4.16 && make all install
RUN rm -rf xapian

# Install zimlib (libicu-dev)
RUN git clone https://github.com/openzim/libzim.git
RUN cd libzim && git checkout 6.1.7
RUN cd libzim && meson . build
RUN cd libzim && ninja -C build install
RUN rm -rf libzim

# Install zimwriterfs (libgumbo-dev libmagic-dev)
COPY . zim-tools
RUN ls zim-tools
RUN export PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig/ && cd zim-tools && meson . build
RUN cd zim-tools && ninja -C build install
RUN rm -rf zim-tools
ENV LD_LIBRARY_PATH /usr/local/lib/x86_64-linux-gnu/

# Boot commands
CMD zimwriterfs ; /bin/bash
