libmarquise
===========

libmarquise is a small C library for writing data frames to
[vaultaire][0]. 

requirements
============

 - [zeromq][1] 4.0+
 - [protobuf][2]
 - [protobuf-c][3]

building
========

	./configure
	make
	sudo make install

configuration
=============

Some libmarquise behaviours are informed by environment variables. The
following are required, and writes will fail with EINVAL if they are not
set:

 - LIBMARQUISE_ORIGIN (origin value for vaultaire)

The following are optional, and modify default behaviours:

 - LIBMARQUISE_DEBUG (enable debug output)
 - LIBMARQUISE_COLLATOR_MAX_MESSAGES (the number of messages that must
   be written in order to force collation and sending; consequently,
   this is the maximum number of DataFrames which will be encoded in the
   DataBurst that libmarquise sends).
 - LIBMARQUISE_PROFILING (enable detailed debug output for tracking
   individual dataframes)
 - LIBMARQUISE_DEFERRAL_DIR is the directory to use for serialization of
   bursts that have timed out on send - defaults to /var/tmp.
 - LIBMARQUISE_HIGH_WATER_MARK is the number of bursts which can remain
   in-flight before being deferred - defaults to 256.

bindings
========

 - go: https://github.com/anchor/gomarquise
 - haskell: https://github.com/anchor/marquise-haskell

[0]: https://github.com/anchor/vaultaire
[1]: http://zeromq.org/
[2]: https://code.google.com/p/protobuf/
[3]: https://code.google.com/p/protobuf-c/

