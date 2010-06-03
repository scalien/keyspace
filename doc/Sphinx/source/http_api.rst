.. _http_api:


********
HTTP API
********

Configuring the HTTP server
===========================

Keyspace includes a HTTP server, running on port 8080 by default. To change the port, set ``http.port`` in the configuration file.

Please note that the HTTP server is intended to be used for testing, debugging and examining small amounts of data. Don't wrap the HTTP API in client libraries, as performance will be much lower than possible and you'll miss out on some features, such as batched commands or non-null terminated key-values. If you want to write a client library for a new language, wrap the fully featured C/C++ client (which also handles master failover) libraries using `SWIG <http://www.swig.org>`_.

HTTP modes
==========

The Keyspace HTTP server can return data in:

- plain-text
- nicely formatted HTML helpful when examining the output of list operations, or
- JSON data.

The default is plain-text. To see HTML formatted output prefix the path with ``/html``, to get JSON prefix the path with ``/json``.

The following section assumes that Keyspace server is running on ``localhost`` port 8080.

All commands use the following format:

  http://localhost:8080/optional-mode/command?param1=param1-value&param2=param2-value...

For example:

  http://localhost:8080/set?key=foo&value=bar

or

  http://localhost:8080/get?key=foo

or

  http://localhost:8080/html/listkeyvalues?prefix=/users/

Issuing single write commands
=============================

The Keyspace write commands are: ``set``, ``testandset``, ``rename``, ``add``, ``delete``, ``remove`` and ``prune``.

``set`` command
---------------

The ``set`` command sets a ``key => value`` pair, creating a new pair if ``key`` did not previously exist, overwriting the old value if it did::

  http://localhost:8080/set?key=kdata&value=vdata

``testandset`` command
------------------------

The ``testandset`` command conditionally and atomically sets a ``key => value`` pair, but only if the current ``value`` matches the user specified value ``test``::

  http://localhost:8080/testandset?key=kdata&test=tdata&value=vdata

``rename`` command
------------------

The ``rename`` command atomically renames a ``key``, leaving its ``value`` alone::

  http://localhost:8080/rename?from=fdata&to=tdata

If the database looked like ``fdata => value`` at the beginning, then it changed to ``tdata => value`` after the successfull ``rename`` operation.

``add`` command
---------------

The ``add`` command takes the value of the key, parses it as a number and atomically increments it by the given offset::

  http://localhost:8080/set?key=i&value=10
  http://localhost:8080/add?key=i&num=3

If the database looked like ``i => 10`` at the beginning, then it changed to ``i => 13`` after the successfull ``add`` operation.

``delete`` command
------------------

The ``delete`` command deletes a ``key => value`` pair by its ``key``::

  http://localhost:8080/delete?key=kdata

``remove`` command
------------------

The ``delete`` command deletes a ``key => value`` pair by its ``key`` and returns the old ``value``::

  http://localhost:8080/set?key=kdata&value=vdata
  http://localhost:8080/remove?key=kdata

``prune`` command
-----------------

The ``prune`` command deletes all ``key => value`` pairs where the ``key`` starts with the given prefix::

  http://localhost:8080/prune?prefix=pdata

For example::

  http://localhost:8080/set?key=john&value=john_data
  http://localhost:8080/set?key=jane&value=jane_data
  http://localhost:8080/set?key=mark&value=mark_data
  http://localhost:8080/prune?prefix=j

to delete ``john`` and ``jane``.

Issuing single read commands
============================

The only Keyspace single read command is ``get``.

``get`` command
---------------

The ``get`` command retrieves a single value from the Keyspace cluster::

  http://localhost:8080/set?key=kdata&value=vdata
  http://localhost:8080/get?key=kdata

You can also issue the identical ``dirtyget`` command, which will be serviced by all nodes, not just the master::

  http://localhost:8080/dirtyget?key=kdata

Issuing list commands
=====================

There are two list commands: ``listkeys`` and ``listkeyvalues`` and one ``count`` command, all have the same set of parameters.

``listkeys`` command
---------------------

The ``listkeys`` command retrieves all keys from the Keyspace cluster which start with a given ``prefix``. Optionally:

- listing can start at a specified ``start`` key
- the maximum number of keys to return can be specified with the ``count`` parameter
- listing can proceed forward or backward using the ``direction`` parameter; listing proceeds forward unless ``direction=b``
- listing can skip the first key using the ``offset=1`` parameter

For example:

  http://localhost:8080/html/listkeys?prefix=test2&start=test25&direction=b&count=10

You can also issue the identical ``dirtylistkeys`` command, which will be serviced by all nodes, not just the master.

``listkeyvalues`` command
--------------------------

The ``listkeyvalues`` command in nearly identical to ``listkeys``, except it also returns the values.

The ``listkeyvalues`` command retrieves all keys and values from the Keyspace cluster which start with a given ``prefix``. Optionally:

- listing can start at a specified ``start`` key
- the maximum number of keys to return can be specified with the ``count`` parameter
- listing can proceed forward or backward using the ``direction`` parameter; listing proceeds forward unless ``direction=b``
- listing can skip the first key using the ``offset=1`` parameter

For example:

  http://localhost:8080/html/listkeyvalues?prefix=test2&start=test25&direction=b&count=10

You can also issue the identical ``dirtylistkeyvalues`` command, which will be serviced by all nodes, not just the master.

``count`` command
-----------------

The ``count`` command has the same parameters as ``listkeys`` or ``listkeyvalues``, but returns the number of keys (or key-value pairs) that they would return.

For example:

  http://localhost:8080/html/count?prefix=test2&start=test25&direction=b&count=10

You can also issue the identical ``dirtycount`` command, which will be serviced by all nodes, not just the master.
