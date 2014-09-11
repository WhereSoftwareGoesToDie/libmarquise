libmarquise
===========

libmarquise is a small C library for writing data frames to
[vaultaire][0]. It writes frames to a spool file which is then read by 
the Marquise daemon.

building
========

	autoreconf -i
	./configure
	make
	sudo make install

bindings
========

 - Python: https://github.com/anchor/pymarquise

The following bindings have not been updated for Vaultaire v2: 

 - go: https://github.com/anchor/gomarquise
 - Ruby: https://github.com/mpalmer/marquise-ruby (`gem install marquise`)

In addition, the reference implementation of the Marquise interface is in 
Haskell: https://github.com/anchor/marquise

packages
========

There are source packages available for Centos/RHEL and Debian
[here][1]. 

[0]: https://github.com/anchor/vaultaire
[1]: https://github.com/anchor/packages

