.. _configuration:


*************
Configuration
*************

Required lines
==============

The required lines specify whether Keyspace is running in single or replicated mode.

Configuring a single node
-------------------------

Running Keyspace in single (not replicated) mode is useful during development and testing. In this mode, no replication takes place.

To configure Keyspace to run in single mode, put::

  mode = single

as the first line in your keyspace.conf file.

Single sample configuration
-------------------------------

Here is a sample configuration file for a single Keyspace node::

  mode = single
  
  database.dir = /var/keyspace
  
  http.port = 8080
  keyspace.port = 7080
  
  log.trace = false
  log.targets = stdout, file
  log.file = keyspace.log
  log.timestamping = false


Configuring replicated nodes
----------------------------

To configure Keyspace to run in replicated mode, put::

  mode = replicated

as the first line in your ``keyspace.conf`` file.

Next, identify this node in the replication unit. Numbering starts at 0, so for the zeroth node put::

  paxos.nodeID = 0

and so on. Unless you're doing something special, ``paxos.nodeID`` should be the only line that's different on your nodes, the remainder of the configuration file should be identical on all nodes!

Next, specify the nodes making up the replication unit. This is given by a list of IP addresses and ports, starting with the 0th node, then the 1st, and so on. The list always includes *all* the nodes, including the one you are configuring right now. Here's an example::

  paxos.endpoints = 192.168.1.50:10000, 192.168.1.51:10000, 192.168.1.52:10000

In this example each node is receiving the replication traffic on port 10000. It's a good idea to use the same ports on the different nodes for simplicity. Of course, if you're testing Keyspace on ``localhost``, you will use different ports here.

**Important:** Keyspace also uses the 2 ports above the ones specified in ``paxos.endpoints``, +1 for the master lease and +2 for catchup. So in the example above, Keyspace is listening on ports 10000-10002 on all three servers. Note that Keyspace uses TCP for all traffic.

Optional lines
==============

The optional lines only affect performance, allow you to change default port values and logging behaviour. In the examples below the default values are shown. We have listed the options in order of decreasing importance.

::

  keyspace.port = 7080

The port of the Keyspace client protocol. If you run multiple instances on the same host, this must be different for all instances.

::

  http.port = 8080


The port of the Keyspace HTTP server.  If you run multiple instances on the same host, this must be different for all instances.

::

  database.dir = .

The directory where the BerkeleyDB files are stored. If you run  multiple instances on the same host, this must be different for all nodes.

::

  database.cacheSize = 500M

Set the cache size of the backend database. Keyspace performance will degrade once the size of your database exceeds the cache size. Ideally, you should set this to a little less than the amount of RAM in your machine.

::

  database.logBufferSize = 250M

Sets the buffer size for transaction logs.

::

  database.warmCache = true

Warm the operating system's file cache by pre-reading the database files. First pre-reads all ``log.*`` files and then the main ``keyspace`` database file up to ``database.cacheSize`` bytes.

::

  log.trace = false

Whether to print out debug traces. Only fiddle with this if Keyspace is crashing and you want to send us a log file for debugging.
	

::

  log.targets = stdout

Options are ``stdout``, ``file`` and ``syslog``. More than one can be given, seperated with commas. 

::
	
  log.file = [empty]

The path of the log file to use if log.targets = file is given.

::
	
  log.timestamping = false

Whether to put a timestamp in front of log messages.

::

  daemon.user = [empty]

``daemon.user`` will cause Keyspace to drop to this user if started as root.

::

  database.directDB = true

Turn off system buffering of Berkeley DB database files to avoid double caching. See the `BerkeleyDB reference <http://www.oracle.com/technology/documentation/berkeley-db/db/api_reference/C/envset_flags.html>`_ for more.

::

  database.txnNoSync = false

If set, Berkeley DB will not write or synchronously flush the log on transaction commit. See the `BerkeleyDB reference <http://www.oracle.com/technology/documentation/berkeley-db/db/api_reference/C/envset_flags.html>`_ for more.

::

  database.txnWriteNoSync = true

If set, Berkeley DB will write, but will not synchronously flush, the log on transaction commit. See the `BerkeleyDB reference <http://www.oracle.com/technology/documentation/berkeley-db/db/api_reference/C/envset_flags.html>`_ for more.

::

  database.pageSize = 65536

Set the page size (in bytes) in the backend database. Leave this alone unless you know what you're doing. Must be a number less than or equal to 65536.

::

  database.checkpointTimeout = 60

Database checkpointing occurs this often in seconds. Checkpointing is when the storage engine compacts the transaction log files into the main database file. You will see the ``log*`` files disappearing and the file called ``keyspace``, which is the main database, increasing in size.

::
	
  database.numReaders = 20

Number of threads used for ``LIST`` and ``COUNT`` operations. Only fiddle with this if you expect to run a lot of concurrent ``LIST`` operations.

::

  database.verbose = false

Turns on BDB verbosity for debugging. Only fiddle with this if Keyspace is crashing and you want to send us a log file for debugging.

::

  rlog.cacheSize = 100000

Number of replication (Paxos) rounds cached on disk in the database. Only used when ``mode = replicated``. This is used to help lagging nodes catch up. Don't change this unless you know what you're doing.

::

  io.maxfd = 1024

Number of file descriptors used, roughly equal to the number of connections Keyspace will handle. You usually don't have to fiddle with this.

Replicated sample configuration
-------------------------------

Here is a sample configuration file for running a 3-way replicated cluster::

  mode = replicated
  
  paxos.nodeID = 0
  # paxos.nodeID must be 1 and 2 on the other nodes
  
  paxos.endpoints = 192.168.1.50:10000, 192.168.1.51:10000, 192.168.1.52:10000
  # paxos.endpoints must be the same on the other nodes
  
  database.dir = /var/keyspace
  
  http.port = 8080
  keyspace.port = 7080
  
  log.trace = false
  log.targets = stdout, file
  log.file = keyspace.log
  log.timestamping = false

Comments
========

Lines beginning with ``#`` are treated as comments::

  # this is a comment

Client configuration
====================

When a client connects to a Keyspace cluster, you have to tell the Keyspace client library where to connect to. This is the connection string: the host name(s) and the port numbers. **Very important: You always tell the client library the ``keyspace.port``s, that's where the client connects to. You never give the client the paxos.endpoints line!** In the example above, the ``keyspace.port`` is 7080, so the connection string would be::

  192.168.1.50:7080, 192.168.1.51:7080, 192.168.1.52:7080
  # not the same as paxos.endpoints - note the ports!

Tuning
======

Version 1.x of Keyspace uses `Oracle's BerkeleyDB <http://www.oracle.com/technology/products/berkeley-db/index.html>`_ transactional b-tree as its disk-based datastore. Configuration options that start with ``database.`` are all BDB related.

Cache sizes
-----------

As a rule of thumb, BDB will be much faster if the entire database fits into RAM. Under heavy, non-localized database load you will see performance degrade if your database does not fit into the amount of memory specified in ``database.cacheSize`` (default is 500MB).

Checkpointing and log cache sizes
---------------------------------

When performing writes, BDB puts them in the transaction log (these are the files that start with ``log.``). Every once in a while checkpointing occurs, at which point the modifications in the transaction log are merged into the main database file (called ``keyspace``). The checkpoint interval is specified by ``database.checkpointTimeout``, the default is 60 seconds. Note that checkpointing will not happen if at least 100MB of logs have not accumulated. Hence the default value of ``database.logBufferSize`` if a safe 250MB.

Page sizes
----------

Page sizes affect the granularity of database operations, and matter mostly when pages are read from and written to disk. The page size is specified by ``database.pageSize`` and ranges from 4096 (4K) to 65536 (64K). Be default, Keyspace uses ``database.pageSize = 65536``.

Page size won't matter much until your database fits into the cache specified by ``database.cacheSize``. Once you go past the cache size, non-local operations will be hurt by larger page sizes, while local operations will be faster.

There is one use-case in Keyspace where having a large page size is important: iteration. Since BDB is only able to iterate the database in-order, and the pages may be spread out on the physical disk, iteration will be slow if the page size is low. For example, with a random write pattern it is easy to produce a database file where iteration is no faster than 250K/sec if the page size is 4K. The same pattern, with the maximal 64K page size produces a database file where iteration is 16x times faster, an acceptable 4MB/s.

Why should you care about iteration? Due to the way BerkeleyDB is structured, iteration happens when you issue list, count and prune commands. Most importantly, when a node is lagging behind and it copies over the entire database from the master (see Understanding Keyspace for more), the master uses an iterator to write out its database to the lagging node. This needs to be fast, otherwise the lagging node will never catch up!

We recommend you use the default ``database.`` settings unless you have a highly specific I/O pattern.