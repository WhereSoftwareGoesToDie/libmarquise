libmarquise
===========

libmarquise is a small C library for writing data frames to
[vaultaire][0]. It writes frames to a spool file which is then read by 
the Marquise daemon.

building
========

	./configure
	make
	sudo make install

bindings
========

 - go: https://github.com/anchor/gomarquise
 - haskell: https://github.com/anchor/marquise-haskell
 - Ruby: https://github.com/mpalmer/marquise-ruby (`gem install marquise`)

packages
========

There are source packages available for Centos/RHEL and Debian
[here][1]. 

[0]: https://github.com/anchor/vaultaire
[1]: https://github.com/anchor/packages

