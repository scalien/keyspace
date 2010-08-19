.. _running:


***********************************
Running Keyspace for the first time 
***********************************

Keyspace takes one command argument: the configuration file. Note that all paths in the configuration file such as ``database.dir`` must either be absolute or relative to the working directory. So, if the Keyspace executable is in ``/home/joe/keyspace/bin``, the configuration file is in ``/home/joe/keyspace/0/keyspace.conf`` and the configuration file specifies ``database.dir = 0``, then you should probably launch Keyspace from ``/home/joe/keyspace`` using::

  $ bin/keyspace 0/keyspace.conf

You should see something like this::

  Keyspace v1.5.1 r1231 started
  Opening database...
  Database opened
  Database is empty
  Node 0 is the master

Note that for master election to be successful a majority of nodes have to be up and running.

Verifying the installation
==========================

At this point, it's a good idea to examine the HTTP status of the nodes. Assuming the default configuration, where Keyspace's built-in web server is running on port 8080, direct your browser to ``http://<ip-address-of-a-node>:8080``. You should see something like this::

  Keyspace v1.5.1 r1231

  Running in replicated mode with 3 nodes

  I am node 0

  Master is node 0

  I am in replication round 1

  Last replication round was 0 bytes, took 0 msec, thruput was 1 KB/sec

Be sure to examine all ``n`` nodes.

Verifying the cluster takes writes
==================================

Using the HTTP output from above, find the master node. Next, send a ``SET`` command to the master using the HTTP API, by typing the following into your browser::

  http://<ip-address-of-master-node>:8080/set?key=testkey&value=testvalue

If everything is working, it replies with ``OK``. Now make sure the value was stored properly::

  http://<ip-address-of-master-node>:8080/get?key=testkey

It should print ``testvalue``. Next, make sure the command was replicated to the other ``n-1`` nodes. Non-master nodes do not serve ``GET`` request (only the master), but they do serve ``DIRTYGET`` request::

  http://<ip-address-of-non-master-node>:8080/dirtyget?key=testkey

It should print ``testvalue``. For a complete command reference, see the next section.

Verifying the cluster recovers from failures
============================================

Next, shut down one of the nodes (eg. using ``Control+C``). Verify that the node is not up by looking at its HTTP port. Next, issue a ``SET`` to the remaining nodes::

  http://<ip-address-of-master-node>:8080/set?key=testkey2&value=testvalue2

Launch the node again, and after 7 seconds, once it reports the master, make sure it has caught up to the rest of the cluster by querying ``testkey2`` from it::

  http://<ip-address-of-non-master-node>:8080/dirtyget?key=testkey2

It should print ``testvalue2``.

You can think of any number of such tests. To fully understand how Keyspace handles failures, how it catches up nodes, please read the section Understanding Keyspace.