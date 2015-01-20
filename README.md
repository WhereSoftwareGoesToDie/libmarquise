libmarquise
===========

libmarquise is a small C library for writing data frames to
[vaultaire][0]. It writes frames to a spool file which is then read by
the [Marquise](https://github.com/anchor/marquise) daemon.

Building
========

	autoreconf -i
	./configure
	make
	sudo make install

Bindings
========

 - Python: https://github.com/anchor/pymarquise

The following bindings have not been updated for Vaultaire v2:

 - go: https://github.com/anchor/gomarquise
 - Ruby: https://github.com/mpalmer/marquise-ruby (`gem install marquise`)

In addition, the reference implementation of the Marquise interface is in
Haskell: https://github.com/anchor/marquise

Environment variables
========

 - `MARQUISE_SPOOL_DIR` (`/var/spool/marquise`). The location of the
   spool base directory.
 - `DISABLE_NAMESPACE_LOCK` (`0`). If enabled, multiple instances of
   this library are allowed to access the same spool file/contents.
 - `MARQUISE_LOCK_DIR` (`/var/run/marquise`). The location of the lock
   files to ensure no two instances of marquise access the same
   spool/contents files. Has no effect if `DISABLE_NAMESPACE_LOCK` is
   set to `1`.


Packages
========

There are source packages available for Centos/RHEL and Debian
[here][1].

[0]: https://github.com/anchor/vaultaire
[1]: https://github.com/anchor/packages

