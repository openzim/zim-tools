FROM ubuntu:latest

# Update system
RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections
RUN apt-get update -y
RUN apt-get install -y apt-utils
RUN apt-get dist-upgrade -y

# Configure locales
RUN apt-get install locales
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8
RUN locale-gen en_US.UTF-8

# Install necessary packages
RUN apt-get install -y git
RUN apt-get install -y pkg-config
RUN apt-get install -y libtool
RUN apt-get install -y automake
RUN apt-get install -y autoconf
RUN apt-get install -y make
RUN apt-get install -y g++
RUN apt-get install -y liblzma-dev
RUN apt-get install -y coreutils
RUN apt-get install -y meson
RUN apt-get install -y ninja-build

# Install Xapian
RUN apt-get install -y wget
RUN apt-get install -y zlib1g-dev
RUN wget https://oligarchy.co.uk/xapian/1.4.11/xapian-core-1.4.11.tar.xz
RUN tar xvf xapian-core-1.4.11.tar.xz
RUN cd xapian-core-1.4.11 && ./configure
RUN cd xapian-core-1.4.11 && make all install
RUN rm -rf xapian

# Install zimlib
RUN apt-get install -y libicu-dev
RUN git clone https://github.com/openzim/libzim.git
RUN cd libzim && git checkout 5.0.0
RUN cd libzim && meson . build
RUN cd libzim && ninja -C build install
RUN rm -rf libzim

# Install zimwriterfs
RUN apt-get install -y libgumbo-dev
RUN apt-get install -y libmagic-dev
RUN git clone https://github.com/openzim/zimwriterfs.git
RUN cd zimwriterfs && git checkout 1.3.3
RUN cd zimwriterfs && meson . build
RUN cd zimwriterfs && ninja -C build install
RUN rm -rf zimwriterfs
RUN ldconfig
ENV LD_LIBRARY_PATH /usr/local/lib/x86_64-linux-gnu/

# Cleaning
RUN apt-get remove --purge --assume-yes git
RUN apt-get remove --purge --assume-yes pkg-config
RUN apt-get remove --purge --assume-yes libtool
RUN apt-get remove --purge --assume-yes automake
RUN apt-get remove --purge --assume-yes autoconf
RUN apt-get remove --purge --assume-yes make
RUN apt-get remove --purge --assume-yes g++
RUN apt-get remove --purge --assume-yes liblzma-dev
RUN rm -rf /var/lib/apt/lists/* /var/tmp/* /tmp/*

# Boot commands
CMD zimwriterfs ; /bin/bash