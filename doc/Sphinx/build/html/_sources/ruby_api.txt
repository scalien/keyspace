.. _ruby_api:


********
Ruby API
********

Installing from source on Linux and other UNIX platforms
========================================================

You will need the Ruby runtime sources to install the Ruby libraries. If you don't already have it, download it from the http://java.sun.com website. To make sure you have the JDK installed, type::

  find / -name ruby.h

This command will also tell you where the header files are located on your system. Suppose ``ruby.h`` is  located in ``/usr/include/ruby``, then the ``make`` command to build the Keyspace Ruby libraries becomes::

  make rubylib RUBY_INCLUDE=-I/usr/include/ruby

This will create the file ``keyspace.rb`` and ``keyspace_client.ruby`` in ``bin/ruby``. Put both files into your project directory, and you are ready to use Keyspace!

Note: On MacOS, you need to install the `Developer Tools <http://developer.apple.com/technologies/tools/>`_, and the headers are in ``/Developer/SDKs/MacOSX10.5.sdk/System/Library/Frameworks/Ruby.framework/Versions/1.8/Headers``

Installing from source on Windows
=================================

Currently not supported.

Connecting to the Keyspace cluster
==================================

First, import the keyspace client library::

  require 'keyspace'

Then, create a client object by specifying the Keyspace cluster::

  client = KeyspaceClient.new(["192.168.1.50:7080",
                               "192.168.1.51:7080",
                               "192.168.1.52:7080"])

Return values
=============

All Keyspace functions return ``nil`` on failure.

Setting timeout values
======================

Next, if you would like to, change the global timeout. The global timeout specifies the maximum time, in msec, that a Keyspace client call will block your application. The default is 120 seconds::

  client.set_global_timeout(120*1000)

Next, if you would like to, change the master timeout. The master timeout specifies the maximum time, in msec, that the library will spend trying to find the master node. The default is 21 seconds::

  client.set_master_timeout(21*1000)

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

If the database looked like ``from => value`` at the beginning, then it changed to ``to => value`` after the successfull ``rename`` operation.

``add`` command
---------------

The ``add`` command takes the value of the key, parses it as a number and atomically increments it by the given offset::

  client.set("key", str(10))
  result = client.add("key", 3) # returns 13

If the database looked like ``key => 10`` at the beginning, then it changed to ``key => 13`` after the successfull ``add`` operation and the variable ``result`` holds the value 13.

``delete`` command
------------------

The ``delete`` command deletes a ``key => value`` pair by its ``key``::

  client.delete("key")

``remove`` command
------------------

The ``remove`` command deletes a ``key => value`` pair by its ``key`` and returns the old ``value``::

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

Issuing key expiry commands
===========================

``set_expiry`` command
----------------------

The ``set_expiry`` sets an expiry on the key ``key`` to occur in ``t`` seconds. The command will succeed and set the expiry irrespective of whether the key exists. If the key is created in the meantime, it will be expired when the timeout occurs. The command replaces any active expiry on the key::

  client.set_expiry("key", 60);

Key will be deleted in 60 seconds.

``remove_expiry`` command
-------------------------

Removes any outstanding expiry on the key. The command will succeed irrespective of whether an expiry is set for the key::

  client.remove_expiry("key")

``clear_expiries`` command
--------------------------

Clears all expiries in the database::

  client.clear_expiries()


Issuing single read commands
============================

The only Keyspace single read command is ``get``.

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

A list operation retrieves all keys from the Keyspace cluster which start with a given ``prefix``. Optionally:

- listing can start at a specified ``startKey``
- the maximum number of keys to return can be specified with the ``count`` parameter
- listing can proceed forward or backward
- listing can skip the first key

List type functions take an associative array as their arguemnts, which can contain the following parameters: ``prefix, start_key, count, skip, forward``.

The default values are::

  "prefix" : ""

  "start_key" : ""

  "count" : 0 // no limit

  "skip" : false

  "forward" : true

``list_keys`` command
---------------------

The signature of the function is::

  def list_keys(args = {}) # returns an array

The result of a list operation is a standard ``array``::

  client.set("/user:mtrencseni", "mtrencseni_data")
  client.set("/user:agazso",     "agazso_data")
  client.list_keys({"prefix": "/user:"})
  // ["/user:agazso", "/user:mtrencseni"]

You can also issue the identical ``dirty_list_keys`` command, which will be serviced by all nodes, not just the master.

``list_key_values`` command
---------------------------

The ``list_key_values`` command in nearly identical to ``list_keys``, except it also returns the values.

The ``list_key_values`` command retrieves all keys and values from the Keyspace cluster which start with a given ``prefix``. The signature of the function is::

  def list_key_values(args = {}) # returns a hash

The result of a list operation is a standard ``array``::

  client.set("/user:mtrencseni", "mtrencseni_data");
  client.set("/user:agazso",     "agazso_data");
  client.list_key_values({"prefix": "/user:"});
  // { "/user:mtrencseni": "mtrencseni_data",
  //   "/user:agazso"    : "agazso_data"}

You can also issue the identical ``dirty_list_key_values`` command, which will be serviced by all nodes, not just the master.

``count`` command
-----------------

The ``count`` command has the same parameters as ``list_keys`` or ``list_key_values``, but returns the number of keys (or key-value pairs) that they would return. The signature of the function is::

  def count(args = {}) # returns an int

  client.count({"prefix": "/user:"});

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

It is possible to issue ``get`` read commands in a batched fashion. Since ``get`` commands are not replicated, only the round-trip time is saved. Nevertheless, batched ``get`` can achieve 3-5x higher throughput than single ``get``.

To send batched ``get`` commands, first call ``begin()`` function, then issue the ``get`` commands, and finally call ``submit()``. The commands are sent on ``submit()``. After the commands complete, the results are returned as a standard python dictionary ::

  client.set("/user:mtrencseni", "mtrencseni_data")
  client.set("/user:agazso",     "agazso_data")
  client.begin()
  client.get("/user:mtrencseni")
  client.get("/user:agazso")
  client.submit()

  # fetch result
  client.result.key_values()
  # => {'/user:mtrencseni': 'mtrencseni_data', '/user:agazso': 'agazso_data'}

Understanding Keyspace status codes
===================================

Keyspace exposes a rich set of status codes through the client library. These are especially useful for batched operations. After issuing command(s), there are four types of status codes which give information about the state of the Keyspace cluster.

To print the constant name of the status, use::

  KeyspaceClient :: status_string(status)

``transport_status`` tells the application the portion of commands that were sent to the Keyspace cluster::

  KEYSPACE_SUCCESS: all commands were sent
  KEYSPACE_PARTIAL: only a portion of the commands
                    could be sent before a timeout occured
  KEYSPACE_FAILURE: no commands could be sent

To retrieve the ``transport_status``, use::

  status = client.result.transport_status()
  print(KeyspaceClient::status_string(status))

``connectivity_status`` code
----------------------------

``connectivity_status`` tells the application the network conditions between the client and the Keyspace cluster::

  KEYSPACE_SUCCESS:      the master could be found
  KEYSPACE_NOMASTER:     some nodes were reachable,
                         but there was no master or it went down
  KEYSPACE_NOCONNECTION: the entire grid was unreachable within timeouts

To retrieve the ``connectivity_status``, use::

  status = client.result.connectivity_status()
  print(KeyspaceClient::status_string(status))

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
  print(KeyspaceClient::status_string(status))

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
  print(KeyspaceClient::status_string(status))

Header files
============

Check out ``src/Application/Keyspace/Client/Ruby/keyspace.rb`` for a full reference!
