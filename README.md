libmarquise
===========

libmarquise is a small C library for writing data frames to
vaultaire[0]. 

requirements
============

 - zeromq[1] 4.0+
 - protobuf[2]
 - protobuf-c[3]

building
========

	./configure
	make
	sudo make install

bindings
========

 - go: https://github.com/anchor/gomarquise

[0]: https://github.com/anchor/vaultaire
[1]: http://zeromq.org/
[2]: https://code.google.com/p/protobuf/
[3]: https://code.google.com/p/protobuf-c/

