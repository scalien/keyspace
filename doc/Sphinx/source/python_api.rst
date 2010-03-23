.. _python_api:


**********
Python API
**********

Installing from source on Linux and other UNIX platforms
========================================================

You will need the ``python-config`` program to make the Python libraries. To check if you have it, type::

  $ which python-config

If not, you must install the Pyhton dev packages. On Debian, type::

  $ sudo apt-get install python-dev

On Redhat-like systems, type::

  $ sudo yum install python-devel

Note that on some systems, the name of the package may have the Python version number appended. In this case you can specify it as an argument to make eg. ``make pythonlib PYTHON_CONFIG=python2.4-config``.

Once you have verified you have python-config installed, make the Python client libraries::

  $ make pythonlib

in the Keyspace folder. This will create the files ``_keyspace_client.so``, ``keyspace_client.py`` and ``keyspace.py`` in ``bin/python``. Copy these to your Python project, and you are ready to use Keyspace!

Installing from source on Windows
=================================

Currently not supported.

Connecting to the Keyspace cluster
==================================

First, import the keyspace client library::

  import keyspace

Then, create a client object by specifying the Keyspace cluster::

  client = keyspace.Client(["192.168.1.50:7080",
                            "192.168.1.51:7080",
                            "192.168.1.52:7080"])
  # use client here
  del client # closes the connections

Return values
=============

All Keyspace functions return ``None`` on failure.

Setting timeout values
======================

Next, if you would like to, change the global timeout. The global timeout specifies the maximum time, in msec, that a Keyspace client call will block your application. The default is 120 seconds::

  client.set_global_timeout(120*1000)

Next, if you would like to, change the maser timeout timeout. The master timeout specifies the maximum time, in msec, that the library will spend trying to find the master node. The default is 21 seconds::

  client.set_master_timeout(21*1000);

At this point, you are ready to start issuing commands.

Issuing single write commands
=============================

The Keyspace write commands are: ``set``, ``test_and_set``, ``rename``, ``add``, ``delete``, ``remove`` and ``prune``. Note that all Keyspace keys and values do not have to be NULL-terminated strings (eg. you can set a value to be a binary file).

``set`` command
---------------

The ``set`` command sets a ``key => value`` pair, creating a new pair if ``key`` did not previously exist, overwriting the old value if it did::

  client.set("key", "value")

``test_and_set`` command
------------------------

The ``test_and_set`` command conditionally and atomically sets a ``key => value`` pair, but only if the current ``value`` matches the user specified value ``test``::

  client.test_and_set("key", "test", "value")

``rename`` command
------------------

The ``rename`` command atomically renames a ``key``, leaving its ``value`` alone::

  client.rename("from", "to")

If the database was looked like ``from => value`` at the beginning, then it changed to ``to => value`` after the successfull ``rename`` operation.

``add`` command
---------------

The ``add`` command takes the value of the key, parses it as a number and atomically increments it by the given offset::

  client.set("key", str(10))
  client.add("key", 3) # returns 13

If the database was looked like ``key => 10`` at the beginning, then it changed to ``key => 13`` after the successfull ``add`` operation and the variable ``result`` holds the value 13.

``delete`` command
------------------

The ``delete`` command deletes a ``key => value`` pair by its ``key``::

  client.delete("key")

``remove`` command
------------------

The ``delete`` command deletes a ``key => value`` pair by its ``key`` and returns the old ``value``::

  client.set("key", "value")
  client.remove("key") # returns "value"

``prune`` command
-----------------

The ``prune`` command deletes all ``key => value`` pairs where the ``key`` starts with the given prefix::

  client.prune("prefix")

For example::

  client.set("john", "john_data")
  client.set("jane", "jane_data")
  client.set("mark", "mark_data")
  client.prune("j") # deletes "john" => "john_data" and "jane" => "jane_data"

Issuing single read commands
============================

The only Keyspace single read commands is ``get``.

``get`` command
---------------

The ``get`` command retrieves a single value from the Keyspace cluster::

  client.set("key", "value")
  client.get("key") # returns "value"

You can also issue the identical ``dirty_get`` command, which will be serviced by all nodes, not just the master::

  client.set("key", "value")
  client.dirty_get("key") # may return "value"

Issuing list commands
=====================

There are two list commands: ``list_keys`` and ``list_key_values`` and one ``count`` command, all have the same set of parameters.

``list_keys`` command
---------------------

The ``list_keys`` command retrieves all keys from the Keyspace cluster which start with a given ``prefix``. Optionally:

- listing can start at a specified ``start_key``
- the maximum number of keys to return can be specified with the ``count`` parameter
- listing can proceed forward or backward
- listing can skip the first key

The signature of the function is::

  def list_keys(self, prefix = "", start_key = "", count = 0, skip = False, forward = True)

The recommended method to use ``list_keys`` is named arguments::

  client.list_keys(prefix="j")

The result of a list operation in iterated using the ``client``'s result object::

  client.list_keys(prefix="prefix")
  while not client.result.is_end():
    # client.result.key() is a key!
    print(client.result.key())
    client.result.next()

You can also issue the identical ``dirty_list_keys`` command, which will be serviced by all nodes, not just the master.

``list_keyvalues`` command
--------------------------

The ``list_keyvalues`` command in nearly identical to ``list_keys``, except it also returns the values. Hence in the result iteration, ``key`` and ``value`` may be called.

The ``list_keyvalues`` command retrieves all keys and values from the Keyspace cluster which start with a given ``prefix``. Optionally:

- listing can start at a specified ``start_key``
- the maximum number of keys to return can be specified with the ``count`` parameter
- listing can proceed forward or backward
- listing can skip the first key

The signature of the function is::

  def list_key_values(self, prefix = "", start_key = "", count = 0, skip = False, forward = True)

The result of a list operation in iterated using the ``client``'s result object::

  client.list_keys(prefix="prefix")
  while not client.result.is_end():
    # client.result.key() is a key!
    # client.result.value() is a value!
    print(client.result.key() + " => " + client.result.value())
    client.result.next()

You can also issue the identical ``dirty_list_keyvalues`` command, which will be serviced by all nodes, not just the master.

``count`` command
-----------------

The ``count`` command has the same parameters as ``list_keys`` or ``list_keyvalues``, but returns the number of keys (or key-value pairs) that they would return. The signature of the function is::

  def count(self, prefix = "", start_key = "",
            count = 0, skip = False, forward = True)

  client.count(prefix="prefix")

You can also issue the identical ``dirty_count`` command, which will be serviced by all nodes, not just the master.

Issuing batched write commands
==============================

For maximum thruput performance, it is possible to issue many write commands together; this is called batched writing. It will be faster then issuing single write commands because

#. The Keyspace cluster will replicate them together
#. The client library will not wait for the previous' write commands response before send the next write command (saves rount-trip times).

In practice batched ``set`` can achieve 5-10x higher throughput than single ``set``.

To send batched write commands, first call ``begin()`` function, then issue the write commands, and finally call ``submit()``. The commands are sent on ``submit()``::

  client.begin()
  client.set("a1", "a1_value")
  client.set("a2", "a2_value")
  ...
  client.set("a99", "a99_value")
  client.submit() # commands are sent in batch

Issuing batched read commands
=============================

It is only possible to issue ``get`` read commands in a batched fashion. Since ``get`` commands are not replicated, only the round-trip time is saved. Nevertheless, batched ``get`` can achieve 3-5x higher throughput than single ``get``.

To send batched ``get`` commands, first call ``begin()`` function, then issue the ``get`` commands, and finally call ``submit()``. The commands are sent on ``submit()``. After the commands complete, the result object must be fetched and iterated to retrieve the individual key-value pairs::

  client.begin();
  client.get("a1")
  client.get("a2")
  ...
  client.get("a99")
  client.submit()

  # now iterate results
  while not client.result.is_end():
    # client.result.key() is a key!
    # client.result.value() is a value!
    print(client.result.key() + " => " + client.result.value())
    client.result.next()

Understanding Keyspace status codes
===================================

Keyspace exposes a rich set of status codes through the client library. These are especially useful for batched operations. After issuing command(s), there are four types of status codes which give information about the state of the Keyspace cluster.

To print the constant name of the status, use::

  keyspace.str_status(status)

``transport_status`` code
-------------------------

``transport_status`` tells the application the portion of commands that were sent to the Keyspace cluster::

  KEYSPACE_SUCCESS: all commands were sent
  KEYSPACE_PARTIAL: only a portion of the commands
                    could be sent before a timeout occured
  KEYSPACE_FAILURE: no commands could be sent

To retrieve the ``transport_status``, use::

  status = client.result.transport_status()
  print(keyspace.str_status(status))

``connectivity_status`` code
----------------------------

``connectivity_status`` tells the application the network conditions between the client and the Keyspace cluster::

  KEYSPACE_SUCCESS:      the master could be found
  KEYSPACE_NOMASTER:     some nodes were reachable,
                         but there was no master or it went down
  KEYSPACE_NOCONNECTION: the entire grid was unreachable within timeouts

To retrieve the ``connectivity_status``, use::

  status = client.result.connectivity_status()
  print(keyspace.str_status(status))

``timeout_status`` code
----------------------------

``timeout_status`` tells the application what timeouts occured, if any::

  KEYSPACE_SUCCESS:        no timeout occured, everything went fine
  KEYSPACE_MASTER_TIMEOUT: a master could not be found
                           within the master timeout
  KEYSPACE_GLOBAL_TIMEOUT: the blocking client library call
                           returned because the global timeout
                           has expired

To retrieve the ``timeout_status``, use::

  status = client.result.timeout_status()
  print(keyspace.str_status(status))

``command_status`` code
-----------------------

``command_status`` is the actual return value of a command::

  KEYSPACE_SUCCESS:   command succeeded
  KEYSPACE_FAILED:    the command was executed, but
                      its return value was FAILED;
                      eg. can happen for test_and_set if the test value
                      does not match or for get if the key does not exist
  KEYSPACE_NOSERVICE: the command was not executed

When using single or batched commands, retrieve the ``command_status`` like::

  status = client.result.command_status()
  print(keyspace.str_status(status))

Note that during result operation ``command_status`` changes to reflect the status of each command.
