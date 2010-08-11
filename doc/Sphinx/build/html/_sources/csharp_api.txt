.. _csharp_api:


******
C# API
******

Installing from binaries on Windows
=================================

Download and extract the Windows zip distribution. The ``bin\csharp`` folder contains two files: ``keyspace_client.dll`` and ``KeyspaceClient.dll``. Add ``KeyspaceClient.dll`` to your C# project's References folder by right-clicking and choosing Add reference and then selecting the file in the Browse tab. Then make sure the project's working directory contains the keyspace_client.dll file.

Installing from source on Windows
=================================

Optionally, you can also re-compile the above files. First, open ``Keyspace.vcproj\Keyspace.sln`` and build KeyspaceClientLib and KeyspaceClintCSharpDLL (creates ``keyspace_client.dll``), then open ``src\Application\Keyspace\Client\CSharp\KeyspaceClient.sln`` and build KeyspaceClient (creates ``KeyspaceClient.dll``).

Connecting to the Keyspace cluster
==================================

First, import the keyspace client library::

  using Keyspace;

The C# library uses a number of standard template containers, so it's best to import these, too::

  using System;
  using System.Collections.Generic;
  using System.Test;

Then, create a client object by specifying the Keyspace cluster::

  String[] nodes = {"192.168.1.50:7080",
                    "192.168.1.51:7080",
                    "192.168.1.52:7080"};

  Client client = new Client(nodes);

Exceptions
==========

All Keyspace functions throw ``Keyspace.Exception`` on failure.

Setting timeout values
======================

Next, if you would like to, change the global timeout. The global timeout specifies the maximum time, in msec, that a Keyspace client call will block your application. The default is 120 seconds::

  client.SetGlobalTimeout(120*1000);

Next, if you would like to, change the master timeout. The master timeout specifies the maximum time, in msec, that the library will spend trying to find the master node. The default is 21 seconds::

  client.SetMasterTimeout(21*1000);

These fields also have the appropriate get functions ``GetGlobalTimeout()`` and ``GetMasterTimeout()``.

At this point, you are ready to start issuing commands.

Issuing single write commands
=============================

The Keyspace write commands are: ``Set``, ``TestAndSet``, ``Rename``, ``Add``, ``Delete``, ``Remove``, ``Prune`` and key expiry commands. Note that all Keyspace keys and values do not have to be NULL-terminated strings (eg. you can set a value to be a binary file).

``Set`` command
---------------

The ``Set`` command sets a ``key => value`` pair, creating a new pair if ``key`` did not previously exist, overwriting the old value if it did::

  client.Set("key", "value");

``TestAndSet`` command
------------------------

The ``TestAndSet`` command conditionally and atomically sets a ``key => value`` pair, but only if the current ``value`` matches the user specified value ``test``::

  client.TestAndSet("key", "test", "value");

``Rename`` command
------------------

The ``Rename`` command atomically renames a ``key``, leaving its ``value`` alone::

  client.Rename("from", "to");

If the database looked like ``from => value`` at the beginning, then it changed to ``to => value`` after the successfull ``Rename`` operation.

``Add`` command
---------------

The ``Add`` command takes the value of the key, parses it as a number and atomically increments it by the given offset::

  client.Set("key", "10");
  long result = client.Add("key", 3); // returns 13

If the database looked like ``key => 10`` at the beginning, then it changed to ``key => 13`` after the successfull ``Add`` operation and the variable ``result`` holds the value 13.

``Delete`` command
------------------

The ``Delete`` command deletes a ``key => value`` pair by its ``key``::

  client.Delete("key")

``Remove`` command
------------------

The ``Remove`` command deletes a ``key => value`` pair by its ``key`` and returns the old ``value``::

  client.Set("key", "value");
  client.Remove("key"); // returns "value"

``Prune`` command
-----------------

The ``Prune`` command deletes all ``key => value`` pairs where the ``key`` starts with the given prefix::

  client.Prune("prefix");

For example::

  client.Set("john", "john_data");
  client.Set("jane", "jane_data");
  client.Set("mark", "mark_data");
  client.Prune("j"); // deletes "john" => "john_data" and "jane" => "jane_data"

Issuing key expiry commands
===========================

``SetExpiry`` command
----------------------

The ``SetExpiry`` sets an expiry on the key ``key`` to occur in ``t`` seconds. The command will succeed and set the expiry irrespective of whether the key exists. If the key is created in the meantime, it will be expired when the timeout occurs. The command replaces any active expiry on the key::

  client.SetExpiry("key", 60);

Key will be deleted in 60 seconds.

``RemoveExpiry`` command
-------------------------

Removes any outstanding expiry on the key. The command will succeed irrespective of whether an expiry is set for the key::

  client.RemoveExpiry("key")

``ClearExpiries`` command
--------------------------

Clears all expiries in the database::

  client.ClearExpiries()

Issuing single read commands
============================

The only Keyspace single read command is ``Get``.

``Get`` command
---------------

The ``Get`` command retrieves a single value from the Keyspace cluster::

  client.Set("key", "value");
  client.Get("key"); // returns "value"

You can also issue the identical ``DirtyGet`` command, which will be serviced by all nodes, not just the master::

  client.Set("key", "value");
  client.DirtyGet("key"); // may return "value"

Issuing list commands
=====================

There are two list commands: ``ListKeys`` and ``ListKeyValues`` and one ``Count`` command, all have the same set of parameters.

``class ListParam``
-------------------

A list operation retrieves all keys from the Keyspace cluster which start with a given ``prefix``. Optionally:

- listing can start at a specified ``startKey``
- the maximum number of keys to return can be specified with the ``count`` parameter
- listing can proceed forward or backward
- listing can skip the first key

A ``ListParams`` is a special class to make it more convenient to pass the various parameters to list functions. Its member functions are::

  public ListParam SetPrefix(String prefix)

  public ListParam SetStartKey(String startKey)

  public ListParam SetCount(long count)

  public ListParam SetSkip(boolean skip)

  public ListParam SetForward(boolean forward)

Note that the functions return the current object, so they can be chained, like::

  new ListParam().SetPrefix("/user:").SetCount(100)

The default values are::

  prefix = "";

  startKey = "";

  count = 0; // no limit

  skip = false;

  forward = true;

``listKeys`` command
---------------------

The signature of the function is::

  public List<string> ListKeys(ListParam lp)

The result of a list operation is a standard ``List<string>``::

  client.Set("/user:mtrencseni", "mtrencseni_data");
  client.Set("/user:agazso",     "agazso_data");
  client.ListKeys(new ListParam().setPrefix("/user:"));
  // ['/user:agazso', '/user:mtrencseni']

You can also issue the identical ``DirtyListKeys`` command, which will be serviced by all nodes, not just the master.

``ListKeyValues`` command
--------------------------

The ``ListKeyValues`` command in nearly identical to ``ListKeys``, except it also returns the values.

The ``ListKeyValues`` command retrieves all keys and values from the Keyspace cluster which start with a given ``prefix``. The signature of the function is::

  Dictionary<string, string> ListKeyValues(ListParam lp)

The result of a list operation is a standard ``Dictionary<string, string>``::

  client.Set("/user:mtrencseni", "mtrencseni_data");
  client.Set("/user:agazso",     "agazso_data");
  client.ListKeyValues(new ListParam().SetPrefix("/user:"));
  // {'/user:mtrencseni': 'mtrencseni_data', '/user:agazso': 'agazso_data'}

You can also issue the identical ``DirtyListKeyValues`` command, which will be serviced by all nodes, not just the master.

``Count`` command
-----------------

The ``Count`` command has the same parameters as ``ListKeys`` or ``ListKeyValues``, but returns the number of keys (or key-value pairs) that they would return. The signature of the function is::

  public long Count(ListParam lp)

  client.Count(new ListParam().SetPrefix("/user:"));

You can also issue the identical ``DirtyCount`` command, which will be serviced by all nodes, not just the master.

Issuing batched write commands
==============================

For maximum thruput performance, it is possible to issue many write commands together; this is called batched writing. It will be faster then issuing single write commands because

#. The Keyspace cluster will replicate them together
#. The client library will not wait for the previous' write commands response before send the next write command (saves rount-trip times).

In practice batched ``Set`` can achieve 5-10x higher throughput than single ``Set``.

To send batched write commands, first call ``Begin()`` function, then issue the write commands, and finally call ``Submit()``. The commands are sent on ``Submit()``::

  client.Begin();
  client.Set("a1", "a1_value");
  client.Set("a2", "a2_value");
  ...
  client.Set("a99", "a99_value");
  client.Submit(); // commands are sent in batch

Issuing batched read commands
=============================

It is possible to issue ``Get`` read commands in a batched fashion. Since ``Get`` commands are not replicated, only the round-trip time is saved. Nevertheless, batched ``Get`` can achieve 3-5x higher throughput than single ``Get``.

To send batched ``Get`` commands, first call ``Begin()`` function, then issue the ``Get`` commands, and finally call ``Submit()``. The commands are sent on ``Submit()``. After the commands complete, the results are returned as a ``Dictionary<string, string>`` ::

  client.Set("/user:mtrencseni", "mtrencseni_data");
  client.Set("/user:agazso",     "agazso_data");
  client.Begin();
  client.Get("/user:mtrencseni");
  client.Get("/user:agazso");
  client.Submit();

  // fetch result
  client.GetResult().GetKeyValues();
  // {'/user:mtrencseni': 'mtrencseni_data', '/user:agazso': 'agazso_data'}

Understanding Keyspace status codes
===================================

Keyspace exposes a rich set of status codes through the client library. These are especially useful for batched operations. After issuing command(s), there are four types of status codes which give information about the state of the Keyspace cluster.

The status is always returned as an ``int`` code.

To print the constant name of the status, use the static ``ToString()`` method of the ``Status`` class::

  Status.ToString(int status)

``TransportStatus`` code
-------------------------

``TransportStatus`` tells the application the portion of commands that were sent to the Keyspace cluster::

  KEYSPACE_SUCCESS: all commands were sent
  KEYSPACE_PARTIAL: only a portion of the commands
                    could be sent before a timeout occured
  KEYSPACE_FAILURE: no commands could be sent

To retrieve the ``TransportStatus``, use::

  int status = client.GetResult().GetTransportStatus();
  Console.WriteLine(Keyspace.Status.ToString(status));

``ConnectivityStatus`` code
----------------------------

``ConnectivityStatus`` tells the application the network conditions between the client and the Keyspace cluster::

  KEYSPACE_SUCCESS:      the master could be found
  KEYSPACE_NOMASTER:     some nodes were reachable,
                         but there was no master or it went down
  KEYSPACE_NOCONNECTION: the entire grid was unreachable within timeouts

To retrieve the ``ConnectivityStatus``, use::

  int status = client.GetResult().GetConnectivityStatus();
  Console.WriteLine(Keyspace.Status.ToString(status));

``TimeoutStatus`` code
----------------------------

``TimeoutStatus`` tells the application what timeouts occured, if any::

  KEYSPACE_SUCCESS:        no timeout occured, everything went fine
  KEYSPACE_MASTER_TIMEOUT: a master could not be found
                           within the master timeout
  KEYSPACE_GLOBAL_TIMEOUT: the blocking client library call
                           returned because the global timeout
                           has expired

To retrieve the ``TimeoutStatus``, use::

  int status = client.GetResult().GetTimeoutStatus();
  Console.WriteLine(Keyspace.Status.ToString(status));

``CommandStatus`` code
-----------------------

``CommandStatus`` is the actual return value of a command::

  KEYSPACE_SUCCESS:   command succeeded
  KEYSPACE_FAILED:    the command was executed, but
                      its return value was FAILED;
                      eg. can happen for test_and_set if the test value
                      does not match or for get if the key does not exist
  KEYSPACE_NOSERVICE: the command was not executed

When using single or batched commands, retrieve the ``commandStatus`` like::

  int status = client.GetResult().GetCommandStatus();
  Console.WriteLine(Keyspace.Status.ToString(status));

Header files
============

Check out ``src/Application/Keyspace/Client/CSharp/KeyspaceClient/{Client, ListParam, Result, Status}.cs`` for a full reference!

