.. _java_api:


********
Java API
********

Installing from source on Linux and other UNIX platforms
========================================================

You will need the JDK 1.6 or later to install the Java libraries. If you don't already have it, download it from the http://java.sun.com website. To make sure you have the JDK installed, type::

  find / -name jni.h

This command will also tell you where the header files are located on your system. Suppose ``jni.h`` is  located in ``/usr/lib/java/include``, then the ``make`` command to build the Keyspace Java libraries becomes::

  make javalib JAVA_INCLUDE=-I/usr/lib/java/include

This will create the file ``keyspace.jar`` and ``libkeyspace_client.so`` (or ``.dylib`` on MacOS) in ``bin/java``. Put the ``jar`` file into your classpath and the dynamic library into your ``java.library.path``, and you are ready to use Keyspace!

Note: On MacOS, you need to install the `Developer Tools <http://developer.apple.com/technologies/tools/>`_, and the headers are in ``/Developer/SDKs/MacOSX10.5.sdk/System/Library/Frameworks/JavaVM.framework/Headers``

Installing from source on Windows
=================================

Currently not supported.

Connecting to the Keyspace cluster
==================================

First, import the keyspace client library::

  import com.scalien.keyspace.*;                                                                                                               

The Java library uses a number of standard template containers, so it's best to import these, too::

  import java.util.ArrayList;                                                                                                                  
  import java.util.Collection;                                                                                                                 
  import java.util.TreeMap;

Then, create a client object by specifying the Keyspace cluster::

  String[] nodes = {"192.168.1.50:7080",
                    "192.168.1.51:7080",
                    "192.168.1.52:7080"};

  Client client = new Client(nodes);

Exceptions
==========

All Keyspace functions throw ``KeyspaceException`` on failure.

Setting timeout values
======================

Next, if you would like to, change the global timeout. The global timeout specifies the maximum time, in msec, that a Keyspace client call will block your application. The default is 120 seconds::

  client.setGlobalTimeout(120*1000);

Next, if you would like to, change the master timeout. The master timeout specifies the maximum time, in msec, that the library will spend trying to find the master node. The default is 21 seconds::

  client.setMasterTimeout(21*1000);

These fields also have the appropriate get functions ``getGlobalTimeout()`` and ``getMasterTimeout()``.

At this point, you are ready to start issuing commands.

Issuing single write commands
=============================

The Keyspace write commands are: ``set``, ``testAndSet``, ``rename``, ``add``, ``delete``, ``remove`` and ``prune``. Note that all Keyspace keys and values do not have to be NULL-terminated strings (eg. you can set a value to be a binary file).

``set`` command
---------------

The ``set`` command sets a ``key => value`` pair, creating a new pair if ``key`` did not previously exist, overwriting the old value if it did::

  client.set("key", "value");

``testAndSet`` command
------------------------

The ``testAndSet`` command conditionally and atomically sets a ``key => value`` pair, but only if the current ``value`` matches the user specified value ``test``::

  client.testAndSet("key", "test", "value");

``rename`` command
------------------

The ``rename`` command atomically renames a ``key``, leaving its ``value`` alone::

  client.rename("from", "to");

If the database looked like ``from => value`` at the beginning, then it changed to ``to => value`` after the successfull ``rename`` operation.

``add`` command
---------------

The ``add`` command takes the value of the key, parses it as a number and atomically increments it by the given offset::

  client.set("key", "10");
  long result = client.add("key", 3); // returns 13

If the database looked like ``key => 10`` at the beginning, then it changed to ``key => 13`` after the successfull ``add`` operation and the variable ``result`` holds the value 13.

``delete`` command
------------------

The ``delete`` command deletes a ``key => value`` pair by its ``key``::

  client.delete("key")

``remove`` command
------------------

The ``remove`` command deletes a ``key => value`` pair by its ``key`` and returns the old ``value``::

  client.set("key", "value");
  client.remove("key"); // returns "value"

``prune`` command
-----------------

The ``prune`` command deletes all ``key => value`` pairs where the ``key`` starts with the given prefix::

  client.prune("prefix");

For example::

  client.set("john", "john_data");
  client.set("jane", "jane_data");
  client.set("mark", "mark_data");
  client.prune("j"); // deletes "john" => "john_data" and "jane" => "jane_data"

Issuing single read commands
============================

The only Keyspace single read command is ``get``.

``get`` command
---------------

The ``get`` command retrieves a single value from the Keyspace cluster::

  client.set("key", "value");
  client.get("key"); // returns "value"

You can also issue the identical ``dirtyGet`` command, which will be serviced by all nodes, not just the master::

  client.set("key", "value");
  client.dirtyGet("key"); // may return "value"

Issuing list commands
=====================

There are two list commands: ``listKeys`` and ``listKeyValues`` and one ``count`` command, all have the same set of parameters.

``class ListParam``
-------------------

A list operation retrieves all keys from the Keyspace cluster which start with a given ``prefix``. Optionally:

- listing can start at a specified ``startKey``
- the maximum number of keys to return can be specified with the ``count`` parameter
- listing can proceed forward or backward
- listing can skip the first key

A ``ListParams`` is a special class to make it more convenient to pass the various parameters to list functions. Its member functions are::

  public ListParams setPrefix(String prefix)

  public ListParams setStartKey(String startKey)

  public ListParams setCount(long count)

  public ListParams setSkip(boolean skip)

  public ListParams setForward(boolean forward)

Note that the functions return the current object, so they can be chained, like::

  new ListParams().setPrefix("/user:").setCount(100)

The default values are::

  prefix = "";

  startKey = "";

  count = 0; // no limit

  skip = false;

  forward = true;

``listKeys`` command
---------------------

The signature of the function is::

  public ArrayList<String> listKeys(ListParams params) throws KeyspaceException

The result of a list operation is a standard ``ArrayList<String>``::

  client.set("/user:mtrencseni", "mtrencseni_data");
  client.set("/user:agazso",     "agazso_data");
  client.listKeys(new ListParams().setPrefix("/user:"));
  // ['/user:agazso', '/user:mtrencseni']

You can also issue the identical ``dirtyListKeys`` command, which will be serviced by all nodes, not just the master.

``listKeyValues`` command
--------------------------

The ``listKeyValues`` command in nearly identical to ``listKeys``, except it also returns the values.

The ``listKeyValues`` command retrieves all keys and values from the Keyspace cluster which start with a given ``prefix``. The signature of the function is::

  TreeMap<String, String> listKeyValues(ListParams params) throws KeyspaceException

The result of a list operation is a standard ``TreeMap<String, String>``::

  client.set("/user:mtrencseni", "mtrencseni_data");
  client.set("/user:agazso",     "agazso_data");
  client.listKeyValues(new ListParams().setPrefix("/user:"));
  // {'/user:mtrencseni': 'mtrencseni_data', '/user:agazso': 'agazso_data'}

You can also issue the identical ``dirtyListKeyValues`` command, which will be serviced by all nodes, not just the master.

``count`` command
-----------------

The ``count`` command has the same parameters as ``listKeys`` or ``listKeyValues``, but returns the number of keys (or key-value pairs) that they would return. The signature of the function is::

  public long count(ListParams params) throws KeyspaceException

  client.count(new ListParams().setPrefix("/user:"));

You can also issue the identical ``dirtyCount`` command, which will be serviced by all nodes, not just the master.

Issuing batched write commands
==============================

For maximum thruput performance, it is possible to issue many write commands together; this is called batched writing. It will be faster then issuing single write commands because

#. The Keyspace cluster will replicate them together
#. The client library will not wait for the previous' write commands response before send the next write command (saves rount-trip times).

In practice batched ``set`` can achieve 5-10x higher throughput than single ``set``.

To send batched write commands, first call ``begin()`` function, then issue the write commands, and finally call ``submit()``. The commands are sent on ``submit()``::

  client.begin();
  client.set("a1", "a1_value");
  client.set("a2", "a2_value");
  ...
  client.set("a99", "a99_value");
  client.submit(); // commands are sent in batch

Issuing batched read commands
=============================

It is possible to issue ``get`` read commands in a batched fashion. Since ``get`` commands are not replicated, only the round-trip time is saved. Nevertheless, batched ``get`` can achieve 3-5x higher throughput than single ``get``.

To send batched ``get`` commands, first call ``begin()`` function, then issue the ``get`` commands, and finally call ``submit()``. The commands are sent on ``submit()``. After the commands complete, the results are returned as a ``TreeMap<String, String>`` ::

  client.set("/user:mtrencseni", "mtrencseni_data");
  client.set("/user:agazso",     "agazso_data");
  client.begin();
  client.get("/user:mtrencseni");
  client.get("/user:agazso");
  client.submit();

  // fetch result
  client.getResult().getKeyValues();
  // {'/user:mtrencseni': 'mtrencseni_data', '/user:agazso': 'agazso_data'}

Understanding Keyspace status codes
===================================

Keyspace exposes a rich set of status codes through the client library. These are especially useful for batched operations. After issuing command(s), there are four types of status codes which give information about the state of the Keyspace cluster.

The status is always returned as an ``int`` code.

To print the constant name of the status, use the static ``toString()`` method of the ``Status`` class::

  Status.toString(int status)

``transportStatus`` code
-------------------------

``transportStatus`` tells the application the portion of commands that were sent to the Keyspace cluster::

  KEYSPACE_SUCCESS: all commands were sent
  KEYSPACE_PARTIAL: only a portion of the commands
                    could be sent before a timeout occured
  KEYSPACE_FAILURE: no commands could be sent

To retrieve the ``transportStatus``, use::

  int status = client.getResult().getTransportStatus();
  System.out.println(Status.toString(status));

``connectivityStatus`` code
----------------------------

``connectivityStatus`` tells the application the network conditions between the client and the Keyspace cluster::

  KEYSPACE_SUCCESS:      the master could be found
  KEYSPACE_NOMASTER:     some nodes were reachable,
                         but there was no master or it went down
  KEYSPACE_NOCONNECTION: the entire grid was unreachable within timeouts

To retrieve the ``connectivityStatus``, use::

  int status = client.getResult().getConnectivityStatus();
  System.out.println(Status.toString(status));

``timeoutStatus`` code
----------------------------

``timeoutStatus`` tells the application what timeouts occured, if any::

  KEYSPACE_SUCCESS:        no timeout occured, everything went fine
  KEYSPACE_MASTER_TIMEOUT: a master could not be found
                           within the master timeout
  KEYSPACE_GLOBAL_TIMEOUT: the blocking client library call
                           returned because the global timeout
                           has expired

To retrieve the ``timeoutStatus``, use::

  int status = client.getResult().getTimeoutStatus();
  System.out.println(Status.toString(status));

``commandStatus`` code
-----------------------

``commandStatus`` is the actual return value of a command::

  KEYSPACE_SUCCESS:   command succeeded
  KEYSPACE_FAILED:    the command was executed, but
                      its return value was FAILED;
                      eg. can happen for test_and_set if the test value
                      does not match or for get if the key does not exist
  KEYSPACE_NOSERVICE: the command was not executed

When using single or batched commands, retrieve the ``commandStatus`` like::

  int status = client.getResult().getCommandStatus();
  System.out.println(Status.toString(status));

Header files
============

Check out ``src/Application/Keyspace/Client/Java/{Client, ListParams, Result, Status}.java`` for a full reference!

