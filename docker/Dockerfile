FROM ubuntu:latest

# Configure deb system
RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections
RUN locale-gen "en_US.UTF-8"
ENV LANG en_US.UTF-8
RUN update-locale
RUN dpkg-reconfigure locales

# Increase standards system limits
RUN echo "* - nofile unlimited" >  /etc/security/limits.conf
RUN echo "* - stack  unlimited" >> /etc/security/limits.conf

# Update system
RUN apt-get update -y
RUN apt-get install -y apt-utils
RUN apt-get dist-upgrade -y

# Install necessary packages
RUN apt-get install -y git
RUN apt-get install -y pkg-config
RUN apt-get install -y libtool
RUN apt-get install -y automake
RUN apt-get install -y autoconf
RUN apt-get install -y make
RUN apt-get install -y g++
RUN apt-get install -y liblzma-dev

# Install zimlib
RUN git clone https://github.com/openzim/libzim.git
RUN cd libzim && ./autogen.sh
RUN cd libzim && ./configure
RUN cd libzim && make all install
RUN rm -rf libzim

# Install Xapian
RUN apt-get install -y wget
RUN apt-get install -y zlib1g-dev
RUN wget https://oligarchy.co.uk/xapian/1.4.3/xapian-core-1.4.3.tar.xz
RUN tar xvf xapian-core-1.4.3.tar.xz
RUN cd xapian-core-1.4.3 && ./configure
RUN cd xapian-core-1.4.3 && make all install
RUN rm -rf xapian

# Install zimwriterfs
RUN apt-get install -y libgumbo-dev
RUN apt-get install -y libmagic-dev
RUN apt-get install -y libicu-dev
RUN git clone https://github.com/openzim/zimwriterfs.git
RUN cd zimwriterfs && ./autogen.sh
RUN cd zimwriterfs && ./configure
RUN cd zimwriterfs && make all install
RUN rm -rf zimwriterfs

# Boot commands
CMD zimwriterfs ; /bin/bash
