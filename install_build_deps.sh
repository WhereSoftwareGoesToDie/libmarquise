#! /bin/bash

# Install build dependencies on (on recent Debian-like systems). 

sudo apt-get install -y autoconf libtool automake build-essential libglib2.0-dev libprotobuf-c0-dev protobuf-c-compiler rubygems
gem install ronn
