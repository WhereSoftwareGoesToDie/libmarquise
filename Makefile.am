ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src
dist_doc_DATA = README

# Debian specific, for travis CI, but you may use it also.
debian-install-deps: 
	sudo apt-get install -y build-essential libzmq-dev libglib2.0-dev protobuf-c-compiler

